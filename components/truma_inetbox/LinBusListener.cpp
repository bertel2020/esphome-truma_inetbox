#include "LinBusListener.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "helpers.h"
#ifdef USE_ESP32
#include <esp_rom_sys.h>
#endif

namespace esphome {
namespace truma_inetbox {

static const char *const TAG = "truma_inetbox.LinBusListener";

#define LIN_BREAK 0x00
#define LIN_SYNC 0x55
#define DIAGNOSTIC_FRAME_MASTER 0x3c
#define DIAGNOSTIC_FRAME_SLAVE 0x3d
#define QUEUE_WAIT_DONT_BLOCK (TickType_t) 0

void LinBusListener::dump_config() {
  ESP_LOGCONFIG(TAG, "LinBusListener:");
  LOG_PIN("  CS Pin: ", this->cs_pin_);
  LOG_PIN("  FAULT Pin: ", this->fault_pin_);
  LOG_UPDATE_INTERVAL(this);
  ESP_LOGCONFIG(TAG, "  LIN checksum Version: %d", this->lin_checksum_ == LIN_CHECKSUM::LIN_CHECKSUM_VERSION_1 ? 1 : 2);
  ESP_LOGCONFIG(TAG, "  Observer mode: %s", YESNO(this->observer_mode_));
  this->check_uart_settings(9600, 2, esphome::uart::UART_CONFIG_PARITY_NONE, 8);
}

void LinBusListener::setup() {
  ESP_LOGCONFIG(TAG, "Setting up LIN BUS...");
  this->time_per_baud_ = (1000.0f * 1000.0f / this->parent_->get_baud_rate());
  this->time_per_lin_break_ = this->time_per_baud_ * this->lin_break_length * 1.1f;
  this->time_per_pid_ = this->time_per_baud_ * this->frame_length_ * 1.1f;
  this->time_per_first_byte_ = this->time_per_baud_ * this->frame_length_ * 10.0f;
  this->time_per_byte_ = this->time_per_baud_ * this->frame_length_ * 3.0f;

  if (this->cs_pin_ != nullptr) {
    this->cs_pin_->setup();
  }

  if (this->fault_pin_ != nullptr) {
    this->fault_pin_->setup();
  }

  this->setup_framework();

#if ESPHOME_LOG_LEVEL > ESPHOME_LOG_LEVEL_NONE
  assert(this->log_queue_ != 0);
#endif

  if (this->cs_pin_ != nullptr) {
    this->cs_pin_->digital_write(!this->observer_mode_);
  }
}

void LinBusListener::update() { this->check_for_lin_fault_(); }

void LinBusListener::loop() {
  if (!this->check_for_lin_fault_()) {
    if (this->available() > 0 || this->current_state_ == READ_STATE_DATA) {
      this->on_receive_();
    }
  }

  this->process_lin_msg_queue(QUEUE_WAIT_DONT_BLOCK);

#if ESPHOME_LOG_LEVEL > ESPHOME_LOG_LEVEL_NONE
  this->process_log_queue(QUEUE_WAIT_DONT_BLOCK);
#endif
}

void LinBusListener::write_lin_answer_(const uint8_t *data, uint8_t len) {
  QUEUE_LOG_MSG log_msg = QUEUE_LOG_MSG();
  if (!this->can_write_lin_answer_) {
    log_msg.type = QUEUE_LOG_MSG_TYPE::ERROR_LIN_ANSWER_CAN_WRITE_LIN_ANSWER;
    TRUMA_LOGE(log_msg);
    return;
  }
  this->can_write_lin_answer_ = false;
  if (len > 8) {
    log_msg.type = QUEUE_LOG_MSG_TYPE::ERROR_LIN_ANSWER_TOO_LONG;
    TRUMA_LOGE(log_msg);
    return;
  }

  uint8_t data_CRC = 0;
  if (this->lin_checksum_ == LIN_CHECKSUM::LIN_CHECKSUM_VERSION_1 || this->current_PID_ == DIAGNOSTIC_FRAME_SLAVE) {
    data_CRC = data_checksum(data, len, 0);
  } else {
    data_CRC = data_checksum(data, len, this->current_PID_with_parity_);
  }

  if (!this->observer_mode_) {
    this->current_PID_order_answered_ = true;
    this->write_array(data, len);
    this->write(data_CRC);
    // flush() waits for TX FIFO to drain (~(len+1) * 1.15ms at 9600 baud).
    // Echo bytes arrive in RX buffer during transmission on single-wire LIN bus.
    this->flush();
    // Read back echo bytes that have arrived so far.
    // No extra delay — remaining echo bytes (0x03 node address) are not valid
    // BREAK/SYNC bytes and will be discarded by the state machine.
    uint8_t echo;
    for (uint8_t i = 0; i < (len + 1); i++) {
      if (this->available()) this->read_byte(&echo);
    }
  }

  log_msg.type = QUEUE_LOG_MSG_TYPE::VERBOSE_LIN_ANSWER_RESPONSE;
  log_msg.current_PID = this->current_PID_;
  for (uint8_t i = 0; i < len; i++) {
    log_msg.data[i] = data[i];
  }
  log_msg.data[len] = data_CRC;
  log_msg.len = len++;
  TRUMA_LOGV(log_msg);
}

bool LinBusListener::check_for_lin_fault_() {
  if (this->fault_pin_ != nullptr) {
    if (!this->fault_pin_->digital_read()) {
      if (this->fault_on_lin_bus_reported_ < 0xFF) {
        this->fault_on_lin_bus_reported_++;
      } else {
        this->fault_on_lin_bus_reported_ = 0x0F;
      }
      if (this->fault_on_lin_bus_reported_ % 3 == 0) {
        QUEUE_LOG_MSG log_msg = QUEUE_LOG_MSG();
        log_msg.type = QUEUE_LOG_MSG_TYPE::ERROR_CHECK_FOR_LIN_FAULT_DETECTED;
        TRUMA_LOGE(log_msg);
      }
    } else if (this->get_lin_bus_fault()) {
      this->fault_on_lin_bus_reported_ = 0;
      QUEUE_LOG_MSG log_msg = QUEUE_LOG_MSG();
      log_msg.type = QUEUE_LOG_MSG_TYPE::INFO_CHECK_FOR_LIN_FAULT_FIXED;
      TRUMA_LOGI(log_msg);
    } else {
      this->fault_on_lin_bus_reported_ = 0;
    }
  }

  if (this->get_lin_bus_fault()) {
    this->current_state_reset_();
    this->clear_uart_buffer_();
    return true;
  } else {
    return false;
  }
}

void LinBusListener::on_receive_() {
  while (this->available() || this->current_state_ == READ_STATE_DATA) {
    const uint32_t now = micros();

    if (this->last_data_recieved_ != 0 &&
        (now - this->last_data_recieved_) > this->time_per_lin_break_) {
      this->current_state_ = READ_STATE_BREAK;
    }

    bool had_data = this->available();
    this->read_lin_frame_();
    // Only update timestamp when a byte was consumed, not on empty-wait
    if (had_data) {
      this->last_data_recieved_ = micros();
    }

    // If still in DATA state but no bytes available, exit and let next loop() call retry
    if (this->current_state_ == READ_STATE_DATA && !this->available()) {
      break;
    }
  }
}

void LinBusListener::read_lin_frame_() {
  uint8_t buf;
  QUEUE_LOG_MSG log_msg = QUEUE_LOG_MSG();

  switch (this->current_state_) {
    case READ_STATE_BREAK:
      // Check if there was an unanswered message before break.
      if (this->current_PID_with_parity_ != 0x00 && this->current_PID_ != 0x00 && this->current_data_valid) {
        if (this->current_data_count_ < 8) {
          log_msg.current_PID = this->current_PID_;
          if (this->current_PID_order_answered_) {
            // ESP answered — master sent no data back (normal for slave-response frames)
            log_msg.type = QUEUE_LOG_MSG_TYPE::ERROR_READ_LIN_FRAME_UNABLE_TO_ANSWER;
            TRUMA_LOGV_ISR(log_msg);
          } else if (this->current_PID_ == DIAGNOSTIC_FRAME_SLAVE) {
            // 0x3D not answered (queue empty) — reading passively, this is normal
            log_msg.type = QUEUE_LOG_MSG_TYPE::ERROR_READ_LIN_FRAME_LOST_MSG;
            TRUMA_LOGV_ISR(log_msg);
          } else {
            log_msg.type = QUEUE_LOG_MSG_TYPE::ERROR_READ_LIN_FRAME_LOST_MSG;
            for (uint8_t i = 0; i < this->current_data_count_; i++) {
              log_msg.data[i] = this->current_data_[i];
            }
            log_msg.len = this->current_data_count_;
            TRUMA_LOGE_ISR(log_msg);
          }
        }
      }

      this->current_state_reset_();

      if (!this->read_byte(&buf) || (buf != LIN_BREAK && buf != LIN_SYNC)) {
        log_msg.type = QUEUE_LOG_MSG_TYPE::VV_READ_LIN_FRAME_BREAK_EXPECTED;
        log_msg.current_PID = buf;
        TRUMA_LOGVV_ISR(log_msg);
      } else {
        if (buf == LIN_BREAK) {
          this->current_state_ = READ_STATE_SYNC;
        } else if (buf == LIN_SYNC) {
          this->current_state_ = READ_STATE_SID;
        }
      }
      break;

    case READ_STATE_SYNC:
      if (!this->read_byte(&buf) || buf != LIN_SYNC) {
        log_msg.type = QUEUE_LOG_MSG_TYPE::VV_READ_LIN_FRAME_SYNC_EXPECTED;
        log_msg.current_PID = buf;
        TRUMA_LOGVV_ISR(log_msg);
        this->current_state_ = buf == LIN_BREAK ? READ_STATE_SYNC : READ_STATE_BREAK;
      } else {
        this->current_state_ = READ_STATE_SID;
      }
      break;

    case READ_STATE_SID:
      this->read_byte(&(this->current_PID_with_parity_));
      this->current_PID_ = this->current_PID_with_parity_ & 0x3F;
      if (this->lin_checksum_ == LIN_CHECKSUM::LIN_CHECKSUM_VERSION_2) {
        if (this->current_PID_with_parity_ != (this->current_PID_ | (addr_parity(this->current_PID_) << 6))) {
          log_msg.type = QUEUE_LOG_MSG_TYPE::WARN_READ_LIN_FRAME_SID_CRC;
          log_msg.current_PID = this->current_PID_with_parity_;
          TRUMA_LOGW_ISR(log_msg);
          this->current_data_valid = false;
        }
      }

      if (this->current_data_valid) {
        this->can_write_lin_answer_ = true;
        this->answer_lin_order_(this->current_PID_);
        this->can_write_lin_answer_ = false;
      }

      this->last_data_recieved_ = micros();  // Timeout ab jetzt für erste Datenbyte
      this->current_state_ = READ_STATE_DATA;
      break;

    case READ_STATE_DATA: {
      auto current = micros();
      if (current > (this->last_data_recieved_ + this->time_per_first_byte_)) {
        this->current_state_ = READ_STATE_BREAK;
        return;
      }
      if (!this->available()) {
        esp_rom_delay_us(200);
        return;
      }
      this->read_byte(&buf);
      this->current_data_[this->current_data_count_++] = buf;
      this->last_data_recieved_ = micros();

      if (this->current_data_count_ >= sizeof(this->current_data_)) {
        this->current_state_ = READ_STATE_ACT;
      }
      break;
    }

    default:
      break;
  }

  if (this->current_state_ == READ_STATE_ACT && this->current_data_count_ > 1) {
    uint8_t data_length = this->current_data_count_ - 1;
    uint8_t data_CRC = this->current_data_[this->current_data_count_ - 1];
    bool message_source_know = false;
    bool message_from_master = true;
    // Only validate CRC for complete frames (9 bytes = 8 data + 1 CRC)
    bool full_frame = (this->current_data_count_ == sizeof(this->current_data_));

    if (this->lin_checksum_ == LIN_CHECKSUM::LIN_CHECKSUM_VERSION_1 ||
        (this->current_PID_ == DIAGNOSTIC_FRAME_MASTER || this->current_PID_ == DIAGNOSTIC_FRAME_SLAVE)) {
      if (full_frame && data_CRC != data_checksum(this->current_data_, data_length, 0)) {
        log_msg.type = QUEUE_LOG_MSG_TYPE::WARN_READ_LIN_FRAME_LINv1_CRC;
        TRUMA_LOGW_ISR(log_msg);
        this->current_data_valid = false;
      }
      if (this->current_PID_ == DIAGNOSTIC_FRAME_MASTER) {
        message_source_know = true;
        message_from_master = true;
      } else if (this->current_PID_ == DIAGNOSTIC_FRAME_SLAVE) {
        message_source_know = true;
        message_from_master = false;
      }
    } else {
      if (full_frame) {
        uint8_t data_CRC_master = data_checksum(this->current_data_, data_length, this->current_PID_);
        uint8_t data_CRC_slave = data_checksum(this->current_data_, data_length, this->current_PID_with_parity_);
        if (data_CRC != data_CRC_master && data_CRC != data_CRC_slave) {
          log_msg.type = QUEUE_LOG_MSG_TYPE::WARN_READ_LIN_FRAME_LINv2_CRC;
          TRUMA_LOGW_ISR(log_msg);
          this->current_data_valid = false;
        }
        if (data_CRC == data_CRC_slave) {
          message_from_master = false;
        }
      }
      message_source_know = true;
    }

#ifdef ESPHOME_LOG_HAS_VERBOSE
    log_msg.type = QUEUE_LOG_MSG_TYPE::VERBOSE_READ_LIN_FRAME_MSG;
    log_msg.current_PID = this->current_PID_;
    for (uint8_t i = 0; i < this->current_data_count_; i++) {
      log_msg.data[i] = this->current_data_[i];
    }
    log_msg.len = this->current_data_count_;
    log_msg.current_data_valid = this->current_data_valid;
    log_msg.message_source_know = message_source_know;
    log_msg.message_from_master = message_from_master;
    TRUMA_LOGV_ISR(log_msg);
#endif

    if (this->current_data_valid && message_from_master) {
      QUEUE_LIN_MSG lin_msg;
      lin_msg.current_PID = this->current_PID_;
      lin_msg.len = this->current_data_count_ - 1;
      for (uint8_t i = 0; i < lin_msg.len; i++) {
        lin_msg.data[i] = this->current_data_[i];
      }
      xQueueSend(this->lin_msg_queue_, (void *) &lin_msg, QUEUE_WAIT_DONT_BLOCK);
    }
    this->current_state_ = READ_STATE_BREAK;
  }
}

void LinBusListener::clear_uart_buffer_() {
  uint8_t buffer;
  while (this->available() && this->read_byte(&buffer)) {
  }
}

void LinBusListener::process_lin_msg_queue(TickType_t xTicksToWait) {
  QUEUE_LIN_MSG lin_msg;
  while (xQueueReceive(this->lin_msg_queue_, &lin_msg, xTicksToWait) == pdPASS) {
    this->lin_message_recieved_(lin_msg.current_PID, lin_msg.data, lin_msg.len);
  }
}

void LinBusListener::process_log_queue(TickType_t xTicksToWait) {
#if ESPHOME_LOG_LEVEL > ESPHOME_LOG_LEVEL_NONE
  QUEUE_LOG_MSG log_msg;
  while (xQueueReceive(this->log_queue_, &log_msg, xTicksToWait) == pdPASS) {
    auto current_PID = log_msg.current_PID;
    switch (log_msg.type) {
      case QUEUE_LOG_MSG_TYPE::ERROR_LIN_ANSWER_CAN_WRITE_LIN_ANSWER:
        ESP_LOGE(TAG, "Cannot answer LIN because there is no open order from master.");
        break;
      case QUEUE_LOG_MSG_TYPE::ERROR_LIN_ANSWER_TOO_LONG:
        ESP_LOGE(TAG, "LIN answer cannot be longer than 8 bytes.");
        break;
      case QUEUE_LOG_MSG_TYPE::VERBOSE_LIN_ANSWER_RESPONSE:
        if (!this->observer_mode_) {
          ESP_LOGV(TAG, "RESPONSE %02X %s", current_PID, format_hex_pretty(log_msg.data, log_msg.len).c_str());
        } else {
          ESP_LOGV(TAG, "RESPONSE %02X %s - NOT SEND (OBSERVER MODE)", current_PID,
                   format_hex_pretty(log_msg.data, log_msg.len).c_str());
        }
        break;
      case QUEUE_LOG_MSG_TYPE::ERROR_CHECK_FOR_LIN_FAULT_DETECTED:
        ESP_LOGE(TAG, "Fault on LIN BUS detected.");
        break;
      case QUEUE_LOG_MSG_TYPE::INFO_CHECK_FOR_LIN_FAULT_FIXED:
        ESP_LOGI(TAG, "Fault on LIN BUS fixed.");
        break;
      case QUEUE_LOG_MSG_TYPE::ERROR_READ_LIN_FRAME_UNABLE_TO_ANSWER:
        // Downgraded to VERBOSE: ESP answered this PID correctly,
        // master sent no data back — normal for slave-response frames (0x18, 0x3D).
        ESP_LOGV(TAG, "PID %02X      order - answered, no master data (normal)", current_PID);
        break;
      case QUEUE_LOG_MSG_TYPE::ERROR_READ_LIN_FRAME_LOST_MSG:
        if (log_msg.len == 0) {
          ESP_LOGV(TAG, "PID %02X      order no answer", current_PID);
        } else if (log_msg.len < 8) {
          ESP_LOGW(TAG, "PID %02X      %s partial data received", current_PID,
                   format_hex_pretty(log_msg.data, log_msg.len).c_str());
        }
        break;
      case QUEUE_LOG_MSG_TYPE::VV_READ_LIN_FRAME_BREAK_EXPECTED:
        ESP_LOGVV(TAG, "0x%02X Expected BREAK not received.", current_PID);
        break;
      case QUEUE_LOG_MSG_TYPE::VV_READ_LIN_FRAME_SYNC_EXPECTED:
        ESP_LOGVV(TAG, "0x%02X Expected SYNC not found.", current_PID);
        break;
      case QUEUE_LOG_MSG_TYPE::WARN_READ_LIN_FRAME_SID_CRC:
        ESP_LOGW(TAG, "0x%02X LIN CRC error on SID.", current_PID);
        break;
      case QUEUE_LOG_MSG_TYPE::WARN_READ_LIN_FRAME_LINv1_CRC:
        ESP_LOGW(TAG, "LIN v1 CRC error");
        break;
      case QUEUE_LOG_MSG_TYPE::WARN_READ_LIN_FRAME_LINv2_CRC:
        ESP_LOGW(TAG, "LIN v2 CRC error");
        break;
      case QUEUE_LOG_MSG_TYPE::VERBOSE_READ_LIN_FRAME_MSG:
        if (current_PID == 0x20 || current_PID == 0x21 || current_PID == 0x22 ||
            ((current_PID == DIAGNOSTIC_FRAME_MASTER || current_PID == DIAGNOSTIC_FRAME_SLAVE) &&
             log_msg.data[0] == 0x01)) {
          ESP_LOGVV(TAG, "PID %02X      %s %s %s", current_PID, format_hex_pretty(log_msg.data, log_msg.len).c_str(),
                    log_msg.message_source_know ? (log_msg.message_from_master ? " - MASTER" : " - SLAVE") : "",
                    log_msg.current_data_valid ? "" : "INVALID");
        } else {
          ESP_LOGV(TAG, "PID %02X      %s %s %s", current_PID, format_hex_pretty(log_msg.data, log_msg.len).c_str(),
                   log_msg.message_source_know ? (log_msg.message_from_master ? " - MASTER" : " - SLAVE") : "",
                   log_msg.current_data_valid ? "" : "INVALID");
        }
        break;
      default:
        break;
    }
  }
#endif
}

#undef LIN_BREAK
#undef LIN_SYNC
#undef DIAGNOSTIC_FRAME_MASTER
#undef DIAGNOSTIC_FRAME_SLAVE
#undef QUEUE_WAIT_DONT_BLOCK

}  // namespace truma_inetbox
}  // namespace esphome

#include "TrumaiNetBoxApp.h"
#include "TrumaStatusFrameBuilder.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "helpers.h"

namespace esphome {
namespace truma_inetbox {

static const char *const TAG = "truma_inetbox.TrumaiNetBoxApp";

TrumaiNetBoxApp::TrumaiNetBoxApp() {
  this->airconAuto_.set_parent(this);
  this->airconManual_.set_parent(this);
  this->clock_.set_parent(this);
  // this->config_.set_parent(this);
  this->heater_.set_parent(this);
  this->timer_.set_parent(this);
}

void TrumaiNetBoxApp::update() {
  // Call listeners in after method 'lin_multiframe_recieved' call.
  // Because 'lin_multiframe_recieved' is time critical an all these sensors can take some time.

  // Run through callbacks
  this->airconAuto_.update();
  this->airconManual_.update();
  this->clock_.update();
  this->config_.update();
  this->heater_.update();
  this->timer_.update();

  LinBusProtocol::update();

#ifdef USE_TIME
  // Update time of CP Plus automatically when
  // - Time component configured
  // - Update was not done
  // - 30 seconds after init data recieved
  if (this->time_ != nullptr && !this->update_status_clock_done && this->init_recieved_ > 0) {
    if (micros() > ((30 * 1000 * 1000) + this->init_recieved_ /* 30 seconds after init recieved */)) {
      this->update_status_clock_done = true;
      this->clock_.action_write_time();
    }
  }
#endif  // USE_TIME
}

const std::array<uint8_t, 4> TrumaiNetBoxApp::lin_identifier() {
  return {0x17 /*Supplied Id*/, 0x46 /*Supplied Id*/, 0x00 /*Function Id*/, 0x1F /*Function Id*/};
}

void TrumaiNetBoxApp::lin_heartbeat() { this->device_registered_ = micros(); }

void TrumaiNetBoxApp::lin_reset_device() {
  LinBusProtocol::lin_reset_device();
  this->device_registered_ = micros();
  this->init_recieved_ = 0;

  this->airconAuto_.reset();
  this->airconManual_.reset();
  this->clock_.reset();
  this->config_.reset();
  this->heater_.reset();
  this->timer_.reset();

  this->update_time_ = 0;
}

bool TrumaiNetBoxApp::answer_lin_order_(const uint8_t pid) {
  // Alive message
  if (pid == LIN_PID_TRUMA_INET_BOX) {
    std::array<uint8_t, 8> response = this->lin_empty_response_;

    if (this->updates_to_send_.empty() && !this->has_update_to_submit_()) {
      response[0] = 0xFE;
    }
    this->write_lin_answer_(response.data(), (uint8_t) sizeof(response));
    return true;
  }
  return LinBusProtocol::answer_lin_order_(pid);
}

bool TrumaiNetBoxApp::lin_read_field_by_identifier_(uint8_t identifier, std::array<uint8_t, 5> *response) {
  if (identifier == 0x00 /* LIN Product Identification */) {
    auto lin_identifier = this->lin_identifier();
    (*response)[0] = lin_identifier[0];
    (*response)[1] = lin_identifier[1];
    (*response)[2] = lin_identifier[2];
    (*response)[3] = lin_identifier[3];
    (*response)[4] = 0x01;  // Variant
    return true;
  } else if (identifier == 0x20 /* Product details to display in CP plus */) {
    auto lin_identifier = this->lin_identifier();
    (*response)[0] = lin_identifier[0];
    (*response)[1] = lin_identifier[1];
    (*response)[2] = lin_identifier[2];
    return true;
  } else if (identifier == 0x22 /* unknown usage */) {
    return true;
  }
  return false;
}

const uint8_t *TrumaiNetBoxApp::lin_multiframe_recieved(const uint8_t *message, const uint8_t message_len,
                                                         uint8_t *return_len) {
  static uint8_t response[48] = {};
  if (message_len < truma_message_header.size()) {
    return nullptr;
  }
  for (uint8_t i = 1; i < truma_message_header.size() - 3; i++) {
    if (message[i] != truma_message_header[i] && message[i] != alde_message_header[i]) {
      return nullptr;
    }
  }
  if (message[4] != (uint8_t) this->company_) {
    ESP_LOGI(TAG, "Switch company to 0x%02x", message[4]);
    this->company_ = (TRUMA_COMPANY) message[4];
  }

  if (message[0] == LIN_SID_READ_STATE_BUFFER) {
    memset(response, 0, sizeof(response));
    auto response_frame = reinterpret_cast<StatusFrame *>(response);

    if (this->init_recieved_ == 0) {
      status_frame_create_init(response_frame, return_len, this->message_counter++);
      return response;
    } else if (this->heater_.has_update()) {
      this->heater_.create_update_data(response_frame, return_len, this->message_counter++);
      this->update_time_ = 0;
      return response;
    } else if (this->timer_.has_update()) {
      this->timer_.create_update_data(response_frame, return_len, this->message_counter++);
      this->update_time_ = 0;
      return response;
    } else if (this->airconManual_.has_update()) {
      this->airconManual_.create_update_data(response_frame, return_len, this->message_counter++);
      this->update_time_ = 0;
      return response;
    } else if (this->airconAuto_.has_update()) {
      this->airconAuto_.create_update_data(response_frame, return_len, this->message_counter++);
      this->update_time_ = 0;
      return response;
#ifdef USE_TIME
    } else if (this->clock_.has_update()) {
      this->clock_.create_update_data(response_frame, return_len, this->message_counter++);
      this->update_time_ = 0;
      return response;
#endif  // USE_TIME
    }
  }

  if (message_len < sizeof(StatusFrame) && message[0] == LIN_SID_FIll_STATE_BUFFFER) {
    return nullptr;
  }

  auto statusFrame = reinterpret_cast<const StatusFrame *>(message);
  auto header = &statusFrame->genericHeader;
  
  if (header->checksum != data_checksum(&statusFrame->raw[10], sizeof(StatusFrame) - 10, (0xFF - header->checksum)) ||
      header->header_2 != 'T' || header->header_3 != 0x01) {
    ESP_LOGE(TAG, "Truma checksum fail.");
    return nullptr;
  }

  response[0] = (header->service_identifier | LIN_SID_RESPONSE);
  (*return_len) = 1;

  if (header->message_type == STATUS_FRAME_HEATER && header->message_length == sizeof(StatusFrameHeater)) {
    this->heater_.set_status(statusFrame->heater);
    return response;
  } else if (header->message_type == STATUS_FRAME_AIRCON_MANUAL &&
             header->message_length == sizeof(StatusFrameAirconManual)) {
    this->airconManual_.set_status(statusFrame->airconManual);
    return response;
  } else if (header->message_type == STATUS_FRAME_AIRCON_MANUAL_INIT &&
             header->message_length == sizeof(StatusFrameAirconManualInit)) {
    return response;
  } else if (header->message_type == STATUS_FRAME_AIRCON_AUTO &&
             header->message_length == sizeof(StatusFrameAirconAuto)) {
    this->airconAuto_.set_status(statusFrame->airconAuto);
    return response;
  } else if (header->message_type == STATUS_FRAME_AIRCON_AUTO_INIT &&
             header->message_length == sizeof(StatusFrameAirconAutoInit)) {
    return response;
  } else if (header->message_type == STATUS_FRAME_TIMER && header->message_length == sizeof(StatusFrameTimer)) {
    this->timer_.set_status(statusFrame->timer);
    return response;
  } else if (header->message_type == STATUS_FRAME_CLOCK && header->message_length == sizeof(StatusFrameClock)) {
    this->clock_.set_status(statusFrame->clock);
    return response;
  } else if (header->message_type == STAUTS_FRAME_CONFIG && header->message_length == sizeof(StatusFrameConfig)) {
    this->config_.set_status(statusFrame->config);
    return response;
  } else if (header->message_type == STATUS_FRAME_RESPONSE_ACK &&
             header->message_length == sizeof(StatusFrameResponseAck)) {
    auto data = statusFrame->responseAck;
    if (data.error_code != ResponseAckResult::RESPONSE_ACK_RESULT_OKAY) {
      this->lin_reset_device();
    }
    return response;
  } else if (header->message_type == STATUS_FRAME_DEVICES && header->message_length == sizeof(StatusFrameDevice)) {
    auto device = statusFrame->device;
    ESP_LOGD(TAG, "Device detected: ID %d", device.device_id);

    const auto truma_device = static_cast<TRUMA_DEVICE>(device.software_revision[0]);
    const auto is_CPPLUSDevice = device.device_id == 0;

    if (!is_CPPLUSDevice) {
      if (device.device_id == 1) {
        this->heater_device_ = truma_device;
      }
      if (device.device_id == 2) {
        this->aircon_device_ = TRUMA_DEVICE::AIRCON_DEVICE;
      }
    }

    if (device.device_count == 2 && this->heater_device_ != TRUMA_DEVICE::UNKNOWN) {
      this->init_recieved_ = micros();
    } else if (device.device_count == 3 && this->heater_device_ != TRUMA_DEVICE::UNKNOWN &&
               this->aircon_device_ != TRUMA_DEVICE::UNKNOWN) {
      this->init_recieved_ = micros();
    }
    return response;
  }
  (*return_len) = 0;
  return nullptr;
}

bool TrumaiNetBoxApp::has_update_to_submit_() {
  if (this->init_requested_ == 0) {
    this->init_requested_ = micros();
    return true;
  } else if (this->init_recieved_ == 0) {
    auto init_wait_time = micros() - this->init_requested_;
    if (init_wait_time > 1000 * 1000 * 5) {
      this->init_requested_ = micros();
      return true;
    }
  } else if (this->airconAuto_.has_update() || this->airconManual_.has_update() || this->clock_.has_update() ||
             this->heater_.has_update() || this->timer_.has_update()) {
    if (this->update_time_ == 0) {
      this->update_time_ = micros();
      return true;
    }
    auto update_wait_time = micros() - this->update_time_;
    if (update_wait_time > 1000 * 1000 * 5) {
      this->update_time_ = micros();
      return true;
    }
  }
  return false;
}

}  // namespace truma_inetbox
}  // namespace esphome
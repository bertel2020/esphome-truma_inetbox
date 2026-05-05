#include "TrumaTextSensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace truma_inetbox {

static const char *const TAG = "truma_inetbox.text_sensor";

// CP Plus gilt als offline wenn seit mehr als 10 Sekunden kein Heartbeat empfangen
static const uint32_t CP_PLUS_TIMEOUT_US = 10 * 1000 * 1000;

void TrumaTextSensor::setup() {
  // Callback für Uhrzeit
  this->parent_->get_clock()->add_on_message_callback([this](const StatusFrameClock *c) {
    if (this->type_ != TRUMA_TEXT_SENSOR_TYPE::CLOCK) return;
    char buf[6];
    snprintf(buf, sizeof(buf), "%02d:%02d", c->clock_hour, c->clock_minute);
    this->publish_state(buf);
  });

  // UPDATE_STATUS und CP_PLUS_STATUS werden in update_status() gepollt
  // da sie auf internem App-State basieren, nicht auf Frame-Callbacks
}

void TrumaTextSensor::update_status() {
  switch (this->type_) {
    case TRUMA_TEXT_SENSOR_TYPE::UPDATE_STATUS: {
      // Entspricht danielfett/inetbox.py TRANSLATIONS_STATES update_status
      std::string status;
      if (this->parent_->get_last_cp_plus_request() == 0) {
        // Noch kein Heartbeat empfangen → kein Init möglich
        status = "Warte auf CP Plus";
      } else if (!this->parent_->get_heater()->get_status_valid()) {
        // Init empfangen aber noch kein Heater-Status
        status = "Warte auf CP Plus";
      } else if (this->parent_->has_pending_updates()) {
        // Update in Queue, wird gesendet
        status = "Warte auf Versand";
      } else {
        status = "Leerlauf";
      }
      if (!this->has_state() || this->state != status) {
        this->publish_state(status);
      }
      break;
    }

    case TRUMA_TEXT_SENSOR_TYPE::CP_PLUS_STATUS: {
      // Entspricht danielfett/inetbox.py cp_plus_status
      std::string status;
      uint32_t last = (uint32_t) this->parent_->get_last_cp_plus_request();
      if (last == 0) {
        status = "warte...";
      } else {
        uint32_t elapsed = micros() - last;
        status = (elapsed < CP_PLUS_TIMEOUT_US) ? "online" : "warte...";
      }
      if (!this->has_state() || this->state != status) {
        this->publish_state(status);
      }
      break;
    }

    default:
      break;
  }
}

void TrumaTextSensor::dump_config() {
  LOG_TEXT_SENSOR("", "Truma Text Sensor", this);
  ESP_LOGCONFIG(TAG, "  Type '%s'", enum_to_c_str(this->type_));
}

}  // namespace truma_inetbox
}  // namespace esphome

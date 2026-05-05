#include "TrumaTextSensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace truma_inetbox {

static const char *const TAG = "truma_inetbox.text_sensor";

void TrumaTextSensor::setup() {
  // Callback für Uhrzeit
  this->parent_->get_clock()->add_on_message_callback([this](const StatusFrameClock *c) {
    if (this->type_ != TRUMA_TEXT_SENSOR_TYPE::CLOCK) return;
    char buf[6];
    snprintf(buf, sizeof(buf), "%02d:%02d", c->clock_hour, c->clock_minute);
    this->publish_state(buf);
  });

  // UPDATE_STATUS und CP_PLUS_STATUS werden via Callback bei jedem update() gepollt
  if (this->type_ == TRUMA_TEXT_SENSOR_TYPE::UPDATE_STATUS ||
      this->type_ == TRUMA_TEXT_SENSOR_TYPE::CP_PLUS_STATUS) {
    this->parent_->add_on_update_callback([this]() { this->update_status(); });
  }
}

void TrumaTextSensor::update_status() {
  switch (this->type_) {
    case TRUMA_TEXT_SENSOR_TYPE::UPDATE_STATUS: {
      // Entspricht danielfett/inetbox.py send_update_status()
      std::string status;
      if (!this->parent_->get_heater()->get_status_valid()) {
        // Noch kein Status vom CP Plus empfangen
        status = "Warte auf CP Plus";
      } else if (this->parent_->has_pending_updates()) {
        // Update in Queue, wird an Truma gesendet
        status = "Warte auf Truma";
      } else {
        status = "Leerlauf";
      }
      if (!this->has_state() || this->state != status) {
        this->publish_state(status);
      }
      break;
    }

    case TRUMA_TEXT_SENSOR_TYPE::CP_PLUS_STATUS: {
      // Entspricht danielfett/inetbox.py send_cp_plus_status()
      // can_send_updates = erster Heater-Status-Frame empfangen
      std::string status = this->parent_->get_heater()->get_status_valid() ? "online" : "warte...";
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

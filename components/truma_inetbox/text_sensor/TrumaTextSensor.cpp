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

  // Display-Status Text-Sensoren aus PID 0x22
  if (this->type_ == TRUMA_TEXT_SENSOR_TYPE::CP_PLUS_DISPLAY_STATUS ||
      this->type_ == TRUMA_TEXT_SENSOR_TYPE::HEATING_STATUS ||
      this->type_ == TRUMA_TEXT_SENSOR_TYPE::HEATING_STATUS_2) {
    this->parent_->get_display()->add_on_message_callback([this](const StatusFrameDisplay *d) {
      switch (this->type_) {
        case TRUMA_TEXT_SENSOR_TYPE::CP_PLUS_DISPLAY_STATUS: {
          const char *s = "Unknown";
          switch (d->cp_plus_display) {
            case 0x00: s = "Standby - AC Off";  break;
            case 0x01: s = "Warning";           break;
            case 0x20: s = "Standby - AC On";   break;
            case 0x50: s = "Boiler On";         break;
            case 0x40: s = "Boiler Off";        break;
            case 0xF0: s = "Heating On";        break;
            case 0xD0: s = "Error";             break;
            case 0x70: s = "Fatal Error";       break;
          }
          this->publish_state(s);
          break;
        }
        case TRUMA_TEXT_SENSOR_TYPE::HEATING_STATUS: {
          const char *s = "Unknown";
          switch (d->heating_status) {
            case 0x10: s = "Boiler ECO Done";    break;
            case 0x11: s = "Boiler ECO Heating"; break;
            case 0x30: s = "Boiler HOT Done";    break;
            case 0x31: s = "Boiler HOT Heating"; break;
          }
          this->publish_state(s);
          break;
        }
        case TRUMA_TEXT_SENSOR_TYPE::HEATING_STATUS_2: {
          const char *s = "Unknown";
          switch (d->heating_status_2) {
            case 0x04: s = "Normal";       break;
            case 0x05: s = "Error";        break;
            case 0xFF: s = "Fatal Error";  break;
            case 0xFE: s = "Normal (?)";   break;
          }
          this->publish_state(s);
          break;
        }
        default: break;
      }
    });
  }

  // OperatingStatus aus Heater-Frame
  if (this->type_ == TRUMA_TEXT_SENSOR_TYPE::OPERATING_STATUS) {
    this->parent_->get_heater()->add_on_message_callback([this](const StatusFrameHeater *h) {
      const char *s = "Unknown";
      switch (h->operating_status) {
        case OperatingStatus::OPERATING_STATUS_OFF:                s = "Off";               break;
        case OperatingStatus::OPERATING_STATUS_WARNING:            s = "Warning";           break;
        case OperatingStatus::OPERATING_STATUS_START_OR_COOL_DOWN: s = "Start / Cool Down"; break;
        case OperatingStatus::OPERATING_STATUS_ON_5:               s = "On";                break;
        case OperatingStatus::OPERATING_STATUS_ON_6:               s = "On";                break;
        case OperatingStatus::OPERATING_STATUS_ON_7:               s = "On";                break;
        case OperatingStatus::OPERATING_STATUS_ON_8:               s = "On";                break;
        case OperatingStatus::OPERATING_STATUS_ON_9:               s = "On";                break;
      }
      this->publish_state(s);
    });
  }
}

void TrumaTextSensor::update_status() {
  switch (this->type_) {
    case TRUMA_TEXT_SENSOR_TYPE::UPDATE_STATUS: {
      std::string status;
      if (!this->parent_->get_heater()->get_status_valid()) {
        status = "Warte auf CP Plus";
      } else if (this->parent_->has_pending_updates()) {
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

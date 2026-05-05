#include "TrumaSensor.h"
#include "esphome/core/log.h"
#include "esphome/components/truma_inetbox/helpers.h"

namespace esphome {
namespace truma_inetbox {

static const char *const TAG = "truma_inetbox.sensor";

void TrumaSensor::setup() {
  this->parent_->get_heater()->add_on_message_callback([this](const StatusFrameHeater *h) {
    switch (this->type_) {
      case TRUMA_SENSOR_TYPE::CURRENT_ROOM_TEMPERATURE:
        this->publish_state(temp_code_to_decimal(h->current_temp_room)); break;
      case TRUMA_SENSOR_TYPE::CURRENT_WATER_TEMPERATURE:
        this->publish_state(temp_code_to_decimal(h->current_temp_water)); break;
      case TRUMA_SENSOR_TYPE::TARGET_ROOM_TEMPERATURE:
        this->publish_state(temp_code_to_decimal(h->target_temp_room)); break;
      case TRUMA_SENSOR_TYPE::TARGET_WATER_TEMPERATURE:
        this->publish_state(static_cast<float>(h->target_temp_water)); break;
      case TRUMA_SENSOR_TYPE::HEATING_MODE:
        this->publish_state(static_cast<float>(h->heating_mode)); break;
      case TRUMA_SENSOR_TYPE::ELECTRIC_POWER_LEVEL:
        this->publish_state(static_cast<float>(h->el_power_level_a)); break;
      case TRUMA_SENSOR_TYPE::ENERGY_MIX:
        this->publish_state(static_cast<float>(h->energy_mix_a)); break;
      case TRUMA_SENSOR_TYPE::OPERATING_STATUS:
        this->publish_state(static_cast<float>(h->operating_status)); break;
      case TRUMA_SENSOR_TYPE::HEATER_ERROR_CODE:
        // 0xFF = no error (initial/unset value), 0x00 = no error
        if (h->error_code_low == 0xFF && h->error_code_high == 0x00) {
          this->publish_state(0.0f);
        } else {
          this->publish_state(h->error_code_high * 100.0f + h->error_code_low);
        }
        break;
      default: break;
    }
  });

  this->parent_->get_timer()->add_on_message_callback([this](const StatusFrameTimer *t) {
    switch (this->type_) {
      case TRUMA_SENSOR_TYPE::TIMER_START_TIME:
        this->publish_state(static_cast<float>((t->timer_start_hours & 0x1F) * 100 + (t->timer_start_minutes & 0x3F))); break;
      case TRUMA_SENSOR_TYPE::TIMER_STOP_TIME:
        this->publish_state(static_cast<float>((t->timer_stop_hours & 0x1F) * 100 + (t->timer_stop_minutes & 0x3F))); break;
      case TRUMA_SENSOR_TYPE::TIMER_ROOM_TEMPERATURE:
        this->publish_state(static_cast<uint8_t>(t->timer_target_temp_room) == 0 ? 0.0f : temp_code_to_decimal(t->timer_target_temp_room)); break;
      case TRUMA_SENSOR_TYPE::TIMER_WATER_TEMPERATURE:
        this->publish_state(static_cast<uint8_t>(t->timer_target_temp_water) == 0 ? 0.0f : temp_code_to_decimal(t->timer_target_temp_water)); break;
      default: break;
    }
  });

  // Callback für Clock-Daten
  this->parent_->get_clock()->add_on_message_callback([this](const StatusFrameClock *c) {
    switch (this->type_) {
      case TRUMA_SENSOR_TYPE::CLOCK_HOUR:
        this->publish_state(static_cast<float>(c->clock_hour)); break;
      case TRUMA_SENSOR_TYPE::CLOCK_MINUTE:
        this->publish_state(static_cast<float>(c->clock_minute)); break;
      default: break;
    }
  });

  // CP_PLUS_DISPLAY_STATUS und HEATING_STATUS aus PID 0x22
  // Quelle: danielfett/inetbox.py parse_status_2
  this->parent_->get_display()->add_on_message_callback([this](const StatusFrameDisplay *d) {
    switch (this->type_) {
      case TRUMA_SENSOR_TYPE::CP_PLUS_DISPLAY_STATUS:
        this->publish_state(static_cast<float>(d->cp_plus_display)); break;
      case TRUMA_SENSOR_TYPE::HEATING_STATUS:
        this->publish_state(static_cast<float>(d->heating_status)); break;
      default: break;
    }
  });
}

void TrumaSensor::dump_config() {
  LOG_SENSOR("", "Truma Sensor", this);
  ESP_LOGCONFIG(TAG, "  Type '%s'", enum_to_c_str(this->type_));
}

}  // namespace truma_inetbox
}  // namespace esphome

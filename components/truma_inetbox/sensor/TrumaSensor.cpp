#include "TrumaSensor.h"
#include "esphome/core/log.h"
#include "esphome/components/truma_inetbox/helpers.h"

namespace esphome {
namespace truma_inetbox {

static const char *const TAG = "truma_inetbox.sensor";

void TrumaSensor::setup() {
  // Callback für Heater-Daten
  this->parent_->get_heater()->add_on_message_callback([this](const StatusFrameHeater *status_heater) {
    switch (this->type_) {
      case TRUMA_SENSOR_TYPE::CURRENT_ROOM_TEMPERATURE:
        this->publish_state(temp_code_to_decimal(status_heater->current_temp_room));
        break;
      case TRUMA_SENSOR_TYPE::CURRENT_WATER_TEMPERATURE:
        this->publish_state(temp_code_to_decimal(status_heater->current_temp_water));
        break;
      case TRUMA_SENSOR_TYPE::TARGET_ROOM_TEMPERATURE:
        this->publish_state(temp_code_to_decimal(status_heater->target_temp_room));
        break;
      case TRUMA_SENSOR_TYPE::TARGET_WATER_TEMPERATURE:
        this->publish_state(static_cast<float>(status_heater->target_temp_water));
        break;
      case TRUMA_SENSOR_TYPE::HEATING_MODE:
        this->publish_state(static_cast<float>(status_heater->heating_mode));
        break;
      case TRUMA_SENSOR_TYPE::ELECTRIC_POWER_LEVEL:
        this->publish_state(static_cast<float>(status_heater->el_power_level_a));
        break;
      case TRUMA_SENSOR_TYPE::ENERGY_MIX:
        this->publish_state(static_cast<float>(status_heater->energy_mix_a));
        break;
      case TRUMA_SENSOR_TYPE::OPERATING_STATUS:
        this->publish_state(static_cast<float>(status_heater->operating_status));
        break;
      case TRUMA_SENSOR_TYPE::HEATER_ERROR_CODE: {
        float errorcode = status_heater->error_code_high * 100.0f + status_heater->error_code_low;
        this->publish_state(errorcode);
        break;
      }
      default: break;
    }
  });

  this->parent_->get_timer()->add_on_message_callback([this](const StatusFrameTimer *status_timer) {
    switch (this->type_) {
      case TRUMA_SENSOR_TYPE::TIMER_START_TIME: {
        uint8_t h = status_timer->timer_start_hours & 0x1F;
        uint8_t m = status_timer->timer_start_minutes & 0x3F;
        this->publish_state(static_cast<float>(h * 100 + m));
        break;
      }
      case TRUMA_SENSOR_TYPE::TIMER_STOP_TIME: {
        uint8_t h = status_timer->timer_stop_hours & 0x1F;
        uint8_t m = status_timer->timer_stop_minutes & 0x3F;
        this->publish_state(static_cast<float>(h * 100 + m));
        break;
      }
      case TRUMA_SENSOR_TYPE::TIMER_ROOM_TEMPERATURE:
        if (static_cast<uint8_t>(status_timer->timer_target_temp_room) == 0) {
           this->publish_state(0.0f);
        } else {
           this->publish_state(temp_code_to_decimal(status_timer->timer_target_temp_room));
        }
        break;
      case TRUMA_SENSOR_TYPE::TIMER_WATER_TEMPERATURE:
        if (static_cast<uint8_t>(status_timer->timer_target_temp_water) == 0) {
           this->publish_state(0.0f);
        } else {
           this->publish_state(temp_code_to_decimal(status_timer->timer_target_temp_water));
        }
        break;
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
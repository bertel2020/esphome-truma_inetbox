#include "TrumaRoomClimate.h"
#include "esphome/components/truma_inetbox/helpers.h"

namespace esphome {
namespace truma_inetbox {

static const char *const TAG = "truma_inetbox.room_climate";

void TrumaRoomClimate::setup() {
  this->fan_mode = climate::CLIMATE_FAN_OFF;
  this->mode = climate::CLIMATE_MODE_OFF;
  this->publish_state();

  this->parent_->get_heater()->add_on_message_callback([this](const StatusFrameHeater *status_heater) {
    this->target_temperature = temp_code_to_decimal(status_heater->target_temp_room);
    this->current_temperature = temp_code_to_decimal(status_heater->current_temp_room);

    switch (status_heater->heating_mode) {
      case HeatingMode::HEATING_MODE_ECO:   // = VENT_1 = 0x01
        if (status_heater->target_temp_room != TargetTemp::TARGET_TEMP_OFF) {
          this->mode = climate::CLIMATE_MODE_HEAT;
          this->fan_mode = climate::CLIMATE_FAN_LOW;
        } else {
          this->mode = climate::CLIMATE_MODE_FAN_ONLY;
          this->fan_mode = climate::CLIMATE_FAN_OFF;
        }
        break;
      case HeatingMode::HEATING_MODE_VARIO_HEAT_NIGHT:  // VENT_2 = 0x02
      case HeatingMode::HEATING_MODE_VARIO_HEAT_AUTO:   // VENT_3 = 0x03
      case HeatingMode::HEATING_MODE_VENT_4:            // 0x04
      case HeatingMode::HEATING_MODE_VENT_5:            // 0x05
      case HeatingMode::HEATING_MODE_VENT_6:            // 0x06
      case HeatingMode::HEATING_MODE_VENT_7:            // 0x07
      case HeatingMode::HEATING_MODE_VENT_8:            // 0x08
      case HeatingMode::HEATING_MODE_VENT_9:            // 0x09
        this->mode = climate::CLIMATE_MODE_FAN_ONLY;
        this->fan_mode = climate::CLIMATE_FAN_OFF;
        break;
      case HeatingMode::HEATING_MODE_HIGH:  // = VENT_10 = 0x0A
        if (status_heater->target_temp_room != TargetTemp::TARGET_TEMP_OFF) {
          this->mode = climate::CLIMATE_MODE_HEAT;
          this->fan_mode = climate::CLIMATE_FAN_HIGH;
        } else {
          this->mode = climate::CLIMATE_MODE_FAN_ONLY;
          this->fan_mode = climate::CLIMATE_FAN_OFF;
        }
        break;
      default:
        this->mode = climate::CLIMATE_MODE_OFF;
        this->fan_mode = climate::CLIMATE_FAN_OFF;
        break;
    }

    this->publish_state();
  });
}

void TrumaRoomClimate::dump_config() { LOG_CLIMATE(TAG, "Truma Room Climate", this); }

void TrumaRoomClimate::control(const climate::ClimateCall &call) {
  if (call.get_target_temperature().has_value() && !call.get_fan_mode().has_value()) {
    float temp = *call.get_target_temperature();
    this->parent_->get_heater()->action_heater_room(static_cast<uint8_t>(temp));
  }

  if (call.get_mode().has_value()) {
    climate::ClimateMode mode = *call.get_mode();
    auto status_heater = this->parent_->get_heater()->get_status();
    switch (mode) {
      case climate::CLIMATE_MODE_HEAT:
        if (status_heater->target_temp_room == TargetTemp::TARGET_TEMP_OFF) {
          this->parent_->get_heater()->action_heater_room(5, HeatingMode::HEATING_MODE_ECO);
        }
        break;
      case climate::CLIMATE_MODE_FAN_ONLY:
        // Default to vent level 5 — speed can be changed via Fan Only speed sensor
        this->parent_->get_heater()->action_heater_room(0, HeatingMode::HEATING_MODE_VENT_5);
        break;
      default:
        this->parent_->get_heater()->action_heater_room(0);
        break;
    }
  }

  if (call.get_fan_mode().has_value()) {
    auto fan_mode = *call.get_fan_mode();
    auto status_heater = this->parent_->get_heater()->get_status();
    float temp = temp_code_to_decimal(status_heater->target_temp_room, 0);
    if (call.get_target_temperature().has_value()) {
      temp = *call.get_target_temperature();
    }
    switch (fan_mode) {
      case climate::CLIMATE_FAN_LOW:
      case climate::CLIMATE_FAN_HIGH:
        if (temp < 5) temp = 5;
        break;
      default:
        break;
    }
    switch (fan_mode) {
      case climate::CLIMATE_FAN_LOW:
        this->parent_->get_heater()->action_heater_room(static_cast<uint8_t>(temp), HeatingMode::HEATING_MODE_ECO);
        break;
      case climate::CLIMATE_FAN_HIGH:
        this->parent_->get_heater()->action_heater_room(static_cast<uint8_t>(temp), HeatingMode::HEATING_MODE_HIGH);
        break;
      default:
        this->parent_->get_heater()->action_heater_room(0);
        break;
    }
  }
}

climate::ClimateTraits TrumaRoomClimate::traits() {
  auto traits = climate::ClimateTraits();
  traits.add_feature_flags(esphome::climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);

  for (auto mode : this->supported_modes_) {
    traits.add_supported_mode(mode);
  }

  traits.set_supported_fan_modes({{
      climate::CLIMATE_FAN_OFF,
      climate::CLIMATE_FAN_LOW,
      climate::CLIMATE_FAN_HIGH,
  }});

  traits.set_visual_min_temperature(5);
  traits.set_visual_max_temperature(30);
  traits.set_visual_temperature_step(1);
  return traits;
}

void TrumaRoomClimate::set_supported_modes(const std::set<climate::ClimateMode> &modes) {
  this->supported_modes_ = modes;
}

}  // namespace truma_inetbox
}  // namespace esphome

#include "TrumaRoomClimate.h"
#include "esphome/components/truma_inetbox/helpers.h"

namespace esphome {
namespace truma_inetbox {

static const char *const TAG = "truma_inetbox.room_climate";

static const char *const FAN_OFF  = "Off";
static const char *const FAN_ECO  = "Eco";
static const char *const FAN_HIGH = "High";

void TrumaRoomClimate::setup() {
  // Initialize with Off state
  auto call = this->make_call();
  call.set_mode(climate::CLIMATE_MODE_OFF);
  call.set_fan_mode(FAN_OFF);
  call.perform();
  this->publish_state();

  this->parent_->get_heater()->add_on_message_callback([this](const StatusFrameHeater *status_heater) {
    this->target_temperature = temp_code_to_decimal(status_heater->target_temp_room);
    this->current_temperature = temp_code_to_decimal(status_heater->current_temp_room);

    switch (status_heater->heating_mode) {
      case HeatingMode::HEATING_MODE_ECO:   // = VENT_1 = 0x01
        if (status_heater->target_temp_room != TargetTemp::TARGET_TEMP_OFF) {
          this->mode = climate::CLIMATE_MODE_HEAT;
          auto c = this->make_call();
          c.set_fan_mode(FAN_ECO);
          c.perform();
        } else {
          this->mode = climate::CLIMATE_MODE_FAN_ONLY;
        }
        break;
      case HeatingMode::HEATING_MODE_VARIO_HEAT_NIGHT:
      case HeatingMode::HEATING_MODE_VARIO_HEAT_AUTO:
      case HeatingMode::HEATING_MODE_VENT_4:
      case HeatingMode::HEATING_MODE_VENT_5:
      case HeatingMode::HEATING_MODE_VENT_6:
      case HeatingMode::HEATING_MODE_VENT_7:
      case HeatingMode::HEATING_MODE_VENT_8:
      case HeatingMode::HEATING_MODE_VENT_9:
        this->mode = climate::CLIMATE_MODE_FAN_ONLY;
        break;
      case HeatingMode::HEATING_MODE_HIGH:  // = VENT_10 = 0x0A
        if (status_heater->target_temp_room != TargetTemp::TARGET_TEMP_OFF) {
          this->mode = climate::CLIMATE_MODE_HEAT;
          auto c = this->make_call();
          c.set_fan_mode(FAN_HIGH);
          c.perform();
        } else {
          this->mode = climate::CLIMATE_MODE_FAN_ONLY;
        }
        break;
      default:
        this->mode = climate::CLIMATE_MODE_OFF;
        auto c = this->make_call();
        c.set_fan_mode(FAN_OFF);
        c.perform();
        break;
    }

    this->publish_state();
  });
}

void TrumaRoomClimate::dump_config() { LOG_CLIMATE(TAG, "Truma Room Climate", this); }

void TrumaRoomClimate::control(const climate::ClimateCall &call) {
  if (call.get_target_temperature().has_value() && !call.has_custom_fan_mode()) {
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
        this->parent_->get_heater()->action_heater_room(0, HeatingMode::HEATING_MODE_VENT_5);
        break;
      default:
        this->parent_->get_heater()->action_heater_room(0);
        break;
    }
  }

  if (call.has_custom_fan_mode()) {
    const std::string fan = std::string(call.get_custom_fan_mode());
    auto status_heater = this->parent_->get_heater()->get_status();
    float temp = temp_code_to_decimal(status_heater->target_temp_room, 0);
    if (call.get_target_temperature().has_value()) {
      temp = *call.get_target_temperature();
    }
    if (fan == FAN_ECO || fan == FAN_HIGH) {
      if (temp < 5) temp = 5;
    }
    if (fan == FAN_ECO) {
      this->parent_->get_heater()->action_heater_room(static_cast<uint8_t>(temp), HeatingMode::HEATING_MODE_ECO);
    } else if (fan == FAN_HIGH) {
      this->parent_->get_heater()->action_heater_room(static_cast<uint8_t>(temp), HeatingMode::HEATING_MODE_HIGH);
    } else {
      this->parent_->get_heater()->action_heater_room(0);
    }
  }
}

climate::ClimateTraits TrumaRoomClimate::traits() {
  auto traits = climate::ClimateTraits();
  traits.add_feature_flags(esphome::climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);

  for (auto mode : this->supported_modes_) {
    traits.add_supported_mode(mode);
  }

  traits.set_supported_custom_fan_modes({FAN_OFF, FAN_ECO, FAN_HIGH});

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

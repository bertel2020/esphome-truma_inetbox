#include "TrumaHeaterSelect.h"
#include "esphome/core/log.h"
#include "esphome/components/truma_inetbox/helpers.h"

namespace esphome {
namespace truma_inetbox {

static const char *const TAG = "truma_inetbox.heater_select";

void TrumaHeaterSelect::setup() {
  this->parent_->get_heater()->add_on_message_callback([this](const StatusFrameHeater *status) {
    switch (this->type_) {
      case TRUMA_SELECT_TYPE::HEATER_FAN_MODE:
        switch (status->heating_mode) {
          case HeatingMode::HEATING_MODE_ECO:
            this->publish_state(this->at((size_t) TRUMA_SELECT_TYPE_HEATER_FAN_MODE::ECO).value());
            break;
          case HeatingMode::HEATING_MODE_HIGH:
            this->publish_state(this->at((size_t) TRUMA_SELECT_TYPE_HEATER_FAN_MODE::COMBI_HIGH).value());
            break;
          case HeatingMode::HEATING_MODE_BOOST:
            this->publish_state(this->at((size_t) TRUMA_SELECT_TYPE_HEATER_FAN_MODE::BOOST).value());
            break;
          default:
            this->publish_state(this->at((size_t) TRUMA_SELECT_TYPE_HEATER_FAN_MODE::OFF).value());
            break;
        }
        break;

      case TRUMA_SELECT_TYPE::HEATER_ENERGY_MIX:
        switch (status->energy_mix_a) {
          case EnergyMix::ENERGY_MIX_GAS:
            this->publish_state(this->at((size_t) TRUMA_SELECT_TYPE_HEATER_ENERGY_MIX::GAS).value());
            break;
          case EnergyMix::ENERGY_MIX_MIX:
            switch (status->el_power_level_a) {
              case ElectricPowerLevel::ELECTRIC_POWER_LEVEL_900:
                this->publish_state(this->at((size_t) TRUMA_SELECT_TYPE_HEATER_ENERGY_MIX::MIX_1).value());
                break;
              case ElectricPowerLevel::ELECTRIC_POWER_LEVEL_1800:
                this->publish_state(this->at((size_t) TRUMA_SELECT_TYPE_HEATER_ENERGY_MIX::MIX_2).value());
                break;
              default:
                this->publish_state(this->at((size_t) TRUMA_SELECT_TYPE_HEATER_ENERGY_MIX::GAS).value());
                break;
            }
            break;
          case EnergyMix::ENERGY_MIX_ELECTRICITY:
            switch (status->el_power_level_a) {
              case ElectricPowerLevel::ELECTRIC_POWER_LEVEL_900:
                this->publish_state(this->at((size_t) TRUMA_SELECT_TYPE_HEATER_ENERGY_MIX::ELECTRIC_1).value());
                break;
              case ElectricPowerLevel::ELECTRIC_POWER_LEVEL_1800:
                this->publish_state(this->at((size_t) TRUMA_SELECT_TYPE_HEATER_ENERGY_MIX::ELECTRIC_2).value());
                break;
              default:
                this->publish_state(this->at((size_t) TRUMA_SELECT_TYPE_HEATER_ENERGY_MIX::GAS).value());
                break;
            }
            break;
          default:
            this->publish_state(this->at((size_t) TRUMA_SELECT_TYPE_HEATER_ENERGY_MIX::GAS).value());
            break;
        }
        break;

      case TRUMA_SELECT_TYPE::HEATER_FAN_ONLY_SPEED:
        // Only update when in fan only mode (target_temp == OFF)
        if (status->target_temp_room != TargetTemp::TARGET_TEMP_OFF) {
          break;
        }
        switch (status->heating_mode) {
          case HeatingMode::HEATING_MODE_VENT_1:
            this->publish_state(this->at((size_t) TRUMA_SELECT_TYPE_HEATER_FAN_ONLY_SPEED::VENT_1).value());
            break;
          case HeatingMode::HEATING_MODE_VENT_2:
            this->publish_state(this->at((size_t) TRUMA_SELECT_TYPE_HEATER_FAN_ONLY_SPEED::VENT_2).value());
            break;
          case HeatingMode::HEATING_MODE_VENT_3:
            this->publish_state(this->at((size_t) TRUMA_SELECT_TYPE_HEATER_FAN_ONLY_SPEED::VENT_3).value());
            break;
          case HeatingMode::HEATING_MODE_VENT_4:
            this->publish_state(this->at((size_t) TRUMA_SELECT_TYPE_HEATER_FAN_ONLY_SPEED::VENT_4).value());
            break;
          case HeatingMode::HEATING_MODE_VENT_5:
            this->publish_state(this->at((size_t) TRUMA_SELECT_TYPE_HEATER_FAN_ONLY_SPEED::VENT_5).value());
            break;
          case HeatingMode::HEATING_MODE_VENT_6:
            this->publish_state(this->at((size_t) TRUMA_SELECT_TYPE_HEATER_FAN_ONLY_SPEED::VENT_6).value());
            break;
          case HeatingMode::HEATING_MODE_VENT_7:
            this->publish_state(this->at((size_t) TRUMA_SELECT_TYPE_HEATER_FAN_ONLY_SPEED::VENT_7).value());
            break;
          case HeatingMode::HEATING_MODE_VENT_8:
            this->publish_state(this->at((size_t) TRUMA_SELECT_TYPE_HEATER_FAN_ONLY_SPEED::VENT_8).value());
            break;
          case HeatingMode::HEATING_MODE_VENT_9:
            this->publish_state(this->at((size_t) TRUMA_SELECT_TYPE_HEATER_FAN_ONLY_SPEED::VENT_9).value());
            break;
          case HeatingMode::HEATING_MODE_VENT_10:
            this->publish_state(this->at((size_t) TRUMA_SELECT_TYPE_HEATER_FAN_ONLY_SPEED::VENT_10).value());
            break;
          default:
            this->publish_state(this->at((size_t) TRUMA_SELECT_TYPE_HEATER_FAN_ONLY_SPEED::OFF).value());
            break;
        }
        break;

      default:
        break;
    }
  });
}

void TrumaHeaterSelect::control(const std::string &value) {
  auto index = this->index_of(value);
  if (!index.has_value()) {
    return;
  }
  auto heater_device = this->parent_->get_heater_device();
  auto status_heater = this->parent_->get_heater()->get_status();
  float temp = temp_code_to_decimal(status_heater->target_temp_room, 0);
  if (index.value() > 0 && temp < 5) {
    temp = 5;
  }

  switch (this->type_) {
    case TRUMA_SELECT_TYPE::HEATER_FAN_MODE:
      switch ((TRUMA_SELECT_TYPE_HEATER_FAN_MODE) index.value()) {
        case TRUMA_SELECT_TYPE_HEATER_FAN_MODE::ECO:
          if (heater_device == TRUMA_DEVICE::CPPLUS_VARIO) {
            this->parent_->get_heater()->action_heater_room(static_cast<uint8_t>(temp),
                                                            HeatingMode::HEATING_MODE_VARIO_HEAT_NIGHT);
          } else {
            this->parent_->get_heater()->action_heater_room(static_cast<uint8_t>(temp), HeatingMode::HEATING_MODE_ECO);
          }
          break;
        case TRUMA_SELECT_TYPE_HEATER_FAN_MODE::COMBI_HIGH:
          if (heater_device == TRUMA_DEVICE::CPPLUS_VARIO) {
            this->parent_->get_heater()->action_heater_room(static_cast<uint8_t>(temp),
                                                            HeatingMode::HEATING_MODE_VARIO_HEAT_AUTO);
          } else {
            this->parent_->get_heater()->action_heater_room(static_cast<uint8_t>(temp), HeatingMode::HEATING_MODE_HIGH);
          }
          break;
        case TRUMA_SELECT_TYPE_HEATER_FAN_MODE::BOOST:
          this->parent_->get_heater()->action_heater_room(static_cast<uint8_t>(temp), HeatingMode::HEATING_MODE_BOOST);
          break;
        default:
          this->parent_->get_heater()->action_heater_fan_only(HeatingMode::HEATING_MODE_OFF);
          break;
      }
      break;

    case TRUMA_SELECT_TYPE::HEATER_ENERGY_MIX:
      switch ((TRUMA_SELECT_TYPE_HEATER_ENERGY_MIX) index.value()) {
        case TRUMA_SELECT_TYPE_HEATER_ENERGY_MIX::GAS:
          this->parent_->get_heater()->action_heater_energy_mix(EnergyMix::ENERGY_MIX_GAS);
          break;
        case TRUMA_SELECT_TYPE_HEATER_ENERGY_MIX::MIX_1:
          this->parent_->get_heater()->action_heater_energy_mix(EnergyMix::ENERGY_MIX_MIX,
                                                                ElectricPowerLevel::ELECTRIC_POWER_LEVEL_900);
          break;
        case TRUMA_SELECT_TYPE_HEATER_ENERGY_MIX::MIX_2:
          this->parent_->get_heater()->action_heater_energy_mix(EnergyMix::ENERGY_MIX_MIX,
                                                                ElectricPowerLevel::ELECTRIC_POWER_LEVEL_1800);
          break;
        case TRUMA_SELECT_TYPE_HEATER_ENERGY_MIX::ELECTRIC_1:
          this->parent_->get_heater()->action_heater_energy_mix(EnergyMix::ENERGY_MIX_ELECTRICITY,
                                                                ElectricPowerLevel::ELECTRIC_POWER_LEVEL_900);
          break;
        case TRUMA_SELECT_TYPE_HEATER_ENERGY_MIX::ELECTRIC_2:
          this->parent_->get_heater()->action_heater_energy_mix(EnergyMix::ENERGY_MIX_ELECTRICITY,
                                                                ElectricPowerLevel::ELECTRIC_POWER_LEVEL_1800);
          break;
        default:
          break;
      }
      break;

    case TRUMA_SELECT_TYPE::HEATER_FAN_ONLY_SPEED: {
      // Only act when target_temp is OFF (fan only mode active)
      auto status_heater = this->parent_->get_heater()->get_status();
      if (status_heater->target_temp_room != TargetTemp::TARGET_TEMP_OFF) {
        ESP_LOGW(TAG, "Fan Only Speed ignored - heater is active");
        return;
      }
      switch ((TRUMA_SELECT_TYPE_HEATER_FAN_ONLY_SPEED) index.value()) {
        case TRUMA_SELECT_TYPE_HEATER_FAN_ONLY_SPEED::VENT_1:
          this->parent_->get_heater()->action_heater_fan_only(HeatingMode::HEATING_MODE_VENT_1);
          break;
        case TRUMA_SELECT_TYPE_HEATER_FAN_ONLY_SPEED::VENT_2:
          this->parent_->get_heater()->action_heater_fan_only(HeatingMode::HEATING_MODE_VENT_2);
          break;
        case TRUMA_SELECT_TYPE_HEATER_FAN_ONLY_SPEED::VENT_3:
          this->parent_->get_heater()->action_heater_fan_only(HeatingMode::HEATING_MODE_VENT_3);
          break;
        case TRUMA_SELECT_TYPE_HEATER_FAN_ONLY_SPEED::VENT_4:
          this->parent_->get_heater()->action_heater_fan_only(HeatingMode::HEATING_MODE_VENT_4);
          break;
        case TRUMA_SELECT_TYPE_HEATER_FAN_ONLY_SPEED::VENT_5:
          this->parent_->get_heater()->action_heater_fan_only(HeatingMode::HEATING_MODE_VENT_5);
          break;
        case TRUMA_SELECT_TYPE_HEATER_FAN_ONLY_SPEED::VENT_6:
          this->parent_->get_heater()->action_heater_fan_only(HeatingMode::HEATING_MODE_VENT_6);
          break;
        case TRUMA_SELECT_TYPE_HEATER_FAN_ONLY_SPEED::VENT_7:
          this->parent_->get_heater()->action_heater_fan_only(HeatingMode::HEATING_MODE_VENT_7);
          break;
        case TRUMA_SELECT_TYPE_HEATER_FAN_ONLY_SPEED::VENT_8:
          this->parent_->get_heater()->action_heater_fan_only(HeatingMode::HEATING_MODE_VENT_8);
          break;
        case TRUMA_SELECT_TYPE_HEATER_FAN_ONLY_SPEED::VENT_9:
          this->parent_->get_heater()->action_heater_fan_only(HeatingMode::HEATING_MODE_VENT_9);
          break;
        case TRUMA_SELECT_TYPE_HEATER_FAN_ONLY_SPEED::VENT_10:
          this->parent_->get_heater()->action_heater_fan_only(HeatingMode::HEATING_MODE_VENT_10);
          break;
        default:
          // AUS: turn off completely
          this->parent_->get_heater()->action_heater_room(0);
          break;
      }
      break;
    }

    default:
      break;
  }
}

void TrumaHeaterSelect::dump_config() {
  LOG_SELECT("", "Truma Heater Select", this);
  ESP_LOGCONFIG(TAG, "  Type '%s'", enum_to_c_str(this->type_));
}

}  // namespace truma_inetbox
}  // namespace esphome

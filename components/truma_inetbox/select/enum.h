#pragma once

#include "esphome/core/log.h"

namespace esphome {
namespace truma_inetbox {

enum class TRUMA_SELECT_TYPE {
  UNKNOWN,
  HEATER_FAN_MODE,
  HEATER_ENERGY_MIX,
  HEATER_FAN_ONLY_SPEED,
};

enum class TRUMA_SELECT_TYPE_HEATER_FAN_MODE {
  OFF = 0,
  ECO = 1,
  VARIO_HEAT_NIGHT = 1,
  COMBI_HIGH = 2,
  VARIO_HEAT_AUTO = 2,
  BOOST = 3,
};

enum class TRUMA_SELECT_TYPE_HEATER_ENERGY_MIX {
  GAS = 0,
  MIX_1 = 1,
  MIX_2 = 2,
  ELECTRIC_1 = 3,
  ELECTRIC_2 = 4,
};

enum class TRUMA_SELECT_TYPE_HEATER_FAN_ONLY_SPEED {
  OFF    = 0,
  VENT_1 = 1,
  VENT_2 = 2,
  VENT_3 = 3,
  VENT_4 = 4,
  VENT_5 = 5,
  VENT_6 = 6,
  VENT_7 = 7,
  VENT_8 = 8,
  VENT_9 = 9,
  VENT_10 = 10,
};

#ifdef ESPHOME_LOG_HAS_CONFIG
static const char *enum_to_c_str(const TRUMA_SELECT_TYPE val) {
  switch (val) {
    case TRUMA_SELECT_TYPE::HEATER_FAN_MODE:        return "HEATER_FAN_MODE";
    case TRUMA_SELECT_TYPE::HEATER_ENERGY_MIX:      return "HEATER_ENERGY_MIX";
    case TRUMA_SELECT_TYPE::HEATER_FAN_ONLY_SPEED:  return "HEATER_FAN_ONLY_SPEED";
    default:                                        return "";
  }
}
#endif  // ESPHOME_LOG_HAS_CONFIG

}  // namespace truma_inetbox
}  // namespace esphome

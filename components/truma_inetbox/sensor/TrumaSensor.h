#pragma once

#include "esphome/core/log.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/truma_inetbox/TrumaiNetBoxApp.h"

namespace esphome {
namespace truma_inetbox {

enum class TRUMA_SENSOR_TYPE {
  UNKNOWN,
  CURRENT_ROOM_TEMPERATURE,
  CURRENT_WATER_TEMPERATURE,
  TARGET_ROOM_TEMPERATURE,
  TARGET_WATER_TEMPERATURE,
  HEATING_MODE,
  ELECTRIC_POWER_LEVEL,
  ENERGY_MIX,
  OPERATING_STATUS,
  HEATER_ERROR_CODE,
  TIMER_START_TIME,
  TIMER_STOP_TIME,
  TIMER_ROOM_TEMPERATURE,
  TIMER_WATER_TEMPERATURE,
  CLOCK_HOUR,
  CLOCK_MINUTE,
  CP_PLUS_DISPLAY_STATUS,
  HEATING_STATUS,
};

#ifdef ESPHOME_LOG_HAS_CONFIG
static const char *enum_to_c_str(const TRUMA_SENSOR_TYPE val) {
  switch (val) {
    case TRUMA_SENSOR_TYPE::CURRENT_ROOM_TEMPERATURE:  return "CURRENT_ROOM_TEMPERATURE";
    case TRUMA_SENSOR_TYPE::CURRENT_WATER_TEMPERATURE: return "CURRENT_WATER_TEMPERATURE";
    case TRUMA_SENSOR_TYPE::TARGET_ROOM_TEMPERATURE:   return "TARGET_ROOM_TEMPERATURE";
    case TRUMA_SENSOR_TYPE::TARGET_WATER_TEMPERATURE:  return "TARGET_WATER_TEMPERATURE";
    case TRUMA_SENSOR_TYPE::HEATING_MODE:              return "HEATING_MODE";
    case TRUMA_SENSOR_TYPE::ELECTRIC_POWER_LEVEL:      return "ELECTRIC_POWER_LEVEL";
    case TRUMA_SENSOR_TYPE::ENERGY_MIX:                return "ENERGY_MIX";
    case TRUMA_SENSOR_TYPE::OPERATING_STATUS:          return "OPERATING_STATUS";
    case TRUMA_SENSOR_TYPE::HEATER_ERROR_CODE:         return "HEATER_ERROR_CODE";
    case TRUMA_SENSOR_TYPE::CLOCK_HOUR:                return "CLOCK_HOUR";
    case TRUMA_SENSOR_TYPE::CLOCK_MINUTE:              return "CLOCK_MINUTE";
    case TRUMA_SENSOR_TYPE::CP_PLUS_DISPLAY_STATUS:    return "CP_PLUS_DISPLAY_STATUS";
    case TRUMA_SENSOR_TYPE::HEATING_STATUS:            return "HEATING_STATUS";
    default:                                           return "";
  }
}
#endif  // ESPHOME_LOG_HAS_CONFIG

class TrumaSensor : public Component, public sensor::Sensor, public Parented<TrumaiNetBoxApp> {
 public:
  void setup() override;
  void dump_config() override;
  void set_type(TRUMA_SENSOR_TYPE val) { this->type_ = val; }

 protected:
  TRUMA_SENSOR_TYPE type_;
};

}  // namespace truma_inetbox
}  // namespace esphome

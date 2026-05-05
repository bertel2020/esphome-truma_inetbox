#pragma once

#include "esphome/core/log.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/truma_inetbox/TrumaiNetBoxApp.h"

namespace esphome {
namespace truma_inetbox {

enum class TRUMA_TEXT_SENSOR_TYPE {
  UNKNOWN,
  CLOCK,
  UPDATE_STATUS,
  CP_PLUS_STATUS,
  CP_PLUS_DISPLAY_STATUS,
  HEATING_STATUS,
  HEATING_STATUS_2,
};

#ifdef ESPHOME_LOG_HAS_CONFIG
static const char *enum_to_c_str(const TRUMA_TEXT_SENSOR_TYPE val) {
  switch (val) {
    case TRUMA_TEXT_SENSOR_TYPE::CLOCK:                  return "CLOCK";
    case TRUMA_TEXT_SENSOR_TYPE::UPDATE_STATUS:          return "UPDATE_STATUS";
    case TRUMA_TEXT_SENSOR_TYPE::CP_PLUS_STATUS:         return "CP_PLUS_STATUS";
    case TRUMA_TEXT_SENSOR_TYPE::CP_PLUS_DISPLAY_STATUS: return "CP_PLUS_DISPLAY_STATUS";
    case TRUMA_TEXT_SENSOR_TYPE::HEATING_STATUS:         return "HEATING_STATUS";
    case TRUMA_TEXT_SENSOR_TYPE::HEATING_STATUS_2:       return "HEATING_STATUS_2";
    default: return "";
  }
}
#endif

class TrumaTextSensor : public Component, public text_sensor::TextSensor, public Parented<TrumaiNetBoxApp> {
 public:
  void setup() override;
  void update_status();
  void dump_config() override;
  void set_type(TRUMA_TEXT_SENSOR_TYPE val) { this->type_ = val; }

 protected:
  TRUMA_TEXT_SENSOR_TYPE type_ = TRUMA_TEXT_SENSOR_TYPE::UNKNOWN;
};

}  // namespace truma_inetbox
}  // namespace esphome

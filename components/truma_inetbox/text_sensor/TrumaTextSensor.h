#pragma once

#include "esphome/core/log.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/truma_inetbox/TrumaiNetBoxApp.h"

namespace esphome {
namespace truma_inetbox {

enum class TRUMA_TEXT_SENSOR_TYPE {
  UNKNOWN,
  CLOCK,
};

#ifdef ESPHOME_LOG_HAS_CONFIG
static const char *enum_to_c_str(const TRUMA_TEXT_SENSOR_TYPE val) {
  switch (val) {
    case TRUMA_TEXT_SENSOR_TYPE::CLOCK:
      return "CLOCK";
    default:
      return "";
  }
}
#endif  // ESPHOME_LOG_HAS_CONFIG

class TrumaTextSensor : public Component,
                        public text_sensor::TextSensor,
                        public Parented<TrumaiNetBoxApp> {
 public:
  void setup() override;
  void dump_config() override;

  void set_type(TRUMA_TEXT_SENSOR_TYPE val) { this->type_ = val; }

 protected:
  TRUMA_TEXT_SENSOR_TYPE type_ = TRUMA_TEXT_SENSOR_TYPE::UNKNOWN;
};

}  // namespace truma_inetbox
}  // namespace esphome

#include "TrumaTextSensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace truma_inetbox {

static const char *const TAG = "truma_inetbox.text_sensor";

void TrumaTextSensor::setup() {
  this->parent_->get_clock()->add_on_message_callback([this](const StatusFrameClock *c) {
    switch (this->type_) {
      case TRUMA_TEXT_SENSOR_TYPE::CLOCK: {
        char buf[6];
        snprintf(buf, sizeof(buf), "%02d:%02d", c->clock_hour, c->clock_minute);
        this->publish_state(buf);
        break;
      }
      default: break;
    }
  });
}

void TrumaTextSensor::dump_config() {
  LOG_TEXT_SENSOR("", "Truma Text Sensor", this);
  ESP_LOGCONFIG(TAG, "  Type '%s'", enum_to_c_str(this->type_));
}

}  // namespace truma_inetbox
}  // namespace esphome

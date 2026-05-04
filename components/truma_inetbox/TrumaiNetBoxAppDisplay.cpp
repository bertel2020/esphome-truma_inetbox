#include "TrumaiNetBoxAppDisplay.h"
#include "esphome/core/log.h"

namespace esphome {
namespace truma_inetbox {

static const char *const TAG = "truma_inetbox.display";

bool TrumaiNetBoxAppDisplay::set_display_status(const uint8_t *data, uint8_t length) {
  if (length < sizeof(StatusFrameDisplay)) {
    ESP_LOGW(TAG, "Display status frame too short: %d bytes", length);
    return false;
  }
  StatusFrameDisplay frame = {};
  memcpy(&frame, data, sizeof(StatusFrameDisplay));
  this->set_status(frame);
  ESP_LOGD(TAG, "Display: cp_plus=0x%02X heating=0x%02X heating2=0x%02X voltage=%.1fV",
           frame.cp_plus_display, frame.heating_status, frame.heating_status_2,
           (float) frame.voltage_raw / 10.0f);
  return true;
}

void TrumaiNetBoxAppDisplay::dump_data() const {
  if (!this->data_valid_) return;
  ESP_LOGCONFIG(TAG, "  CP Plus Display: 0x%02X", this->data_.cp_plus_display);
  ESP_LOGCONFIG(TAG, "  Heating Status:  0x%02X", this->data_.heating_status);
  ESP_LOGCONFIG(TAG, "  Heating Status2: 0x%02X", this->data_.heating_status_2);
  ESP_LOGCONFIG(TAG, "  Voltage:         %.1f V", (float) this->data_.voltage_raw / 10.0f);
}

}  // namespace truma_inetbox
}  // namespace esphome

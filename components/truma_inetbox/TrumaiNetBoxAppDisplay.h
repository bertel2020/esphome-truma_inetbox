#pragma once

#include "TrumaStausFrameStorage.h"
#include "TrumaEnums.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace truma_inetbox {

class TrumaiNetBoxApp;

// Display-Status empfangen auf PID 0x22
// Quelle: danielfett/inetbox.py parse_status_2
// databytes[0] = Spannung * 10
// databytes[1] = cp_plus_display_status  (CP_PLUS_DISPLAY_STATUS_MAPPING)
// databytes[2] = heating_status          (HEATING_STATUS_MAPPING)
// databytes[3] = heating_status_2
struct StatusFrameDisplay {  // NOLINT(altera-struct-pack-align)
  uint8_t voltage_raw;      // byte 0: Spannung * 10
  uint8_t cp_plus_display;  // byte 1: CP_PLUS_DISPLAY_STATUS_MAPPING
  uint8_t heating_status;   // byte 2: HEATING_STATUS_MAPPING
  uint8_t heating_status_2; // byte 3: HEATING_STATUS_2_MAPPING
  uint8_t unknown_4;
  uint8_t unknown_5;
  uint8_t unknown_6;
  uint8_t unknown_7;
} __attribute__((packed));

class TrumaiNetBoxAppDisplay : public TrumaStausFrameStorage<StatusFrameDisplay>,
                               public Parented<TrumaiNetBoxApp> {
 public:
  void dump_data() const override;

  bool set_display_status(const uint8_t *data, uint8_t length);

  float get_voltage() const {
    return this->data_valid_ ? (float) this->data_.voltage_raw / 10.0f : 0.0f;
  }
  uint8_t get_cp_plus_display_status() const {
    return this->data_valid_ ? this->data_.cp_plus_display : 0;
  }
  uint8_t get_heating_status() const {
    return this->data_valid_ ? this->data_.heating_status : 0;
  }
  uint8_t get_heating_status_2() const {
    return this->data_valid_ ? this->data_.heating_status_2 : 0;
  }
};

}  // namespace truma_inetbox
}  // namespace esphome

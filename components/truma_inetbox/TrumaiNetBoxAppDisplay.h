#pragma once

#include "TrumaStausFrameStorage.h"
#include "TrumaEnums.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace truma_inetbox {

class TrumaiNetBoxApp;

// Display status frame received on PID 0x22
// Source: danielfett/inetbox.py parse_status_2
struct StatusFrameDisplay {  // NOLINT(altera-struct-pack-align)
  uint8_t voltage_raw;          // byte 0: Spannung * 10
  uint8_t cp_plus_display;      // byte 1: CP_PLUS_DISPLAY_STATUS_MAPPING
  uint8_t heating_status;       // byte 2: HEATING_STATUS_MAPPING
  uint8_t heating_status_2;     // byte 3: HEATING_STATUS_2_MAPPING
  uint8_t unknown_4;
  uint8_t unknown_5;
  uint8_t unknown_6;
  uint8_t unknown_7;
} __attribute__((packed));

class TrumaiNetBoxAppDisplay : public TrumaStausFrameStorage<StatusFrameDisplay>,
                               public Parented<TrumaiNetBoxApp> {
 public:
  void dump_data() const override;

  // Called from TrumaiNetBoxApp::answer_lin_order_ when PID 0x22 is received
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

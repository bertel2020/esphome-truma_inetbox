#pragma once

#include "TrumaStausFrameStorage.h"
#include "TrumaEnums.h"
#include "esphome/core/helpers.h"
#include "esphome/core/automation.h"

namespace esphome {
namespace truma_inetbox {

class TrumaiNetBoxApp;

// Display-Status empfangen auf PID 0x22
struct StatusFrameDisplay {  // NOLINT(altera-struct-pack-align)
  uint8_t voltage_raw;      // byte 0: Spannung * 10
  uint8_t cp_plus_display;  // byte 1: CP_PLUS_DISPLAY_STATUS_MAPPING
  uint8_t heating_status;   // byte 2: HEATING_STATUS_MAPPING
  uint8_t heating_status_2; // byte 3: HEATING_STATUS_2_MAPPING
} __attribute__((packed));

class TrumaiNetBoxAppDisplay : public TrumaStausFrameStorage<StatusFrameDisplay>,
                               public Parented<TrumaiNetBoxApp> {
 public:
  void dump_data() const override;

  bool set_display_status(const uint8_t *data, uint8_t length);

  // PID 0x20: vent mode (databytes[5] >> 4)
  void set_vent_mode(uint8_t vent_mode) {
    this->vent_mode_ = vent_mode;
    this->vent_mode_valid_ = true;
    this->vent_callbacks_.call(vent_mode);
  }

  void add_on_vent_mode_callback(std::function<void(uint8_t)> callback) {
    this->vent_callbacks_.add(std::move(callback));
  }

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
  uint8_t get_vent_mode() const {
    return this->vent_mode_valid_ ? this->vent_mode_ : 0;
  }
  bool get_vent_mode_valid() const { return this->vent_mode_valid_; }

 protected:
  uint8_t vent_mode_{0};
  bool vent_mode_valid_{false};
  CallbackManager<void(uint8_t)> vent_callbacks_{};
};

}  // namespace truma_inetbox
}  // namespace esphome

#pragma once
// Generic HID gamepad support: a small HID report descriptor parser that
// finds the buttons, hat switch and axes of a device's input reports, and a
// decoder that turns a raw input report into a GamepadState.
//
// Face-button numbering differs between controller families, so the map
// carries a layout:
//   - Xbox-style (default): 1 = south, 2 = east, 3 = west, 4 = north
//   - DirectInput / DualShock: 1 = west, 2 = south, 3 = east, 4 = north
//     (picked when the descriptor has a hat switch and >= 13 buttons)
// Always: 5/6 = L1/R1, 7/8 = L2/R2, 9 = select/share, 10 = start/options.
// Positions map to the Nintendo-style layout the emulators use (A = east,
// B = south, X = north, Y = west). Sticks, hat and d-pad usages feed the
// d-pad. Per-controller quirks are keyed on VID/PID (see apply_quirks()).
#include <cstddef>
#include <cstdint>
#include <vector>

#include "gamepad_state.hpp"

struct HidGamepadMap {
  struct Field {
    uint8_t report_id{0}; ///< 0 when the device does not use report IDs
    uint16_t bit_offset{0};
    uint8_t bit_size{0};
    uint16_t usage_page{0};
    uint16_t usage{0};     ///< for array fields: usage minimum
    uint16_t usage_max{0}; ///< array fields only
    int32_t logical_min{0};
    int32_t logical_max{0};
    bool array{false}; ///< the value is a usage index (usage + value - logical_min), 0 = none
  };
  std::vector<Field> buttons; ///< usage page 0x09, usage = button number (1-based)
  std::vector<Field> axes;    ///< generic desktop X/Y/Z/Rx/Ry/Rz
  std::vector<Field> hats;    ///< generic desktop hat switch
  std::vector<Field> dpad;    ///< generic desktop DPad up/down/left/right (0x90..0x93)
  std::vector<Field> consumer; ///< consumer page (0x0C): Menu / AC Home / AC Back ... (variable or array)
  bool uses_report_ids{false};
  bool y_up_positive{false}; ///< quirk: stick Y grows upwards (HID convention is downwards)
  enum class Layout { Xbox, DirectInput };
  Layout layout{Layout::Xbox};
  bool dpad_updown_swapped{false}; ///< quirk: usages 0x90/0x91 carry down/up

  bool empty() const { return buttons.empty() && axes.empty() && hats.empty() && dpad.empty(); }
  /// Parse a HID report descriptor (input items only).
  static HidGamepadMap parse(const uint8_t *desc, size_t len);
  /// Apply known per-controller quirks (button numbering, d-pad usages).
  void apply_quirks(uint16_t vid, uint16_t pid);
  /// Decode an input report; `menu` is set when select+start are held.
  /// `raw` (optional) receives a bitmask of pressed button numbers (bit n =
  /// button n) and the d-pad / hat / axis values, for mapping diagnostics.
  struct Raw {
    uint32_t buttons{0};
    int32_t axes[6]{0, 0, 0, 0, 0, 0}; ///< X Y Z Rx Ry Rz (0 when absent)
    int32_t hat{-1};
    uint8_t dpad{0}; ///< bit0 up, bit1 down, bit2 right, bit3 left
    uint16_t consumer[4]{0, 0, 0, 0}; ///< consumer usages currently active
    uint8_t report_id{0};
    bool known{false}; ///< the report id has fields in this map
  };
  bool decode(const uint8_t *report, size_t len, GamepadState &state, bool &menu, Raw *raw = nullptr,
              GamepadAxes *axes = nullptr) const;
  /// One-line summary for the log
  size_t button_count() const { return buttons.size(); }
};

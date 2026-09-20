#pragma once

#include <cstdint>

struct GamepadState {
  enum class Button {
    ANY = -1,
    A = 0,
    B = 1,
    X = 2,
    Y = 3,
    SELECT = 4,
    START = 5,
    UP = 6,
    DOWN = 7,
    LEFT = 8,
    RIGHT = 9,
    L = 10,
    R = 11
  };

  union {
    struct {
      int a : 1;
      int b : 1;
      int x : 1;
      int y : 1;
      int select : 1;
      int start : 1;
      int up : 1;
      int down : 1;
      int left : 1;
      int right : 1;
      int l : 1; ///< left shoulder (SNES L, GBA L)
      int r : 1; ///< right shoulder
    };
    uint16_t buttons{0};
  };

  bool is_pressed(Button button) const {
    switch (button) {
      case Button::ANY: return buttons != 0;
      case Button::A: return a;
      case Button::B: return b;
      case Button::X: return x;
      case Button::Y: return y;
      case Button::SELECT: return select;
      case Button::START: return start;
      case Button::UP: return up;
      case Button::DOWN: return down;
      case Button::LEFT: return left;
      case Button::RIGHT: return right;
      case Button::L: return l;
      case Button::R: return r;
      default: return false;
    }
  }

  bool operator==(const GamepadState& other) const {
    return buttons == other.buttons;
  }
  bool operator!=(const GamepadState& other) const {
    return !(*this == other);
  }
};

/// Analog sticks, -32767..32767; x grows to the right, y grows downwards
/// (the HID convention). 0 when the controller has no sticks.
struct GamepadAxes {
  int16_t lx{0};
  int16_t ly{0};
  int16_t rx{0};
  int16_t ry{0};
};

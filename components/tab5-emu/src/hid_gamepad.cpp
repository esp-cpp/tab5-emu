#include "hid_gamepad.hpp"

#include <algorithm>
#include <cstring>

namespace {

constexpr uint16_t PAGE_GENERIC_DESKTOP = 0x01;
constexpr uint16_t PAGE_BUTTON = 0x09;
constexpr uint16_t PAGE_CONSUMER = 0x0C;
constexpr uint16_t CONSUMER_MENU = 0x0040, CONSUMER_AC_HOME = 0x0223, CONSUMER_AC_BACK = 0x0224;
// Backbone Pro: share = Record, start = AC Properties, options (...) = AC Exit
constexpr uint16_t CONSUMER_RECORD = 0x00B2, CONSUMER_AC_EXIT = 0x0204, CONSUMER_AC_PROPERTIES = 0x0209;
constexpr uint16_t USAGE_X = 0x30, USAGE_Y = 0x31, USAGE_Z = 0x32, USAGE_RX = 0x33, USAGE_RY = 0x34, USAGE_RZ = 0x35;
constexpr uint16_t USAGE_HAT = 0x39;
constexpr uint16_t USAGE_DPAD_UP = 0x90, USAGE_DPAD_LEFT = 0x93;

int32_t item_signed(const uint8_t *p, int size) {
  switch (size) {
  case 1: return static_cast<int8_t>(p[0]);
  case 2: return static_cast<int16_t>(p[0] | (p[1] << 8));
  case 4: return static_cast<int32_t>(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
  default: return 0;
  }
}
uint32_t item_unsigned(const uint8_t *p, int size) {
  uint32_t v = 0;
  for (int i = 0; i < size; i++) v |= static_cast<uint32_t>(p[i]) << (8 * i);
  return v;
}

uint32_t extract_bits(const uint8_t *report, size_t len, unsigned offset, unsigned size) {
  uint32_t v = 0;
  for (unsigned i = 0; i < size && i < 32; i++) {
    const unsigned bit = offset + i;
    if ((bit >> 3) >= len) break;
    v |= static_cast<uint32_t>((report[bit >> 3] >> (bit & 7)) & 1) << i;
  }
  return v;
}

} // namespace

HidGamepadMap HidGamepadMap::parse(const uint8_t *desc, size_t len) {
  HidGamepadMap map;
  struct Globals {
    uint16_t usage_page{0};
    int32_t logical_min{0}, logical_max{0};
    uint32_t report_size{0}, report_count{0};
    uint8_t report_id{0};
  } g;
  std::vector<Globals> stack;
  std::vector<uint16_t> usages; // local usages for the next main item
  uint16_t usage_min = 0, usage_max = 0;
  bool have_usage_range = false;
  // input bit position per report id
  uint16_t bit_pos[256] = {0};

  size_t i = 0;
  while (i < len) {
    const uint8_t prefix = desc[i++];
    if (prefix == 0xFE) { // long item: skip
      if (i + 1 >= len) break;
      const uint8_t data_len = desc[i];
      i += 2 + data_len;
      continue;
    }
    int size = prefix & 0x03;
    if (size == 3) size = 4;
    const uint8_t type = (prefix >> 2) & 0x03;
    const uint8_t tag = prefix >> 4;
    if (i + size > len) break;
    const uint8_t *data = desc + i;
    i += size;
    switch (type) {
    case 0: // main
      if (tag == 0x8) { // Input
        const uint32_t flags = item_unsigned(data, size);
        const bool constant = flags & 0x01;
        const bool variable = flags & 0x02;
        uint16_t &pos = bit_pos[g.report_id];
        if (constant) {
          pos += g.report_size * g.report_count; // padding
        } else if (!variable) {
          // array item: each slot carries a usage index (consumer controls,
          // keyboards); keep consumer-page ones
          for (uint32_t n = 0; n < g.report_count; n++) {
            if (g.usage_page == PAGE_CONSUMER) {
              Field f;
              f.report_id = g.report_id;
              f.bit_offset = pos;
              f.bit_size = static_cast<uint8_t>(std::min<uint32_t>(g.report_size, 32));
              f.usage_page = g.usage_page;
              f.usage = have_usage_range ? usage_min : (usages.empty() ? 0 : usages.front());
              f.usage_max = have_usage_range ? usage_max : (usages.empty() ? 0 : usages.back());
              f.logical_min = g.logical_min;
              f.logical_max = g.logical_max;
              f.array = true;
              map.consumer.push_back(f);
            }
            pos += g.report_size;
          }
        } else {
          for (uint32_t n = 0; n < g.report_count; n++) {
            uint16_t usage = 0;
            if (have_usage_range) {
              usage = static_cast<uint16_t>(std::min<uint32_t>(usage_min + n, usage_max));
            } else if (!usages.empty()) {
              usage = usages[std::min<size_t>(n, usages.size() - 1)];
            }
            Field f;
            f.report_id = g.report_id;
            f.bit_offset = pos;
            f.bit_size = static_cast<uint8_t>(std::min<uint32_t>(g.report_size, 32));
            f.usage_page = g.usage_page;
            f.usage = usage;
            f.logical_min = g.logical_min;
            f.logical_max = g.logical_max;
            pos += g.report_size;
            if (g.usage_page == PAGE_BUTTON && usage >= 1) {
              map.buttons.push_back(f);
            } else if (g.usage_page == PAGE_GENERIC_DESKTOP) {
              if (usage == USAGE_HAT) {
                map.hats.push_back(f);
              } else if (usage >= USAGE_X && usage <= USAGE_RZ) {
                map.axes.push_back(f);
              } else if (usage >= USAGE_DPAD_UP && usage <= USAGE_DPAD_LEFT) {
                map.dpad.push_back(f);
              }
            } else if (g.usage_page == PAGE_CONSUMER) {
              map.consumer.push_back(f);
            }
          }
        }
      } else if (tag == 0x9 || tag == 0xB) {
        // Output / Feature: their bits live in other reports; nothing to do
      }
      // Collection (0xA) / End Collection (0xC): nothing to track
      usages.clear();
      have_usage_range = false;
      break;
    case 1: // global
      switch (tag) {
      case 0x0: g.usage_page = static_cast<uint16_t>(item_unsigned(data, size)); break;
      case 0x1: g.logical_min = item_signed(data, size); break;
      case 0x2: g.logical_max = item_signed(data, size); break;
      case 0x7: g.report_size = item_unsigned(data, size); break;
      case 0x8:
        g.report_id = static_cast<uint8_t>(item_unsigned(data, size));
        map.uses_report_ids = true;
        break;
      case 0x9: g.report_count = item_unsigned(data, size); break;
      case 0xA: stack.push_back(g); break;
      case 0xB:
        if (!stack.empty()) {
          g = stack.back();
          stack.pop_back();
        }
        break;
      default: break; // physical min/max, unit, exponent
      }
      break;
    case 2: // local
      switch (tag) {
      case 0x0: usages.push_back(static_cast<uint16_t>(item_unsigned(data, size) & 0xFFFF)); break;
      case 0x1: usage_min = static_cast<uint16_t>(item_unsigned(data, size) & 0xFFFF); have_usage_range = true; break;
      case 0x2: usage_max = static_cast<uint16_t>(item_unsigned(data, size) & 0xFFFF); have_usage_range = true; break;
      default: break;
      }
      break;
    default:
      break;
    }
  }
  // a logical_max of 0 (some descriptors omit it for buttons) means 1 bit
  for (auto &b : map.buttons) {
    if (b.logical_max == 0) b.logical_max = 1;
  }
  // DualShock-style descriptors: a hat switch and 13+ buttons
  if (!map.hats.empty() && map.buttons.size() >= 13) {
    map.layout = Layout::DirectInput;
  }
  return map;
}

void HidGamepadMap::apply_quirks(uint16_t vid, uint16_t pid) {
  struct Quirk {
    uint16_t vid, pid;
    Layout layout;
    bool dpad_updown_swapped;
    bool y_up_positive;
  };
  // filled in from controllers seen on the bench (console prints VID/PID)
  static constexpr Quirk quirks[] = {
      {0x054C, 0x05C4, Layout::DirectInput, false, false}, // Sony DualShock 4
      {0x054C, 0x09CC, Layout::DirectInput, false, false}, // Sony DualShock 4 (v2)
      {0x054C, 0x0CE6, Layout::DirectInput, false, false}, // Sony DualSense
      {0x358A, 0x0402, Layout::Xbox, false, true},         // Backbone Pro (sticks report up as +)
  };
  for (const auto &q : quirks) {
    if (q.vid == vid && q.pid == pid) {
      layout = q.layout;
      dpad_updown_swapped = q.dpad_updown_swapped;
      y_up_positive = q.y_up_positive;
      return;
    }
  }
}

bool HidGamepadMap::decode(const uint8_t *report, size_t len, GamepadState &state, bool &menu, Raw *raw) const {
  if (len == 0) return false;
  uint8_t report_id = 0;
  if (uses_report_ids) {
    report_id = report[0];
    report++;
    len--;
  }
  GamepadState s{};
  if (raw) raw->report_id = report_id;
  auto field_value = [&](const Field &f, bool &ok) -> int32_t {
    ok = f.report_id == report_id;
    if (!ok) return 0;
    uint32_t raw = extract_bits(report, len, f.bit_offset, f.bit_size);
    if (f.logical_min < 0 && f.bit_size < 32 && (raw & (1u << (f.bit_size - 1)))) {
      return static_cast<int32_t>(raw | (~0u << f.bit_size)); // sign-extend
    }
    return static_cast<int32_t>(raw);
  };
  bool any = false;
  bool l = false, r = false;
  for (const auto &b : buttons) {
    bool ok;
    const bool pressed = field_value(b, ok) != 0;
    if (!ok) continue;
    any = true;
    if (raw && pressed && b.usage < 32) raw->buttons |= 1u << b.usage;
    // face buttons by layout: (south, east, west, north) button numbers
    const bool xbox = layout == Layout::Xbox;
    const uint16_t south = xbox ? 1 : 2, east = xbox ? 2 : 3, west = xbox ? 3 : 1, north = 4;
    if (b.usage == south) s.b |= pressed;
    else if (b.usage == east) s.a |= pressed;
    else if (b.usage == west) s.y |= pressed;
    else if (b.usage == north) s.x |= pressed;
    switch (b.usage) {
    case 5: l |= pressed; break;
    case 6: r |= pressed; break;
    case 7: l |= pressed; break;
    case 8: r |= pressed; break;
    case 9: s.select |= pressed; break;
    case 10: s.start |= pressed; break;
    default: break;
    }
  }
  s.l = l;
  s.r = r;
  for (const auto &h : hats) {
    bool ok;
    const int32_t v = field_value(h, ok);
    if (!ok) continue;
    any = true;
    if (raw) raw->hat = v;
    const int32_t dir = v - h.logical_min; // 0 = up, clockwise, 8 directions
    if (v >= h.logical_min && v <= h.logical_max && dir < 8) {
      s.up |= dir == 7 || dir == 0 || dir == 1;
      s.right |= dir >= 1 && dir <= 3;
      s.down |= dir >= 3 && dir <= 5;
      s.left |= dir >= 5 && dir <= 7;
    }
  }
  for (const auto &d : dpad) {
    bool ok;
    const bool pressed = field_value(d, ok) != 0;
    if (!ok) continue;
    any = true;
    if (raw && pressed) raw->dpad |= 1u << (d.usage - 0x90);
    switch (d.usage) {
    case 0x90: (dpad_updown_swapped ? s.down : s.up) |= pressed; break;
    case 0x91: (dpad_updown_swapped ? s.up : s.down) |= pressed; break;
    case 0x92: s.right |= pressed; break;
    case 0x93: s.left |= pressed; break;
    default: break;
    }
  }
  // the left stick (X / Y) as a d-pad with a 40% dead zone
  for (const auto &a : axes) {
    bool ok;
    const int32_t v = field_value(a, ok);
    if (!ok || a.logical_max <= a.logical_min) continue;
    if (raw && a.usage >= USAGE_X && a.usage <= USAGE_RZ) raw->axes[a.usage - USAGE_X] = v;
    if (a.usage != USAGE_X && a.usage != USAGE_Y) continue;
    any = true;
    const int32_t range = a.logical_max - a.logical_min;
    const int32_t centered = 2 * (v - a.logical_min) - range; // -range .. +range
    const bool neg = centered < -(range * 2 / 5);
    const bool pos = centered > (range * 2 / 5);
    if (a.usage == USAGE_X) {
      s.left |= neg;
      s.right |= pos;
    } else if (y_up_positive) {
      s.up |= pos;
      s.down |= neg;
    } else {
      s.up |= neg;
      s.down |= pos;
    }
  }
  // consumer controls: Menu = start, AC Back = select, AC Home = pause menu
  bool home = false;
  size_t n_consumer = 0;
  for (const auto &c : consumer) {
    bool ok;
    const int32_t v = field_value(c, ok);
    if (!ok) continue;
    any = true;
    uint16_t usage = 0;
    if (c.array) {
      if (v == 0 && c.logical_min == 0 && c.usage == 0) continue; // empty slot
      usage = static_cast<uint16_t>(c.usage + (v - c.logical_min));
      if (v == c.logical_min && c.usage != 0) continue; // first index = none for offset ranges
    } else {
      if (!v) continue;
      usage = c.usage;
    }
    if (raw && n_consumer < 4) raw->consumer[n_consumer++] = usage;
    switch (usage) {
    case CONSUMER_MENU:
    case CONSUMER_AC_PROPERTIES: s.start = 1; break;
    case CONSUMER_AC_BACK:
    case CONSUMER_AC_EXIT: s.select = 1; break;
    case CONSUMER_AC_HOME:
    case CONSUMER_RECORD: home = true; break; // share / capture: the pause menu (screenshots live there)
    default: break;
    }
  }
  if (raw) raw->known = any;
  if (!any) return false;
  menu = (s.select && s.start) || home;
  state = s;
  return true;
}

#include "make_color.h"

// RGB565, the format of the frames the cores push (for the glue shared with
// esp-box-emu)
extern "C" uint16_t make_color(uint8_t r, uint8_t g, uint8_t b) {
  return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

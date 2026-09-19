// SNES core glue for tab5-emu (snes9x2005 via snes_glue.c)
#include "snes.hpp"
#include "snes_glue.h"

#include <chrono>
#include <cstring>
#include <fstream>
#include <thread>
#include <vector>

#include <esp_heap_caps.h>
#include <esp_timer.h>

#include "box-emu.hpp"
#include "statistics.hpp"

extern "C" {
#include "snes9x.h" // SNES_*_MASK
}

namespace {
bool g_initialized = false;
uint8_t *g_last_frame = nullptr;
int g_last_w = 256, g_last_h = 224;
int g_skip_run = 0;
constexpr int MAX_SKIP = 3;
}

extern "C" uint32_t snes_host_read_joypad(int port) {
  if (port != 0) return 0;
  const auto st = BoxEmu::get().gamepad_state();
  uint32_t j = 0;
  if (st.a) j |= SNES_A_MASK;
  if (st.b) j |= SNES_B_MASK;
  if (st.x) j |= SNES_X_MASK;
  if (st.y) j |= SNES_Y_MASK;
  if (st.start) j |= SNES_START_MASK;
  if (st.select) j |= SNES_SELECT_MASK;
  if (st.up) j |= SNES_UP_MASK;
  if (st.down) j |= SNES_DOWN_MASK;
  if (st.left) j |= SNES_LEFT_MASK;
  if (st.right) j |= SNES_RIGHT_MASK;
  // shoulder buttons: keyboard Q / W when a USB keyboard is attached
  if (BoxEmu::get().usb_keyboard_present()) {
    const auto kb = BoxEmu::get().keyboard_state();
    auto key = [&](int usage) { return (kb.keys[usage >> 3] >> (usage & 7)) & 1; };
    if (key(20)) j |= SNES_TL_MASK; // q
    if (key(26)) j |= SNES_TR_MASK; // w
  }
  return j;
}

extern "C" void snes_host_audio(const int16_t *stereo, size_t frames) {
  if (BoxEmu::get().is_muted()) return;
  BoxEmu::get().play_audio(reinterpret_cast<const uint8_t *>(stereo), frames * 2 * sizeof(int16_t));
}

void init_snes(uint8_t *romdata, size_t rom_data_size) {
  g_initialized = false;
  if (!snes_glue_init(romdata, rom_data_size)) {
    fmt::print("snes: init failed\n");
    snes_glue_deinit();
    return;
  }
  auto &emu = BoxEmu::get();
  emu.palette(nullptr);
  emu.audio_sample_rate(snes_glue_audio_rate());
  emu.audio_max_wait_ms(30);
  emu.native_size(256, 224, snes_glue_pitch() / 2);
  g_last_w = 256;
  g_last_h = 224;
  g_skip_run = 0;
  reset_frame_time();
  g_initialized = true;
  fmt::print("snes: ROM loaded ({}), {} state bytes\n", snes_glue_pal() ? "PAL" : "NTSC", snes_glue_state_size());
}

void reset_snes() {
  if (g_initialized) snes_glue_reset();
}

void run_snes_rom() {
  if (!g_initialized) return;
  const auto start = esp_timer_get_time();
  const uint64_t frame_us = snes_glue_pal() ? 20000 : 16667;
  // adaptive frameskip: after a slow frame, skip rendering up to MAX_SKIP
  static uint64_t last_elapsed = 0;
  bool render = true;
  if (last_elapsed > frame_us && g_skip_run < MAX_SKIP) {
    render = false;
    g_skip_run++;
  } else {
    g_skip_run = 0;
  }
  uint8_t *screen = snes_glue_screen_swap();
  snes_glue_run_frame(render);
  if (render) {
    const int w = snes_glue_width(), h = snes_glue_height();
    if (w != g_last_w || h != g_last_h) {
      BoxEmu::get().wait_frame();
      BoxEmu::get().native_size(w, h, snes_glue_pitch() / 2);
      g_last_w = w;
      g_last_h = h;
    }
    g_last_frame = screen;
    BoxEmu::get().push_frame(screen);
  }
  const uint64_t elapsed = esp_timer_get_time() - start;
  last_elapsed = elapsed;
  update_frame_time(elapsed);
  if (elapsed < frame_us) {
    std::this_thread::sleep_for(std::chrono::microseconds(frame_us - elapsed));
  } else {
    std::this_thread::yield();
  }
}

void load_snes(std::string_view save_path) {
  if (!g_initialized || save_path.empty()) return;
  std::ifstream f(save_path.data(), std::ios::binary);
  if (!f) return;
  std::vector<uint8_t> buf((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  if (!snes_glue_state_load(buf.data(), buf.size())) {
    fmt::print("snes: state load failed ({} bytes, expected {})\n", buf.size(), snes_glue_state_size());
  }
}

void save_snes(std::string_view save_path) {
  if (!g_initialized || save_path.empty()) return;
  std::vector<uint8_t> buf(snes_glue_state_size());
  if (!snes_glue_state_save(buf.data(), buf.size())) return;
  std::ofstream f(save_path.data(), std::ios::binary);
  f.write(reinterpret_cast<const char *>(buf.data()), buf.size());
}

std::pair<size_t, size_t> get_snes_video_size() { return {static_cast<size_t>(g_last_w), static_cast<size_t>(g_last_h)}; }

std::span<uint8_t> get_snes_video_buffer() {
  if (!g_initialized || !g_last_frame) return {};
  // the screenshot writer expects a packed frame; repack the pitch
  static std::vector<uint8_t> packed;
  packed.resize(static_cast<size_t>(g_last_w) * g_last_h * 2);
  for (int y = 0; y < g_last_h; y++) {
    memcpy(packed.data() + static_cast<size_t>(y) * g_last_w * 2, g_last_frame + static_cast<size_t>(y) * snes_glue_pitch(),
           static_cast<size_t>(g_last_w) * 2);
  }
  return packed;
}

void deinit_snes() {
  if (!g_initialized) return;
  g_initialized = false;
  snes_glue_deinit();
  BoxEmu::get().audio_sample_rate(48000);
  g_last_frame = nullptr;
}

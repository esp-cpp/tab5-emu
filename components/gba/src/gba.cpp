// GBA core glue: a minimal libretro host for gpSP's front end.
#include "gba.hpp"

#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#include <esp_timer.h>

#include "box-emu.hpp"
#include "statistics.hpp"

#include <libretro.h>

namespace {
bool g_initialized = false;
int g_frame_index = 0;
uint8_t *g_last_frame = nullptr;
retro_audio_buffer_status_callback_t g_audio_status_cb = nullptr;
uint64_t g_last_elapsed = 0;
float g_fps = 59.7275f;
int g_skip_run = 0; // consecutive skipped frames (bounded by the core's FRAMESKIP_MAX)

constexpr size_t GBA_W = 240, GBA_H = 160;

// core options the host answers
struct Option {
  const char *key;
  const char *value;
};
const Option OPTIONS[] = {
    {"gpsp_frameskip", "auto"},     // skip when the host reports an audio underrun
    {"gpsp_sound_rate", "32768"},   // native-ish bandwidth, half the mixing work
    {"gpsp_drc", "disabled"},       // no dynarec on RISC-V anyway
    {"gpsp_bios", "auto"},          // gba_bios.bin next to the ROM if present
    {"gpsp_boot_mode", "game"},
    {"gpsp_color_correction", "disabled"},
    {"gpsp_frame_mixing", "disabled"},
    {"gpsp_sprlim", "disabled"},
    {"gpsp_rtc", "auto"},
    {"gpsp_rumble", "disabled"},
    {"gpsp_serial", "auto"},
    {"gpsp_turbo_period", "4"},
    {"gpsp_frameskip_threshold", "33"},
    {"gpsp_frameskip_interval", "0"},
};

void log_cb(enum retro_log_level level, const char *fmt, ...) {
  if (level < RETRO_LOG_INFO) return;
  char buf[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  fmt::print("[gpsp] {}", buf);
}

bool environ_cb(unsigned cmd, void *data) {
  switch (cmd) {
  case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
    return *static_cast<enum retro_pixel_format *>(data) == RETRO_PIXEL_FORMAT_RGB565;
  case RETRO_ENVIRONMENT_GET_VARIABLE: {
    auto *var = static_cast<struct retro_variable *>(data);
    for (const auto &o : OPTIONS) {
      if (strcmp(o.key, var->key) == 0) {
        var->value = o.value;
        return true;
      }
    }
    var->value = nullptr;
    return false;
  }
  case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
    *static_cast<bool *>(data) = false;
    return true;
  case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
    static_cast<struct retro_log_callback *>(data)->log = &log_cb;
    return true;
  case RETRO_ENVIRONMENT_GET_INPUT_BITMASKS:
    return true;
  case RETRO_ENVIRONMENT_SET_AUDIO_BUFFER_STATUS_CALLBACK: {
    auto *cb = static_cast<const struct retro_audio_buffer_status_callback *>(data);
    g_audio_status_cb = cb ? cb->callback : nullptr;
    return true;
  }
  case RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO: {
    auto *av = static_cast<const struct retro_system_av_info *>(data);
    g_fps = av->timing.fps;
    BoxEmu::get().audio_sample_rate(static_cast<uint32_t>(av->timing.sample_rate));
    return true;
  }
  case RETRO_ENVIRONMENT_SET_MESSAGE:
    fmt::print("[gpsp] {}\n", static_cast<const struct retro_message *>(data)->msg);
    return true;
  case RETRO_ENVIRONMENT_SET_MESSAGE_EXT:
    fmt::print("[gpsp] {}\n", static_cast<const struct retro_message_ext *>(data)->msg);
    return true;
  case RETRO_ENVIRONMENT_GET_MESSAGE_INTERFACE_VERSION:
    *static_cast<unsigned *>(data) = 1;
    return true;
  default:
    return false; // system dir, VFS, perf, rumble, memory maps, netpacket, ...
  }
}

void video_cb(const void *data, unsigned width, unsigned height, size_t pitch) {
  if (!data) return; // skipped frame
  auto &emu = BoxEmu::get();
  uint8_t *dst = g_frame_index ? emu.frame_buffer1() : emu.frame_buffer0();
  if (!dst) return;
  const size_t row = width * 2;
  if (pitch == row) {
    memcpy(dst, data, row * height);
  } else {
    for (unsigned y = 0; y < height; y++) {
      memcpy(dst + y * row, static_cast<const uint8_t *>(data) + y * pitch, row);
    }
  }
  g_last_frame = dst;
  emu.push_frame(dst);
  g_frame_index ^= 1;
}

size_t audio_batch_cb(const int16_t *data, size_t frames) {
  if (!BoxEmu::get().is_muted()) {
    BoxEmu::get().play_audio(reinterpret_cast<const uint8_t *>(data), frames * 4);
  }
  return frames;
}

void input_poll_cb() {}

int16_t input_state_cb(unsigned port, unsigned device, unsigned index, unsigned id) {
  if (port != 0 || device != RETRO_DEVICE_JOYPAD) return 0;
  const auto st = BoxEmu::get().gamepad_state();
  bool l = st.x, r = st.y; // the touch pad's X / Y stand in for the shoulders
  if (BoxEmu::get().usb_keyboard_present()) {
    const auto kb = BoxEmu::get().keyboard_state();
    auto key = [&](int usage) { return (kb.keys[usage >> 3] >> (usage & 7)) & 1; };
    l |= key(20); // q
    r |= key(26); // w
  }
  int16_t mask = 0;
  if (st.b) mask |= 1 << RETRO_DEVICE_ID_JOYPAD_B;
  if (st.a) mask |= 1 << RETRO_DEVICE_ID_JOYPAD_A;
  if (st.select) mask |= 1 << RETRO_DEVICE_ID_JOYPAD_SELECT;
  if (st.start) mask |= 1 << RETRO_DEVICE_ID_JOYPAD_START;
  if (st.up) mask |= 1 << RETRO_DEVICE_ID_JOYPAD_UP;
  if (st.down) mask |= 1 << RETRO_DEVICE_ID_JOYPAD_DOWN;
  if (st.left) mask |= 1 << RETRO_DEVICE_ID_JOYPAD_LEFT;
  if (st.right) mask |= 1 << RETRO_DEVICE_ID_JOYPAD_RIGHT;
  if (l) mask |= 1 << RETRO_DEVICE_ID_JOYPAD_L;
  if (r) mask |= 1 << RETRO_DEVICE_ID_JOYPAD_R;
  if (id == RETRO_DEVICE_ID_JOYPAD_MASK) return mask;
  return (mask >> id) & 1;
}
} // namespace

void init_gba(const std::string &rom_path) {
  g_initialized = false;
  retro_set_environment(&environ_cb);
  retro_set_video_refresh(&video_cb);
  retro_set_audio_sample_batch(&audio_batch_cb);
  retro_set_input_poll(&input_poll_cb);
  retro_set_input_state(&input_state_cb);
  retro_init();
  struct retro_game_info info = {};
  info.path = rom_path.c_str();
  if (!retro_load_game(&info)) {
    fmt::print("gba: could not load '{}'\n", rom_path);
    retro_deinit();
    return;
  }
  struct retro_system_av_info av = {};
  retro_get_system_av_info(&av);
  g_fps = av.timing.fps;
  auto &emu = BoxEmu::get();
  emu.palette(nullptr);
  emu.audio_sample_rate(static_cast<uint32_t>(av.timing.sample_rate));
  emu.audio_max_wait_ms(30);
  emu.native_size(GBA_W, GBA_H, GBA_W);
  g_frame_index = 0;
  g_last_elapsed = 0;
  g_skip_run = 0;
  reset_frame_time();
  g_initialized = true;
  fmt::print("gba: '{}' loaded, {:.2f} fps, audio {} Hz, state {} bytes\n", rom_path, g_fps,
             static_cast<unsigned>(av.timing.sample_rate), retro_serialize_size());
}

void reset_gba_core() {
  if (g_initialized) retro_reset();
}

void run_gba_rom() {
  if (!g_initialized) return;
  const auto start = esp_timer_get_time();
  const uint64_t frame_us = static_cast<uint64_t>(1e6 / g_fps);
  // the core's "auto" frameskip skips a frame when the host reports an
  // audio underrun: report one whenever the previous frame ran over budget
  if (g_audio_status_cb) {
    const bool behind = g_last_elapsed > frame_us;
    g_audio_status_cb(true, behind ? 0 : 100, behind);
  }
  retro_run();
  const uint64_t elapsed = esp_timer_get_time() - start;
  g_last_elapsed = elapsed;
  update_frame_time(elapsed);
  if (elapsed < frame_us) {
    std::this_thread::sleep_for(std::chrono::microseconds(frame_us - elapsed));
  } else {
    std::this_thread::yield();
  }
}

void load_gba(std::string_view save_path) {
  if (!g_initialized || save_path.empty()) return;
  std::ifstream f(save_path.data(), std::ios::binary);
  if (!f) return;
  std::vector<uint8_t> buf((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  if (!retro_unserialize(buf.data(), buf.size())) {
    fmt::print("gba: state load failed ({} bytes, expected {})\n", buf.size(), retro_serialize_size());
  }
}

void save_gba(std::string_view save_path) {
  if (!g_initialized || save_path.empty()) return;
  std::vector<uint8_t> buf(retro_serialize_size());
  if (!retro_serialize(buf.data(), buf.size())) return;
  std::ofstream f(save_path.data(), std::ios::binary);
  f.write(reinterpret_cast<const char *>(buf.data()), buf.size());
}

std::span<uint8_t> get_gba_video_buffer() {
  if (!g_initialized || !g_last_frame) return {};
  return {g_last_frame, GBA_W * GBA_H * 2};
}

void deinit_gba() {
  if (!g_initialized) return;
  g_initialized = false;
  retro_unload_game();
  retro_deinit();
  BoxEmu::get().audio_sample_rate(48000);
  g_last_frame = nullptr;
  g_audio_status_cb = nullptr;
}

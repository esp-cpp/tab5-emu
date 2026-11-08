#include "tpm.hpp"

#include <atomic>
#include <cstring>
#include <vector>

#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <sdkconfig.h>

#include "logger.hpp"
#include "tab5-emu.hpp"
#include "tpm_files.hpp"
#if defined(TPM_HAVE_ENGINE)
#include "tpm_engine.hpp"
#endif

// With the engine built (CONFIG_TPM_ENGINE and an executable to recompile at
// configure time) and a disc image on the card, the game runs (tpm_engine).
// Otherwise the core is a viewer for the game's load screens: the 44 .RGB
// files in FILE.000, 320x256 in the GPU's 24-bit mode, each found through the
// executable's own file table.

namespace tpm {

namespace {

constexpr size_t SCREEN_W = 320;
constexpr size_t SCREEN_H = 256;  // the .RGB images
constexpr size_t FRAME_H = 240;   // what the HAL frame buffers hold
constexpr size_t RGB_BYTES = SCREEN_W * SCREEN_H * 3;

espp::Logger logger_({.tag = "tpm", .level = espp::Logger::Verbosity::INFO});

GameFiles files_;
std::vector<const GameFiles::Entry *> screens_;
uint8_t *rgb_{nullptr};      // one load screen, PSRAM
uint8_t *frames_[2]{};       // the HAL's RGB565 frame buffers
int frame_index_{0};
std::atomic<uint8_t *> last_frame_{nullptr};
size_t current_{0};
int64_t shown_at_us_{0};
bool running_{false};
bool paused_{false};
bool prev_left_{false}, prev_right_{false};
Config config_;

bool engine_{false}; // the game is running instead of the viewer

bool show(size_t index) {
  const auto &entry = *screens_[index];
  if (files_.read(entry, 0, rgb_, RGB_BYTES) != RGB_BYTES) {
    logger_.error("short read of {}", entry.name);
    return false;
  }
  auto *dst = reinterpret_cast<uint16_t *>(frames_[frame_index_]);
  // 256 lines into 240: drop 8 at the top and 8 at the bottom (the PSX shows
  // ~240 of them on an NTSC set anyway)
  const uint8_t *src = rgb_ + (SCREEN_H - FRAME_H) / 2 * SCREEN_W * 3;
  for (size_t i = 0; i < SCREEN_W * FRAME_H; i++, src += 3) {
    dst[i] = static_cast<uint16_t>(((src[0] & 0xF8) << 8) | ((src[1] & 0xFC) << 3) | (src[2] >> 3));
  }
  last_frame_ = frames_[frame_index_];
  Tab5Emu::get().push_frame(dst);
  frame_index_ ^= 1;
  shown_at_us_ = esp_timer_get_time();
  logger_.info("{} ({}/{})", entry.name, index + 1, screens_.size());
  return true;
}

} // namespace

bool init(const Config &config) {
  config_ = config;
#if defined(TPM_HAVE_ENGINE)
  if (engine::init(config.data_dir, config.memory_card)) {
    engine_ = true;
    running_ = true;
    return true;
  }
  logger_.warn("the game cannot start; showing the load screens instead");
#endif
  std::string error;
  if (!files_.open(config.data_dir, error)) {
    logger_.error("{}", error);
    return false;
  }
  screens_.clear();
  for (const auto &e : files_.entries()) {
    if (e.size == RGB_BYTES && e.name.size() > 4 && e.name.ends_with(".RGB")) {
      screens_.push_back(&e);
    }
  }
  logger_.info("{} files in the table, {} load screens", files_.entries().size(), screens_.size());
  if (screens_.empty()) {
    files_.close();
    return false;
  }
  rgb_ = static_cast<uint8_t *>(heap_caps_malloc(RGB_BYTES, MALLOC_CAP_SPIRAM));
  auto &emu = Tab5Emu::get();
  frames_[0] = emu.frame_buffer0();
  frames_[1] = emu.frame_buffer1();
  if (!rgb_ || !frames_[0] || !frames_[1]) {
    logger_.error("out of memory");
    deinit();
    return false;
  }
  emu.palette(nullptr, 0); // RGB565 frames
  emu.native_size(SCREEN_W, FRAME_H);
  // start on the logo the game itself opens with, when it is there
  current_ = 0;
  for (size_t i = 0; i < screens_.size(); i++) {
    if (screens_[i]->name == "LOGO.RGB") {
      current_ = i;
    }
  }
  running_ = show(current_);
  return running_;
}

void deinit() {
#if defined(TPM_HAVE_ENGINE)
  if (engine_) {
    engine::deinit();
    engine_ = false;
  }
#endif
  running_ = false;
  files_.close();
  screens_.clear();
  heap_caps_free(rgb_);
  rgb_ = nullptr;
  last_frame_ = nullptr;
}

bool reset() {
  deinit();
  return init(config_);
}

bool running() { return running_; }

bool update() {
#if defined(TPM_HAVE_ENGINE)
  if (engine_) {
    running_ = engine::running();
    return running_;
  }
#endif
  if (!running_ || paused_) {
    return running_;
  }
  const auto pad = Tab5Emu::get().gamepad_state();
  const bool left = pad.left, right = pad.right;
  size_t next = current_;
  if (right && !prev_right_) {
    next = (current_ + 1) % screens_.size();
  } else if (left && !prev_left_) {
    next = (current_ + screens_.size() - 1) % screens_.size();
  } else if (esp_timer_get_time() - shown_at_us_ > CONFIG_TPM_SLIDE_SECONDS * 1000000LL) {
    next = (current_ + 1) % screens_.size();
  }
  prev_left_ = left;
  prev_right_ = right;
  if (next != current_) {
    current_ = next;
    show(current_);
  }
  return running_;
}

void pause() {
  paused_ = true;
#if defined(TPM_HAVE_ENGINE)
  if (engine_) {
    engine::pause();
  }
#endif
}

void resume() {
  paused_ = false;
#if defined(TPM_HAVE_ENGINE)
  if (engine_) {
    engine::resume();
    return;
  }
#endif
  shown_at_us_ = esp_timer_get_time();
  if (running_) {
    Tab5Emu::get().native_size(SCREEN_W, FRAME_H);
    show(current_); // the menu drew over the panel
  }
}

std::pair<size_t, size_t> video_size() { return {SCREEN_W, FRAME_H}; }

std::span<uint8_t> video_buffer_rgb565() {
#if defined(TPM_HAVE_ENGINE)
  if (engine_) {
    return engine::last_frame();
  }
#endif
  auto *f = last_frame_.load();
  return f ? std::span<uint8_t>{f, SCREEN_W * FRAME_H * 2} : std::span<uint8_t>{};
}

bool save_state(const std::string &path) {
#if defined(TPM_HAVE_ENGINE)
  if (engine_) {
    return engine::save_state(path);
  }
#endif
  logger_.warn("save state: the game is not running");
  return false;
}

bool load_state(const std::string &path) {
#if defined(TPM_HAVE_ENGINE)
  if (engine_) {
    return engine::load_state(path);
  }
#endif
  logger_.warn("load state: the game is not running");
  return false;
}

void release_arena() {
#if defined(TPM_HAVE_ENGINE)
  engine::release_arena();
#endif
}

} // namespace tpm

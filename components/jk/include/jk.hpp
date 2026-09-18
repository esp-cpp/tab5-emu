#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <utility>

/// C++ interface of the Jedi Knight (OpenJKDF2) core used by JkCart.
namespace jk {

struct Config {
  std::string game_dir; ///< directory holding episode/, resource/, player/
  std::string save_dir; ///< tab5-emu save directory (slot screenshots)
};

/// Start the engine (or the test pattern when CONFIG_JK_ENGINE is off).
bool init(const Config &config);
/// Tear the engine down and free its memory.
void deinit();
/// Run one iteration of the engine's main loop and present a frame.
/// Returns false when the engine wants to exit.
bool run_frame();
/// Pause / resume (called around the tab5-emu pause menu).
void pause();
void resume();
/// Restart the engine (back to the main menu).
void reset();
/// Save / load through the engine's own save files, tagged with the slot.
void save(const std::string &path, int slot);
void load(const std::string &path, int slot);
/// Native (internal) frame size.
std::pair<size_t, size_t> video_size();
/// Recompute the on-screen size from the HAL's video setting and the frame
/// size currently presented (original = integer scale, fit, fill).
void apply_video_setting();
/// The last presented frame converted to RGB565, for screenshots.
std::span<uint8_t> video_buffer_rgb565();

} // namespace jk

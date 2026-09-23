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
/// Allocate the engine's permanent small-object pool (2 MB of PSRAM). Called
/// by the cart before it releases the ROM arena, so the pool never lands in
/// the arena's hole (it is never freed).
void ensure_pool();
bool init(const Config &config);
/// Tear the engine down and free its memory.
void deinit();
/// Run one iteration of the engine's main loop and present a frame.
/// Returns false when the engine wants to exit.
bool run_frame();
/// Pause / resume (called around the tab5-emu pause menu).
void pause();
void resume();
/// Restart the current level.
void reset();
/// Save / load the running game as an engine save file at `path` (absolute).
/// Loading a save from another level switches to that level first. Both run
/// on the engine thread and block until it has taken the request; false when
/// the engine refused (no level running, player dead, unreadable file).
bool save(const std::string &path, int slot);
bool load(const std::string &path, int slot);
/// Size of the frame most recently presented (for screenshots).
std::pair<size_t, size_t> video_size();
/// Recompute the on-screen size from the HAL's video setting and the frame
/// size currently presented (original = integer scale, fit, fill).
void apply_video_setting();
/// The last presented frame converted to RGB565, for screenshots.
std::span<uint8_t> video_buffer_rgb565();

} // namespace jk

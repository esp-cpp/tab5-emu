#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <utility>

/// C++ interface of the Metal Gear Solid core (mgs_reversing + psyz) used by
/// MgsCart. The game runs in its own FreeRTOS tasks (the PSX's mts scheduler
/// on top of them, see port/esp32_threads.c); the cart starts it, freezes it
/// around the menu, and reads the presented frame for screenshots.
namespace mgs {

struct Config {
  /// Directory holding the disc files: STAGE.DIR, RADIO.DAT, FACE.DAT, ...
  /// (what port/extract_disc.py produces), e.g. "/sdcard/mgs".
  std::string data_dir;
};

/// Start the game. Returns false if the data is missing or the engine could
/// not be started.
bool init(const Config &config);
/// Stop the game and put its statics back as linked, so init() can run again.
void deinit();
/// deinit() + init() with the same configuration.
bool reset();
/// Whether the game is alive (started, and its main() has not returned).
bool running();
/// Freeze / thaw around the tab5-emu pause menu: the game is stopped at a
/// quiescent point (its scheduler idle) and every one of its tasks held.
void pause();
void resume();
/// Size of the presented frame (the PSX's 320x240).
std::pair<size_t, size_t> video_size();
/// The last presented frame as RGB565, for screenshots.
std::span<uint8_t> video_buffer_rgb565();

} // namespace mgs

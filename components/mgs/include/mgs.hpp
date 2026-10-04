#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <utility>

/// C++ interface of the Metal Gear Solid core (mgs_reversing + psyz) used by
/// MgsCart. The game runs in its own FreeRTOS tasks (the PSX's mts scheduler
/// on top of them, see port/esp32_threads.c); the cart only starts it, pauses
/// it around the menu, and reads the presented frame for screenshots.
namespace mgs {

struct Config {
  /// Directory holding the disc files: STAGE.DIR, RADIO.DAT, FACE.DAT, ...
  /// (what port/extract_disc.py produces), e.g. "/sdcard/mgs".
  std::string data_dir;
};

/// Start the game. Returns false if the data is missing or the engine could
/// not be started. There is no second start: the game's statics are not
/// re-initialisable, so deinit() reboots the board.
bool init(const Config &config);
/// Tear down: reboots (see init()).
void deinit();
/// Whether the game's main thread is still alive.
bool running();
/// Pause / resume (called around the tab5-emu pause menu): stops the vblank
/// the game's scheduler runs on, and the scanout with it.
void pause();
void resume();
/// Size of the presented frame (the PSX's 320x240).
std::pair<size_t, size_t> video_size();
/// The last presented frame as RGB565, for screenshots.
std::span<uint8_t> video_buffer_rgb565();

} // namespace mgs

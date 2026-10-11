#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <utility>

/// C++ interface of the Star Wars Episode I: The Phantom Menace core used by
/// TpmCart.
///
/// The game is being decompiled in tpm-decomp (SLUS_008.84, USA Rev 1); until
/// enough of it is C to run, this core proves the platform half on its own:
/// it opens the disc files from the SD card, resolves names through the
/// game's own file table (read out of the executable, as the game does), and
/// shows the load screens. The API is shaped like mgs.hpp so the engine can
/// slot in behind it.
namespace tpm {

struct Config {
  /// Directory holding the disc's files: SLUS_008.84 and FILE.000 (FILE.001
  /// .. FILE.003 later, for streams and movies), e.g. "/sdcard/tpm".
  std::string data_dir;
};

/// Open the data. Returns false if it is missing or not the USA Rev 1 disc.
bool init(const Config &config);
void deinit();
bool reset();
bool running();
/// One step of the core; called from the cart's run loop. Returns false when
/// the core has stopped.
bool update();
void pause();
void resume();
/// Size of the presented frame.
std::pair<size_t, size_t> video_size();
/// The last presented frame as RGB565, for screenshots.
std::span<uint8_t> video_buffer_rgb565();

} // namespace tpm

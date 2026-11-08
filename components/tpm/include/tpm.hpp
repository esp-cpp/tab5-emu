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
  /// The memory card in port 1, a 128 KB card image file (the format PC
  /// emulators use): the game's own saves go there. Made, formatted, the
  /// first time the game looks. Empty: no card.
  std::string memory_card;
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
/// Save states, while paused (the emulator's menu). A state loads only into
/// the firmware build that wrote it, with the game's memory at the same
/// address, which the game normally gets back when it is launched first
/// after a restart.
bool save_state(const std::string &path);
bool load_state(const std::string &path);
/// Free the game's PSRAM block, which is kept across quits for the save
/// states; the launcher calls this before it starts any other cart.
void release_arena();

} // namespace tpm

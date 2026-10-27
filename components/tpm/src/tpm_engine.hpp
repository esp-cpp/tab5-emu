#pragma once

#include <cstdint>
#include <span>
#include <string>

/// The recompiled game (tpm-decomp's recomp runtime) behind tpm.hpp.
namespace tpm::engine {

/// Open the disc image in data_dir (a .bin of raw 2352-byte sectors) and
/// start the game. Returns false if there is no usable image. memory_card:
/// the image file of the card in port 1 (made the first time the game looks;
/// empty: no card).
bool init(const std::string &data_dir, const std::string &memory_card);
void deinit();
bool running();
void pause();
void resume();
/// The last presented frame (320x240 RGB565), or empty.
std::span<uint8_t> last_frame();
/// Save states, while paused. A state loads only into the firmware build
/// that wrote it, with the game's memory where it was (see release_arena).
bool save_state(const std::string &path);
bool load_state(const std::string &path);
/// The game's PSRAM block is kept across quits (save states need it at the
/// same address); the launcher frees it before another cart starts.
void release_arena();

} // namespace tpm::engine

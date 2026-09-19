#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

void reset_snes();
void init_snes(uint8_t *romdata, size_t rom_data_size);
void load_snes(std::string_view save_path);
void save_snes(std::string_view save_path);
void run_snes_rom();
void deinit_snes();
/// Size of the last rendered frame (256 or 512 wide, 224/239/448/478 tall)
std::pair<size_t, size_t> get_snes_video_size();
std::span<uint8_t> get_snes_video_buffer();

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

void reset_gba_core();
/// The ROM is read from the file (gpSP pages large ROMs from disk)
void init_gba(const std::string &rom_path);
void load_gba(std::string_view save_path);
void save_gba(std::string_view save_path);
void run_gba_rom();
void deinit_gba();
std::span<uint8_t> get_gba_video_buffer();

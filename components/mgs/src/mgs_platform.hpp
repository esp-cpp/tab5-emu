#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

/// The PSX resolution MGS renders at (SCREEN_WIDTH/HEIGHT in source/include/common.h).
static constexpr size_t MGS_FRAME_W = 320;
static constexpr size_t MGS_FRAME_H = 240;

/// Tab5 side of the port's platform hooks (src/mgs_platform.cpp).
void mgs_platform_init(const char *data_dir);
/// The last frame handed to the display, RGB565, MGS_FRAME_W x MGS_FRAME_H.
std::span<uint8_t> mgs_platform_last_frame();
/// The SPU mixer pull into the HAL's audio queue (src/mgs_platform.cpp).
void mgs_platform_audio_start();
void mgs_platform_audio_stop();
void mgs_platform_audio_pause();
void mgs_platform_audio_resume();

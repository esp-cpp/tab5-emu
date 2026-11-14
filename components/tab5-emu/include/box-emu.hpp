#pragma once
// Compatibility shim: emulator glue shared with esp-box-emu is written against
// BoxEmu; Tab5Emu mirrors that API.
#include "tab5-emu.hpp"
// (box-emu.hpp brings these in for the glue too)
#include "make_color.h"
#include "pool_allocator.h"

using BoxEmu = Tab5Emu;

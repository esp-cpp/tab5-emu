// The initial contents of .ext_ram.data (components/tab5-emu/ext_ram_data.ld)
// for builds that execute from flash: the cores' initialised data lives in
// PSRAM, which the bootloader cannot load, so a copy of it is kept in flash
// and put in place here, before the C++ constructors (some of which write it).
#include <stdint.h>
#include <string.h>

#include "sdkconfig.h"

#if !CONFIG_SPIRAM_XIP_FROM_PSRAM

// Room for the copy: tools/ext_ram_data_image.py writes the linked bytes of
// .ext_ram.data here after linking, and fails the build if they do not fit
// (66 KB in November 2026: the Metal Gear Solid game's 42 KB and the other
// cores' 24 KB). A nonzero initializer keeps it in .rodata, i.e. in the image.
#define EXT_RAM_DATA_IMAGE_BYTES (96 * 1024)
__attribute__((aligned(16), used)) const uint8_t tab5_ext_ram_data_image[EXT_RAM_DATA_IMAGE_BYTES] = {0xa5};

extern uint8_t _ext_ram_data_start[], _ext_ram_data_end[];

// 101: the first priority applications may use, so before every default
// constructor (.init_array is sorted by priority, sections.ld); PSRAM is
// mapped and its heap set up before any constructor runs (startup.c)
__attribute__((constructor(101), used)) void tab5_ext_ram_data_init(void) {
  const uint8_t *src = tab5_ext_ram_data_image;
  // the compiler must not use what it believes the array holds: its contents
  // are written into the image after compiling
  __asm__ volatile("" : "+r"(src));
  memcpy(_ext_ram_data_start, src, (size_t)(_ext_ram_data_end - _ext_ram_data_start));
}

#endif

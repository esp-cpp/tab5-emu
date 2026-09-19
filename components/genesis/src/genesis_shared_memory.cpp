#include <sdkconfig.h>
#include "genesis_shared_memory.hpp"
#include "shared_memory.h"
#include "esp_log.h"

#include <cstring>

extern "C" {
#include "m68k.h"
#include "ym2612.h"
#include "gwenesis_bus.h"
#include "gwenesis_vdp.h"
}

extern unsigned char* VRAM;
extern unsigned short *CRAM; // [CRAM_MAX_SIZE];           // CRAM - Palettes
extern unsigned char *SAT_CACHE; // [SAT_CACHE_MAX_SIZE];  // Sprite cache
extern unsigned char *gwenesis_vdp_regs; // [REG_SIZE];    // Registers
extern unsigned short *fifo; // [FIFO_SIZE];               // Fifo
extern unsigned short *CRAM565; // [CRAM_MAX_SIZE * 4];    // CRAM - Palettes
extern unsigned short *VSRAM; // [VSRAM_MAX_SIZE];         // VSRAM - Scrolling

uint8_t *M68K_RAM = nullptr; // MAX_RAM_SIZE
uint8_t *ZRAM = nullptr; // MAX_Z80_RAM_SIZE
signed int *tl_tab = nullptr; // 13*2*TL_RES_LEN (13*2*256 * sizeof(signed int)) = 26624 bytes

static constexpr const char *TAG = "genesis_shared_memory";

static void *allocate_shared(size_t size, shared_mem_storage_t storage, shared_mem_region_t region = SHARED_MEM_DEFAULT) {
    shared_mem_request_t request = {
        .size = size,
        .region = region,
        .storage = storage,
    };
    return shared_mem_allocate(&request);
}

static void *allocate_shared_prefer_internal(size_t size, const char *name, shared_mem_region_t region = SHARED_MEM_DEFAULT) {
#if CONFIG_IDF_TARGET_ESP32P4
    // internal RAM is scarce on the P4 and PSRAM is fast enough behind the L2 cache
    (void)name;
    return allocate_shared(size, SHARED_MEM_PSRAM, region);
#endif
    void *ptr = allocate_shared(size, SHARED_MEM_INTERNAL, region);
    if (ptr != nullptr) {
        return ptr;
    }

    ESP_LOGW(TAG, "Allocating %s in PSRAM fallback", name);
    return allocate_shared(size, SHARED_MEM_PSRAM, region);
}

void genesis_init_shared_memory(void) {
    // allocate m68k cpu state in shared memory
    m68k = (m68ki_cpu_core*)allocate_shared_prefer_internal(sizeof(m68ki_cpu_core), "m68k");

    // VRAM is large (64kB) and competes with M68K_RAM for the single big internal
    // block. M68K_RAM is allocated first (in genesis.cpp init()) and is hotter, so
    // VRAM prefers internal but falls back to PSRAM. See note in genesis.cpp.
    VRAM = (uint8_t*)allocate_shared_prefer_internal(VRAM_MAX_SIZE, "VRAM", SHARED_MEM_CACHE_LINE); // 0x10000 (64kB) for VRAM
    ZRAM = (uint8_t*)allocate_shared_prefer_internal(MAX_Z80_RAM_SIZE, "ZRAM"); // 0x2000 (8kB) for Z80 RAM

    ym2612 = (YM2612*)allocate_shared_prefer_internal(sizeof(YM2612), "ym2612");
    OPNREGS = (uint8_t*)allocate_shared_prefer_internal(512, "OPNREGS");
    sin_tab = (unsigned int*)allocate_shared_prefer_internal(SIN_LEN * sizeof(unsigned int), "sin_tab");

    render_buffer = (uint8_t*)allocate_shared_prefer_internal(SCREEN_WIDTH + PIX_OVERFLOW*2, "render_buffer", SHARED_MEM_CACHE_LINE);
    sprite_buffer = (uint8_t*)allocate_shared_prefer_internal(SCREEN_WIDTH + PIX_OVERFLOW*2, "sprite_buffer", SHARED_MEM_CACHE_LINE);

    CRAM = (uint16_t*)allocate_shared_prefer_internal(CRAM_MAX_SIZE * sizeof(uint16_t), "CRAM");
    SAT_CACHE = (uint8_t*)allocate_shared_prefer_internal(SAT_CACHE_MAX_SIZE, "SAT_CACHE", SHARED_MEM_CACHE_LINE);
    gwenesis_vdp_regs = (uint8_t*)allocate_shared_prefer_internal(REG_SIZE, "gwenesis_vdp_regs");
    fifo = (uint16_t*)allocate_shared_prefer_internal(FIFO_SIZE * sizeof(uint16_t), "fifo");
    CRAM565 = (uint16_t*)allocate_shared_prefer_internal(CRAM_MAX_SIZE * 4 * sizeof(uint16_t), "CRAM565");
    VSRAM = (uint16_t*)allocate_shared_prefer_internal(VSRAM_MAX_SIZE * sizeof(uint16_t), "VSRAM");

    tl_tab = (signed int*)allocate_shared_prefer_internal(13*2*256 * sizeof(signed int), "tl_tab");
}

void genesis_free_shared_memory(void) {
    // Clear all shared memory
    shared_mem_clear();
}

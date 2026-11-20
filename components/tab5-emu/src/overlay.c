// The game cores' code overlay (overlay.ld, include/overlay.h): executing
// from flash, every core's code is linked at one PSRAM address and the one a
// game needs is copied there from its slot in flash when it launches.
#include "overlay.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "sdkconfig.h"

#if CONFIG_SPIRAM_XIP_FROM_PSRAM

bool tab5_overlay_load(const char *core) {
  (void)core;
  return true;
}

const char *tab5_overlay_loaded(void) { return ""; }

#else

#include "esp_cache.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "overlay";

typedef void (*ctor_fn)(void);

// linker symbols (overlay.ld): the shared PSRAM range, and per core its slot
// in flash, its code size (the symbol's value) and its constructor table
extern uint8_t _tab5_ovl_start[], _tab5_ovl_end[];
#define TAB5_OVL_SYMBOLS(name)                                                                                        \
  extern const uint8_t _tab5_ovl_##name##_lma[];                                                                       \
  extern const char _tab5_ovl_##name##_size[];                                                                         \
  extern const ctor_fn _tab5_ovl_ctors_##name##_start[], _tab5_ovl_ctors_##name##_end[];
TAB5_OVL_SYMBOLS(gba)
TAB5_OVL_SYMBOLS(snes)
TAB5_OVL_SYMBOLS(genesis)
TAB5_OVL_SYMBOLS(doom)
TAB5_OVL_SYMBOLS(nes)
TAB5_OVL_SYMBOLS(gbc)
TAB5_OVL_SYMBOLS(sms)
TAB5_OVL_SYMBOLS(msx)
TAB5_OVL_SYMBOLS(pico8)
TAB5_OVL_SYMBOLS(jk)
TAB5_OVL_SYMBOLS(darkforces)
TAB5_OVL_SYMBOLS(mgs)
TAB5_OVL_SYMBOLS(scummvm)

typedef struct {
  const char *name;
  const uint8_t *lma;
  size_t size;
  const ctor_fn *ctors, *ctors_end;
} member_t;

#define TAB5_OVL_MEMBER(n)                                                                                            \
  {#n, _tab5_ovl_##n##_lma, (size_t)(uintptr_t)_tab5_ovl_##n##_size, _tab5_ovl_ctors_##n##_start,                     \
   _tab5_ovl_ctors_##n##_end}

static int s_loaded = -1;
static uint16_t s_constructed; // bit per member: its constructors have run

static int find(const char *core, member_t *out) {
  const member_t members[] = {
      TAB5_OVL_MEMBER(gba),   TAB5_OVL_MEMBER(snes),  TAB5_OVL_MEMBER(genesis), TAB5_OVL_MEMBER(doom),
      TAB5_OVL_MEMBER(nes),   TAB5_OVL_MEMBER(gbc),   TAB5_OVL_MEMBER(sms),     TAB5_OVL_MEMBER(msx),
      TAB5_OVL_MEMBER(pico8), TAB5_OVL_MEMBER(jk),    TAB5_OVL_MEMBER(darkforces), TAB5_OVL_MEMBER(mgs),
      TAB5_OVL_MEMBER(scummvm),
  };
  _Static_assert(sizeof(members) / sizeof(members[0]) <= 8 * sizeof(s_constructed), "one bit per member");
  for (int i = 0; i < (int)(sizeof(members) / sizeof(members[0])); i++) {
    if (strcmp(core, members[i].name) == 0) {
      *out = members[i];
      return i;
    }
  }
  return -1;
}

static const char *s_loaded_name = "";

bool tab5_overlay_load(const char *core) {
  member_t m;
  const int i = find(core, &m);
  if (i < 0) {
    ESP_LOGE(TAG, "no core '%s' in the overlay", core);
    return false;
  }
  if (i == s_loaded) {
    return true;
  }
  const size_t capacity = (size_t)(_tab5_ovl_end - _tab5_ovl_start);
  if (m.size > capacity) { // the linker makes the range the largest member's size
    ESP_LOGE(TAG, "%s: %u bytes of code, more than the overlay's %u", core, (unsigned)m.size, (unsigned)capacity);
    return false;
  }
  const int64_t t0 = esp_timer_get_time();
  s_loaded = -1;
  s_loaded_name = "";
  memcpy(_tab5_ovl_start, m.lma, m.size);
  // the code was written through the data cache: write it back to PSRAM, and
  // drop every cached line of the whole range (both cores' instruction caches
  // included, which may still hold the previous core's code)
  esp_cache_msync(_tab5_ovl_start, capacity,
                  ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_INVALIDATE | ESP_CACHE_MSYNC_FLAG_UNALIGNED);
  __asm__ volatile("fence.i" ::: "memory");
  s_loaded = i;
  s_loaded_name = m.name;
  const int64_t t1 = esp_timer_get_time();
  if (!(s_constructed & (1u << i))) {
    // the core's static constructors, once per boot (its data persists)
    s_constructed |= 1u << i;
    for (const ctor_fn *c = m.ctors; c < m.ctors_end; c++) {
      (*c)();
    }
  }
  ESP_LOGI(TAG, "%s: %u KB of code loaded in %lld ms (%u constructors)", core, (unsigned)(m.size / 1024),
           (t1 - t0) / 1000, (unsigned)(m.ctors_end - m.ctors));
  return true;
}

const char *tab5_overlay_loaded(void) { return s_loaded_name; }

#endif

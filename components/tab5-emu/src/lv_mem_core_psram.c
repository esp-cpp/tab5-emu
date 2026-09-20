// LVGL allocator (LV_USE_CUSTOM_MALLOC): everything LVGL allocates goes to
// PSRAM. Its objects, styles and strings are small (~50 bytes on average),
// which the system allocator would otherwise keep in internal RAM; the GUI
// alone cost ~45 KB of it, and internal RAM is what the emulators and the
// SD / USB DMA buffers compete for.
#include <sdkconfig.h>
#include <stdlib.h>

#include <esp_heap_caps.h>

#include <lvgl.h>

#if LV_USE_STDLIB_MALLOC == LV_STDLIB_CUSTOM

#define LV_PSRAM_CAPS (MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)

void lv_mem_init(void) {}
void lv_mem_deinit(void) {}

lv_mem_pool_t lv_mem_add_pool(void *mem, size_t bytes) {
  LV_UNUSED(mem);
  LV_UNUSED(bytes);
  return NULL;
}

void lv_mem_remove_pool(lv_mem_pool_t pool) { LV_UNUSED(pool); }

void *lv_malloc_core(size_t size) {
  void *p = heap_caps_malloc(size, LV_PSRAM_CAPS);
  return p ? p : malloc(size);
}

void *lv_realloc_core(void *p, size_t new_size) {
  void *q = heap_caps_realloc(p, new_size, LV_PSRAM_CAPS);
  return q ? q : realloc(p, new_size);
}

void lv_free_core(void *p) { heap_caps_free(p); }

void lv_mem_monitor_core(lv_mem_monitor_t *mon_p) {
  multi_heap_info_t info;
  heap_caps_get_info(&info, LV_PSRAM_CAPS);
  mon_p->total_size = info.total_free_bytes + info.total_allocated_bytes;
  mon_p->free_size = info.total_free_bytes;
  mon_p->free_biggest_size = info.largest_free_block;
  mon_p->free_cnt = info.free_blocks;
  mon_p->used_cnt = info.allocated_blocks;
  mon_p->used_pct = mon_p->total_size ? (uint8_t)(100 - (100ULL * mon_p->free_size) / mon_p->total_size) : 0;
  mon_p->frag_pct = mon_p->free_size ? (uint8_t)(100 - (100ULL * mon_p->free_biggest_size) / mon_p->free_size) : 0;
  mon_p->max_used = 0;
}

lv_result_t lv_mem_test_core(void) { return LV_RESULT_OK; }

#endif // LV_USE_STDLIB_MALLOC == LV_STDLIB_CUSTOM

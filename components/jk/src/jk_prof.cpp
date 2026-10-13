// Bring-up sampling profiler: the FreeRTOS tick hook on core 0 records the
// interrupted PC (mepc) into a small histogram in internal RAM; jk_prof_dump()
// prints the hottest addresses for addr2line. Debug builds only.
#include <sdkconfig.h>
#if defined(JK_ESP_FS_DEBUG)
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <vector>

#include <esp_attr.h>
#include <esp_heap_caps.h>
#include <esp_freertos_hooks.h>
#include <riscv/csr.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "logger.hpp"

extern "C" {

#define JK_PROF_SLOTS 4096
struct jk_prof_slot { uint32_t pc; uint32_t count; };
// the tables live in PSRAM (allocated on the first start): ~36 KB that would
// otherwise sit in internal RAM even when the profiler is never used
static DRAM_ATTR jk_prof_slot *s_slots = nullptr;
static DRAM_ATTR volatile uint32_t s_total = 0, s_dropped = 0;
static DRAM_ATTR volatile int s_enabled = 0;
// where the main task is parked while idle runs: saved-context frame-pointer walks
#define JK_PROF_CHAINS 48
#define JK_PROF_DEPTH 16
struct jk_prof_chain { uint32_t pc[JK_PROF_DEPTH]; uint32_t count; };
static DRAM_ATTR jk_prof_chain *s_chains = nullptr;
static DRAM_ATTR TaskHandle_t s_main = nullptr;
static DRAM_ATTR TaskHandle_t s_idle0 = nullptr;
static inline bool IRAM_ATTR jk_prof_ptr_ok(uint32_t a) {
  return (a >= 0x4FF00000u && a < 0x4FF80000u) || (a >= 0x48000000u && a < 0x4C000000u);
}
#define JK_PROF_TASKS 16
static DRAM_ATTR TaskHandle_t *s_task_h = nullptr;
static DRAM_ATTR uint32_t *s_task_n = nullptr;
static bool jk_prof_alloc() {
  if (s_slots) return true;
  s_slots = static_cast<jk_prof_slot *>(heap_caps_calloc(JK_PROF_SLOTS, sizeof(jk_prof_slot), MALLOC_CAP_SPIRAM));
  s_chains = static_cast<jk_prof_chain *>(heap_caps_calloc(JK_PROF_CHAINS, sizeof(jk_prof_chain), MALLOC_CAP_SPIRAM));
  s_task_h = static_cast<TaskHandle_t *>(heap_caps_calloc(JK_PROF_TASKS, sizeof(TaskHandle_t), MALLOC_CAP_SPIRAM));
  s_task_n = static_cast<uint32_t *>(heap_caps_calloc(JK_PROF_TASKS, sizeof(uint32_t), MALLOC_CAP_SPIRAM));
  return s_slots && s_chains && s_task_h && s_task_n;
}

static void IRAM_ATTR jk_prof_tick(void) {
  if (!s_slots) return;
  if (!s_enabled) return;
  uint32_t pc = RV_READ_CSR(mepc);
  s_total = s_total + 1;
  TaskHandle_t cur = xTaskGetCurrentTaskHandle();
  if (cur == s_idle0 && s_main) {
    // main is switched out: its saved RvExcFrame sits at pxTopOfStack (first TCB member)
    uint32_t *top = *(uint32_t **)s_main;
    if (jk_prof_ptr_ok((uint32_t)top)) {
      uint32_t chain[JK_PROF_DEPTH] = {0};
      chain[0] = top[0];          // mepc
      uint32_t fp = top[8];       // s0 / frame pointer
      for (int d = 1; d < JK_PROF_DEPTH; d++) {
        if (!jk_prof_ptr_ok(fp) || !jk_prof_ptr_ok(fp - 8)) break;
        uint32_t ra = *(uint32_t *)(fp - 4);
        uint32_t nfp = *(uint32_t *)(fp - 8);
        chain[d] = ra;
        if (nfp <= fp) break;
        fp = nfp;
      }
      for (int i = 0; i < JK_PROF_CHAINS; i++) {
        if (s_chains[i].count == 0) { memcpy(s_chains[i].pc, chain, sizeof(chain)); s_chains[i].count = 1; break; }
        if (memcmp(s_chains[i].pc, chain, sizeof(chain)) == 0) { s_chains[i].count++; break; }
      }
    }
  }
  for (int i = 0; i < JK_PROF_TASKS; i++) {
    if (s_task_h[i] == cur) { s_task_n[i]++; break; }
    if (s_task_h[i] == nullptr) { s_task_h[i] = cur; s_task_n[i] = 1; break; }
  }
  uint32_t h = (pc >> 1) * 2654435761u;
  for (int i = 0; i < 8; i++) {
    uint32_t idx = (h + i) & (JK_PROF_SLOTS - 1);
    if (s_slots[idx].pc == pc) { s_slots[idx].count++; return; }
    if (s_slots[idx].pc == 0) { s_slots[idx].pc = pc; s_slots[idx].count = 1; return; }
  }
  s_dropped = s_dropped + 1;
}

void jk_prof_start(void) {
  static bool registered = false;
  if (!jk_prof_alloc()) {
    return;
  }
  memset(s_slots, 0, JK_PROF_SLOTS * sizeof(jk_prof_slot));
  memset(s_task_h, 0, JK_PROF_TASKS * sizeof(TaskHandle_t));
  memset(s_task_n, 0, JK_PROF_TASKS * sizeof(uint32_t));
  memset(s_chains, 0, JK_PROF_CHAINS * sizeof(jk_prof_chain));
  s_main = xTaskGetHandle("jk_engine");
  if (!s_main) s_main = xTaskGetHandle("main"); // the emulator cores run on the main task
  s_idle0 = xTaskGetIdleTaskHandleForCore(0);
  s_total = 0; s_dropped = 0;
  if (!registered) {
    esp_register_freertos_tick_hook_for_cpu(jk_prof_tick, 0);
    registered = true;
  }
  s_enabled = 1;
}

void jk_prof_stop(void) { s_enabled = 0; }

void jk_prof_release(void) {
  s_enabled = 0;
  heap_caps_free(s_slots); s_slots = nullptr;
  heap_caps_free(s_chains); s_chains = nullptr;
  heap_caps_free(s_task_h); s_task_h = nullptr;
  heap_caps_free(s_task_n); s_task_n = nullptr;
}

// No heap use: this runs on the engine task after a level load has taken
// every byte of PSRAM (the material cache grows until an allocation fails),
// and a std::vector of the slots here was what ended the session with
// std::bad_alloc. The tables are sorted in place and printed with snprintf.
void jk_prof_dump(int top) {
  s_enabled = 0;
  if (!s_slots) return;
  // unused slots sort last: count 0
  std::sort(s_slots, s_slots + JK_PROF_SLOTS,
            [](const jk_prof_slot &a, const jk_prof_slot &b) { return a.count > b.count; });
  size_t distinct = 0;
  while (distinct < JK_PROF_SLOTS && s_slots[distinct].pc && s_slots[distinct].count) distinct++;
  printf("[prof] samples %lu dropped %lu distinct %u\n", (unsigned long)s_total, (unsigned long)s_dropped,
         (unsigned)distinct);
  for (int i = 0; i < JK_PROF_TASKS && s_task_h[i]; i++) {
    printf("[prof]   task %-16s %lu samples\n", pcTaskGetName(s_task_h[i]), (unsigned long)s_task_n[i]);
  }
  std::sort(s_chains, s_chains + JK_PROF_CHAINS,
            [](const jk_prof_chain &a, const jk_prof_chain &b) { return a.count > b.count; });
  static char line[512];
  for (int i = 0; i < 6 && s_chains[i].count; i++) {
    int n = snprintf(line, sizeof line, "[prof] blocked x%lu:", (unsigned long)s_chains[i].count);
    for (int d = 0; d < JK_PROF_DEPTH && s_chains[i].pc[d] && n < (int)sizeof line - 16; d++) {
      n += snprintf(line + n, sizeof line - n, " %#lx", (unsigned long)s_chains[i].pc[d]);
    }
    printf("%s\n", line);
  }
  static char stats[2048];
  vTaskGetRunTimeStats(stats);
  printf("[prof] run-time stats:\n%s\n", stats);
  int n = 0;
  for (size_t i = 0; i < distinct && (int)i < top; i++) {
    n += snprintf(line + n, sizeof line - n, "%#lx:%lu ", (unsigned long)s_slots[i].pc, (unsigned long)s_slots[i].count);
    if ((i % 8) == 7 || n > (int)sizeof line - 32) {
      printf("[prof] %s\n", line);
      n = 0;
    }
  }
  if (n) printf("[prof] %s\n", line);
}

} // extern "C"
#endif

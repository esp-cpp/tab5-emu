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
#include <esp_freertos_hooks.h>
#include <riscv/csr.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "logger.hpp"

extern "C" {

#define JK_PROF_SLOTS 4096
struct jk_prof_slot { uint32_t pc; uint32_t count; };
static DRAM_ATTR jk_prof_slot s_slots[JK_PROF_SLOTS];
static DRAM_ATTR volatile uint32_t s_total = 0, s_dropped = 0;
static DRAM_ATTR volatile int s_enabled = 0;
// where the main task is parked while idle runs: saved-context frame-pointer walks
#define JK_PROF_CHAINS 48
#define JK_PROF_DEPTH 16
struct jk_prof_chain { uint32_t pc[JK_PROF_DEPTH]; uint32_t count; };
static DRAM_ATTR jk_prof_chain s_chains[JK_PROF_CHAINS];
static DRAM_ATTR TaskHandle_t s_main = nullptr;
static DRAM_ATTR TaskHandle_t s_idle0 = nullptr;
static inline bool IRAM_ATTR jk_prof_ptr_ok(uint32_t a) {
  return (a >= 0x4FF00000u && a < 0x4FF80000u) || (a >= 0x48000000u && a < 0x4C000000u);
}
#define JK_PROF_TASKS 16
static DRAM_ATTR TaskHandle_t s_task_h[JK_PROF_TASKS];
static DRAM_ATTR uint32_t s_task_n[JK_PROF_TASKS];

static void IRAM_ATTR jk_prof_tick(void) {
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
  memset(s_slots, 0, sizeof(s_slots));
  memset(s_task_h, 0, sizeof(s_task_h));
  memset(s_task_n, 0, sizeof(s_task_n));
  memset(s_chains, 0, sizeof(s_chains));
  s_main = xTaskGetHandle("main");
  s_idle0 = xTaskGetIdleTaskHandleForCore(0);
  s_total = 0; s_dropped = 0;
  if (!registered) {
    esp_register_freertos_tick_hook_for_cpu(jk_prof_tick, 0);
    registered = true;
  }
  s_enabled = 1;
}

void jk_prof_stop(void) { s_enabled = 0; }

void jk_prof_dump(int top) {
  s_enabled = 0;
  std::vector<jk_prof_slot> v;
  for (auto &s : s_slots) if (s.pc) v.push_back(s);
  std::sort(v.begin(), v.end(), [](const jk_prof_slot &a, const jk_prof_slot &b) { return a.count > b.count; });
  espp::Logger logger({.tag = "prof", .level = espp::Logger::Verbosity::INFO});
  logger.info("samples {} dropped {} distinct {}", s_total, s_dropped, v.size());
  for (int i = 0; i < JK_PROF_TASKS && s_task_h[i]; i++) {
    logger.info("  task {:<16} {} samples", pcTaskGetName(s_task_h[i]), s_task_n[i]);
  }
  std::vector<jk_prof_chain> ch(s_chains, s_chains + JK_PROF_CHAINS);
  std::sort(ch.begin(), ch.end(), [](const jk_prof_chain &a, const jk_prof_chain &b) { return a.count > b.count; });
  for (int i = 0; i < 6 && ch[i].count; i++) {
    std::string l = fmt::format("blocked x{}:", ch[i].count);
    for (int d = 0; d < JK_PROF_DEPTH && ch[i].pc[d]; d++) l += fmt::format(" {:#x}", ch[i].pc[d]);
    logger.info("{}", l);
  }
  static char stats[2048];
  vTaskGetRunTimeStats(stats);
  logger.info("run-time stats:\n{}", stats);
  std::string line;
  for (size_t i = 0; i < v.size() && (int)i < top; i++) {
    line += fmt::format("{:#x}:{} ", v[i].pc, v[i].count);
    if ((i % 8) == 7) { logger.info("{}", line); line.clear(); }
  }
  if (!line.empty()) logger.info("{}", line);
}

} // extern "C"
#endif

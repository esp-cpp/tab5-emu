// Crash reporting at boot: espp::CoreDump reads the flash core dump left by a
// panic (and classifies abnormal resets that leave none, e.g. a brownout) and
// prints its text report. The image stays in flash for the USB link's core
// dump service (espp_coredump / emulink coredump), which also erases it.
#include "tab5-emu.hpp"

#include <algorithm>
#include <cstdio>
#include <vector>


#include "coredump.hpp"

void Tab5Emu::dump_core_dump_to_console() {
  espp::CoreDump core_dump;
  const auto report = core_dump.format_report();
  if (!report.empty()) {
    printf("==== CRASH REPORT ====\n%s\n", report.c_str());
  }
  if (core_dump.has_core_dump()) {
    printf("==== CORE DUMP in flash (%u bytes): fetch it over USB with espp_coredump / emulink coredump ====\n",
           (unsigned)core_dump.image_size());
  }
}

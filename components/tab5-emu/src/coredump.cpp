// Crash reporting at boot: espp::CoreDump reads the flash core dump left by a
// panic (and classifies abnormal resets that leave none, e.g. a brownout),
// prints its text report, then prints the whole image base64 to the console
// so it can be decoded on the PC (esp-coredump info_corefile -t raw) even when
// the crash happened while the console was unavailable (the USB drive
// hand-over), and erases it.
#include "tab5-emu.hpp"

#include <algorithm>
#include <cstdio>
#include <vector>

#include <mbedtls/base64.h>

#include "coredump.hpp"

void Tab5Emu::dump_core_dump_to_console() {
  espp::CoreDump core_dump;
  const auto report = core_dump.format_report();
  if (!report.empty()) {
    printf("==== CRASH REPORT ====\n%s\n", report.c_str());
  }
  if (!core_dump.has_core_dump()) {
    return;
  }
  const size_t size = core_dump.image_size();
  printf("==== CORE DUMP BEGIN (%u bytes, base64) ====\n", (unsigned)size);
  std::vector<uint8_t> raw(3 * 1024);
  std::vector<unsigned char> b64(4 * 1024 + 16);
  std::error_code ec;
  for (size_t off = 0; off < size; off += raw.size()) {
    const size_t n = std::min(raw.size(), size - off);
    if (!core_dump.read_image(off, std::span<uint8_t>(raw.data(), n), ec)) {
      printf("==== CORE DUMP READ FAILED at %u: %s ====\n", (unsigned)off, ec.message().c_str());
      break;
    }
    size_t olen = 0;
    mbedtls_base64_encode(b64.data(), b64.size(), &olen, raw.data(), n);
    fwrite(b64.data(), 1, olen, stdout);
    fputc('\n', stdout);
  }
  printf("==== CORE DUMP END ====\n");
  fflush(stdout);
  core_dump.erase(ec);
}

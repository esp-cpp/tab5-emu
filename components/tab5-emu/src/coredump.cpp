#include "tab5-emu.hpp"

#include <algorithm>
#include <cstdio>
#include <vector>

#include <esp_core_dump.h>
#include <esp_partition.h>
#include <mbedtls/base64.h>

void Tab5Emu::dump_core_dump_to_console() {
  if (esp_core_dump_image_check() != ESP_OK) {
    return;
  }
  size_t addr = 0, size = 0;
  if (esp_core_dump_image_get(&addr, &size) != ESP_OK || size == 0) {
    return;
  }
  const esp_partition_t *part =
      esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_COREDUMP, nullptr);
  if (!part) {
    return;
  }
  printf("==== CORE DUMP BEGIN (%u bytes, base64) ====\n", (unsigned)size);
  std::vector<uint8_t> raw(3 * 1024);
  std::vector<unsigned char> b64(4 * 1024 + 16);
  for (size_t off = 0; off < size; off += raw.size()) {
    const size_t n = std::min(raw.size(), size - off);
    if (esp_partition_read(part, off, raw.data(), n) != ESP_OK) {
      break;
    }
    size_t olen = 0;
    mbedtls_base64_encode(b64.data(), b64.size(), &olen, raw.data(), n);
    fwrite(b64.data(), 1, olen, stdout);
    fputc('\n', stdout);
  }
  printf("==== CORE DUMP END ====\n");
  fflush(stdout);
  esp_core_dump_image_erase();
}

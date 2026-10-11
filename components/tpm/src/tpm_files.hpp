#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace tpm {

/// The game's file system: a name table inside SLUS_008.84 pointing into
/// FILE.000. The executable asks for PC-style paths
/// ("c:\zanzibar\data\levels\%s.LVL"); on the disc only the base name is
/// kept, so lookups here are by base name, case-insensitive.
///
///   struct FileEntry { u8 msf[3]; u8 pad; u32 size; char name[16]; };
///
/// at 0x800715A0 in the USA Rev 1 executable. msf is a BCD position inside
/// FILE.000 with the 2-second lead-in (00:02:00 = its first sector).
class GameFiles {
public:
  struct Entry {
    std::string name;
    uint32_t sector;
    uint32_t size;
  };

  ~GameFiles();

  /// Read the table out of `<dir>/SLUS_008.84` and open `<dir>/FILE.000`.
  /// Fails (with a reason) on anything but the USA Rev 1 executable.
  bool open(const std::string &dir, std::string &error);
  void close();

  const std::vector<Entry> &entries() const { return entries_; }
  const Entry *find(std::string_view name) const;
  /// Read `size` bytes at `offset` of a file into `dst`. Returns bytes read.
  size_t read(const Entry &entry, uint32_t offset, void *dst, size_t size);

private:
  std::vector<Entry> entries_;
  FILE *data_{nullptr};
};

} // namespace tpm

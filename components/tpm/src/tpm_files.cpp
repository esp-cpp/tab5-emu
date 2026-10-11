#include "tpm_files.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>

namespace tpm {

namespace {

constexpr size_t EXE_SIZE = 487424;      // SLUS_008.84, USA Rev 1
constexpr uint32_t EXE_TEXT = 0x80010000;
constexpr uint32_t EXE_HEADER = 0x800;
constexpr uint32_t FILE_TABLE = 0x800715A0;
constexpr size_t ENTRY_SIZE = 24;
constexpr size_t MAX_ENTRIES = 128;      // 107 on the disc
constexpr uint32_t SECTOR = 2048;

int bcd(uint8_t b) { return (b >> 4) * 10 + (b & 15); }

bool iequals(std::string_view a, std::string_view b) {
  return a.size() == b.size() &&
         std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) {
           return std::tolower(static_cast<unsigned char>(x)) ==
                  std::tolower(static_cast<unsigned char>(y));
         });
}

} // namespace

GameFiles::~GameFiles() { close(); }

bool GameFiles::open(const std::string &dir, std::string &error) {
  close();
  const std::string exe_path = dir + "/SLUS_008.84";
  FILE *exe = fopen(exe_path.c_str(), "rb");
  if (!exe) {
    error = "missing " + exe_path;
    return false;
  }
  fseek(exe, 0, SEEK_END);
  const long exe_size = ftell(exe);
  uint8_t magic[8] = {};
  fseek(exe, 0, SEEK_SET);
  fread(magic, 1, sizeof(magic), exe);
  if (exe_size != static_cast<long>(EXE_SIZE) || memcmp(magic, "PS-X EXE", 8) != 0) {
    fclose(exe);
    error = exe_path + " is not the USA Rev 1 executable";
    return false;
  }

  uint8_t table[MAX_ENTRIES * ENTRY_SIZE];
  fseek(exe, FILE_TABLE - EXE_TEXT + EXE_HEADER, SEEK_SET);
  const size_t got = fread(table, 1, sizeof(table), exe);
  fclose(exe);

  for (size_t i = 0; i + ENTRY_SIZE <= got; i += ENTRY_SIZE) {
    const uint8_t *e = table + i;
    const char *name = reinterpret_cast<const char *>(e + 8);
    const size_t len = strnlen(name, 16);
    if (len == 0 || !std::all_of(name, name + len, [](char c) { return c > 32 && c < 127; })) {
      break;
    }
    const int sector = (bcd(e[0]) * 60 + bcd(e[1])) * 75 + bcd(e[2]) - 150;
    uint32_t size;
    memcpy(&size, e + 4, sizeof(size));
    entries_.push_back({std::string(name, len), static_cast<uint32_t>(sector), size});
  }
  // the first record is the Assault level's sound bank on this disc
  if (entries_.size() < 100 || entries_.front().name != "ASAULT.PCV") {
    entries_.clear();
    error = "no file table in " + exe_path + " (not USA Rev 1?)";
    return false;
  }

  const std::string data_path = dir + "/FILE.000";
  data_ = fopen(data_path.c_str(), "rb");
  if (!data_) {
    entries_.clear();
    error = "missing " + data_path;
    return false;
  }
  return true;
}

void GameFiles::close() {
  if (data_) {
    fclose(data_);
    data_ = nullptr;
  }
  entries_.clear();
}

const GameFiles::Entry *GameFiles::find(std::string_view name) const {
  // accept a full game path: keep what follows the last separator
  const auto slash = name.find_last_of("\\/");
  if (slash != std::string_view::npos) {
    name.remove_prefix(slash + 1);
  }
  for (const auto &e : entries_) {
    if (iequals(e.name, name)) {
      return &e;
    }
  }
  return nullptr;
}

size_t GameFiles::read(const Entry &entry, uint32_t offset, void *dst, size_t size) {
  if (!data_ || offset >= entry.size) {
    return 0;
  }
  size = std::min<size_t>(size, entry.size - offset);
  if (fseek(data_, static_cast<long>(entry.sector) * SECTOR + offset, SEEK_SET) != 0) {
    return 0;
  }
  return fread(dst, 1, size, data_);
}

} // namespace tpm

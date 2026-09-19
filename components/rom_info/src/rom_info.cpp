#include "rom_info.hpp"

#include <algorithm>
#include <cctype>

#include "tab5-emu.hpp"

static std::string trim(const std::string &s) {
  auto b = s.find_first_not_of(" \t\r\n");
  if (b == std::string::npos) return "";
  auto e = s.find_last_not_of(" \t\r\n");
  return s.substr(b, e - b + 1);
}

static bool endsWith(const std::string &s, const std::string &suffix) {
  return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

static std::string lower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
  return s;
}

std::vector<RomInfo> parse_metadata(const std::string &metadata_path) {
  const std::string fs_prefix = std::string(Tab5Emu::mount_point) + "/";
  std::vector<RomInfo> infos;
  std::ifstream metadata(fs_prefix + metadata_path, std::ios::in);
  if (!metadata.is_open()) {
    fmt::print("Couldn't load metadata file {}!\n", metadata_path);
    return infos;
  }
  // csv: rom_path, boxart_path, name (name last since it may contain commas)
  std::string line;
  while (std::getline(metadata, line)) {
    if (line.empty() || line[0] == '#') {
      continue;
    }
    std::string rom_path, boxart_path, name;
    auto first = line.find(',');
    auto second = first == std::string::npos ? std::string::npos : line.find(',', first + 1);
    if (first == std::string::npos || second == std::string::npos) {
      continue;
    }
    rom_path = trim(line.substr(0, first));
    boxart_path = trim(line.substr(first + 1, second - first - 1));
    name = trim(line.substr(second + 1));

    Emulator platform = Emulator::UNKNOWN;
    auto lp = lower(rom_path);
    if (endsWith(lp, ".jk") || endsWith(lp, "jk.cd") || endsWith(lp, "jk1.gob")) {
#if defined(ENABLE_JK)
      platform = Emulator::JEDI_KNIGHT;
#endif
    } else if (endsWith(lp, ".df") || endsWith(lp, "dark.gob")) {
#if defined(ENABLE_DARK_FORCES)
      platform = Emulator::DARK_FORCES;
#endif
    } else if (endsWith(lp, ".gen")) { // sega genesis
#if defined(ENABLE_GENESIS)
      platform = Emulator::SEGA_GENESIS;
#endif
    } else if (endsWith(lp, ".md")) { // sega mega drive
#if defined(ENABLE_GENESIS)
      platform = Emulator::SEGA_MEGA_DRIVE;
#endif
    } else if (endsWith(lp, ".sfc") || endsWith(lp, ".smc")) { // snes
#if defined(ENABLE_SNES)
      platform = Emulator::SNES;
#endif
    } else if (endsWith(lp, ".gba")) { // game boy advance
#if defined(ENABLE_GBA)
      platform = Emulator::GBA;
#endif
    }
    if (platform != Emulator::UNKNOWN) {
      infos.emplace_back(name, fs_prefix + boxart_path, fs_prefix + rom_path, platform);
      fmt::print("{}", infos.back());
    }
  }
  fmt::print("Loaded {} roms from metadata file {}\n", infos.size(), metadata_path);
  return infos;
}

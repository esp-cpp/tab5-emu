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
#if defined(ENABLE_DARKFORCES)
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
    } else if (endsWith(lp, "stage.dir") || endsWith(lp, ".mgs")) { // metal gear solid
#if defined(ENABLE_MGS)
      platform = Emulator::MGS;
#endif
    } else if (endsWith(lp, ".p8") || endsWith(lp, ".p8.png")) { // pico-8
#if defined(ENABLE_PICO8)
      platform = Emulator::PICO8;
#endif
    } else if (endsWith(lp, ".wad")) { // doom
#if defined(ENABLE_DOOM)
      platform = Emulator::DOOM;
#endif
    } else if (endsWith(lp, ".nes")) { // nes
#if defined(ENABLE_NES)
      platform = Emulator::NES;
#endif
    } else if (endsWith(lp, ".gb")) { // game boy
#if defined(ENABLE_GBC)
      platform = Emulator::GAMEBOY;
#endif
    } else if (endsWith(lp, ".gbc")) { // game boy color
#if defined(ENABLE_GBC)
      platform = Emulator::GAMEBOY_COLOR;
#endif
    } else if (endsWith(lp, ".sms")) { // sega master system
#if defined(ENABLE_SMS)
      platform = Emulator::SEGA_MASTER_SYSTEM;
#endif
    } else if (endsWith(lp, ".gg")) { // sega game gear
#if defined(ENABLE_SMS)
      platform = Emulator::SEGA_GAME_GEAR;
#endif
    } else if (endsWith(lp, ".rom")) { // msx
#if defined(ENABLE_MSX)
      platform = Emulator::MSX;
#endif
    } else if (endsWith(lp, ".scummvm")) { // scummvm (star trek: 25th anniversary, ...)
#if defined(ENABLE_SCUMMVM)
      platform = Emulator::SCUMMVM;
#endif
    } else if (endsWith(lp, "slus_008.84")) { // star wars episode i: the phantom menace
#if defined(ENABLE_TPM)
      platform = Emulator::TPM;
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

Console console_of(Emulator platform) {
  switch (platform) {
  case Emulator::NES:
    return Console::NES;
  case Emulator::SNES:
    return Console::SNES;
  case Emulator::GAMEBOY:
    return Console::GAME_BOY;
  case Emulator::GAMEBOY_COLOR:
    return Console::GAME_BOY_COLOR;
  case Emulator::SEGA_MASTER_SYSTEM:
    return Console::MASTER_SYSTEM;
  case Emulator::SEGA_GAME_GEAR:
    return Console::GAME_GEAR;
  case Emulator::MSX:
    return Console::MSX;
  case Emulator::GBA:
    return Console::GAME_BOY_ADVANCE;
  case Emulator::SEGA_GENESIS:
  case Emulator::SEGA_MEGA_DRIVE:
    return Console::GENESIS;
  case Emulator::MGS:
  case Emulator::TPM:
    return Console::PLAYSTATION;
  case Emulator::JEDI_KNIGHT:
  case Emulator::DARK_FORCES:
  case Emulator::DOOM:
  case Emulator::SCUMMVM:
    return Console::PC;
  case Emulator::PICO8:
    return Console::PICO8;
  default:
    return Console::UNKNOWN;
  }
}

const ConsoleInfo &console_info(Console console) {
  static const ConsoleInfo infos[] = {
      {"NES", "NES", "nes", 0xB22222, ConsoleShape::GAMEPAD},
      {"Super Nintendo", "SNES", "snes", 0x6A5ACD, ConsoleShape::GAMEPAD},
      {"Game Boy", "Game Boy", "gb", 0x5B7F3A, ConsoleShape::HANDHELD_TALL},
      {"Game Boy Color", "GB Color", "gbc", 0x8E44AD, ConsoleShape::HANDHELD_TALL},
      {"Game Boy Advance", "GBA", "gba", 0x4B3FA8, ConsoleShape::HANDHELD_WIDE},
      {"Master System", "SMS", "sms", 0x1F5FAF, ConsoleShape::GAMEPAD},
      {"Game Gear", "Game Gear", "gg", 0x4F6D8F, ConsoleShape::HANDHELD_WIDE},
      {"Sega Genesis", "Genesis", "genesis", 0xC8102E, ConsoleShape::GAMEPAD},
      {"MSX", "MSX", "msx", 0x3A7D7D, ConsoleShape::COMPUTER},
      {"PlayStation", "PlayStation", "playstation", 0x2E6DB4, ConsoleShape::DISC},
      {"PC", "PC", "pc", 0x5A6B7B, ConsoleShape::COMPUTER},
      {"PICO-8", "PICO-8", "pico8", 0xFF004D, ConsoleShape::FANTASY},
      {"Other", "Other", "other", 0x606060, ConsoleShape::GAMEPAD},
  };
  const auto i = static_cast<size_t>(console);
  return infos[i < std::size(infos) ? i : std::size(infos) - 1];
}

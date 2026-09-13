#pragma once

#include <fstream>
#include <string>
#include <vector>

#include "format.hpp"


enum class Emulator { UNKNOWN, JEDI_KNIGHT, DARK_FORCES };

struct RomInfo {
  std::string name;
  std::string boxart_path;
  std::string rom_path;
  Emulator platform;
  bool operator==(const RomInfo &) const = default;
};

/// Parse the metadata csv (rom_path, boxart_path, name) relative to the SD
/// card mount point. The platform is picked from the rom path's extension /
/// file name:
///   - "*.jk"  / "jk.cd"  / "JK1.gob" : Jedi Knight (the directory containing
///                                      the file is the game install dir)
///   - "dark.gob" / "*.df"            : Dark Forces
std::vector<RomInfo> parse_metadata(const std::string &metadata_path);

template <> struct fmt::formatter<Emulator> {
  constexpr auto parse(format_parse_context &ctx) const { return ctx.begin(); }
  template <typename FormatContext> auto format(const Emulator &platform, FormatContext &ctx) const {
    switch (platform) {
    case Emulator::JEDI_KNIGHT:
      return fmt::format_to(ctx.out(), "JEDI_KNIGHT");
    case Emulator::DARK_FORCES:
      return fmt::format_to(ctx.out(), "DARK_FORCES");
    default:
      return fmt::format_to(ctx.out(), "UNKNOWN");
    }
  }
};

template <> struct fmt::formatter<RomInfo> {
  constexpr auto parse(format_parse_context &ctx) const { return ctx.begin(); }
  template <typename FormatContext> auto format(const RomInfo &info, FormatContext &ctx) const {
    return fmt::format_to(ctx.out(), "RomInfo {{ name: {}, boxart_path: {}, rom_path: {}, platform: {} }}\n",
                          info.name, info.boxart_path, info.rom_path, info.platform);
  }
};

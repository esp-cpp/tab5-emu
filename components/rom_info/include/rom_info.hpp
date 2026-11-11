#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#include "format.hpp"


enum class Emulator { UNKNOWN, JEDI_KNIGHT, DARK_FORCES, SEGA_GENESIS, SEGA_MEGA_DRIVE, SNES, GBA, MGS, TPM, PICO8 };

struct RomInfo {
  std::string name;
  std::string boxart_path;
  std::string rom_path;
  Emulator platform;
  bool operator==(const RomInfo &) const = default;
};

/// The console a game is played as, in the order the game list shows them
/// (several emulators can share one, e.g. Genesis / Mega Drive; the PC and
/// PlayStation ports count as the console the game came out on).
enum class Console : uint8_t { SNES, GAME_BOY_ADVANCE, GENESIS, PLAYSTATION, PC, PICO8, UNKNOWN };

/// The shape the firmware draws for a console that has no art on the SD card.
enum class ConsoleShape : uint8_t { GAMEPAD, HANDHELD_TALL, HANDHELD_WIDE, DISC, COMPUTER, FANTASY };

struct ConsoleInfo {
  const char *name;       ///< shown in the list header and the badge
  const char *short_name; ///< for a small screen's badge
  const char *key;        ///< art on the SD card: boxart/consoles/<key>.jpg
  uint32_t color;     ///< badge / header accent (0xRRGGBB)
  ConsoleShape shape;
};

/// The console `platform` games are grouped under.
Console console_of(Emulator platform);
/// Name, art key and colors of `console`.
const ConsoleInfo &console_info(Console console);

/// Parse the metadata csv (rom_path, boxart_path, name) relative to the SD
/// card mount point. The platform is picked from the rom path's extension /
/// file name:
///   - "*.jk"  / "jk.cd"  / "JK1.gob" : Jedi Knight (the directory containing
///                                      the file is the game install dir)
///   - "dark.gob" / "*.df"            : Dark Forces
///   - "STAGE.DIR" / "*.mgs"          : Metal Gear Solid (the directory containing
///                                      the file holds the extracted disc files)
///   - "*.p8" / "*.p8.png"            : PICO-8 cart
std::vector<RomInfo> parse_metadata(const std::string &metadata_path);

template <> struct fmt::formatter<Emulator> {
  constexpr auto parse(format_parse_context &ctx) const { return ctx.begin(); }
  template <typename FormatContext> auto format(const Emulator &platform, FormatContext &ctx) const {
    switch (platform) {
    case Emulator::JEDI_KNIGHT:
      return fmt::format_to(ctx.out(), "JEDI_KNIGHT");
    case Emulator::SEGA_GENESIS:
      return fmt::format_to(ctx.out(), "SEGA_GENESIS");
    case Emulator::SEGA_MEGA_DRIVE:
      return fmt::format_to(ctx.out(), "SEGA_MEGA_DRIVE");
    case Emulator::SNES:
      return fmt::format_to(ctx.out(), "SNES");
    case Emulator::GBA:
      return fmt::format_to(ctx.out(), "GBA");
    case Emulator::DARK_FORCES:
      return fmt::format_to(ctx.out(), "DARK_FORCES");
    case Emulator::MGS:
      return fmt::format_to(ctx.out(), "MGS");
    case Emulator::TPM:
      return fmt::format_to(ctx.out(), "TPM");
    case Emulator::PICO8:
      return fmt::format_to(ctx.out(), "PICO8");
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

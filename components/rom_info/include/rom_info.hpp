#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#include "format.hpp"


enum class Emulator {
  UNKNOWN, JEDI_KNIGHT, DARK_FORCES, SEGA_GENESIS, SEGA_MEGA_DRIVE, SNES, GBA, MGS, TPM, PICO8,
  NES, GAMEBOY, GAMEBOY_COLOR, SEGA_MASTER_SYSTEM, SEGA_GAME_GEAR, MSX, DOOM, SCUMMVM
};

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
enum class Console : uint8_t {
  NES, SNES, GAME_BOY, GAME_BOY_COLOR, GAME_BOY_ADVANCE, MASTER_SYSTEM, GAME_GEAR, GENESIS, MSX, PLAYSTATION, PC, PICO8,
  UNKNOWN
};

struct ConsoleInfo {
  const char *name;       ///< shown in the list header and the badge
  const char *short_name; ///< for a small screen's badge
  const char *key;        ///< art on the SD card (boxart/consoles/<key>.jpg) and
                          ///< the launcher's look (emu_ui::console_style)
};

/// The console `platform` games are grouped under.
Console console_of(Emulator platform);
/// Name and key of `console`.
const ConsoleInfo &console_info(Console console);

/// Parse the metadata csv (rom_path, boxart_path, name) relative to the SD
/// card mount point. The platform is picked from the rom path's extension /
/// file name:
///   - "*.jk"  / "jk.cd"  / "JK1.gob" : Jedi Knight (the directory containing
///                                      the file is the game install dir)
///   - "*.jkm" / "jkmots.cd"          : Jedi Knight: Mysteries of the Sith
///                                      (likewise; the engine runs it with
///                                      -motsCompat)
///   - "dark.gob" / "*.df"            : Dark Forces (the directory containing
///                                      the file holds the game's GOBs)
///   - "*.wad"                        : Doom (likewise, for the IWAD)
///   - "*.nes"                        : NES
///   - "*.gb" / "*.gbc"               : Game Boy / Game Boy Color
///   - "*.sms" / "*.gg"               : Master System / Game Gear
///   - "*.rom"                        : MSX
///   - "*.scummvm"                    : a ScummVM game (a marker file: the
///                                      directory containing it holds the
///                                      game's files; ScummVM detects which)
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
    case Emulator::NES:
      return fmt::format_to(ctx.out(), "NES");
    case Emulator::GAMEBOY:
      return fmt::format_to(ctx.out(), "GAMEBOY");
    case Emulator::GAMEBOY_COLOR:
      return fmt::format_to(ctx.out(), "GAMEBOY_COLOR");
    case Emulator::SEGA_MASTER_SYSTEM:
      return fmt::format_to(ctx.out(), "SEGA_MASTER_SYSTEM");
    case Emulator::SEGA_GAME_GEAR:
      return fmt::format_to(ctx.out(), "SEGA_GAME_GEAR");
    case Emulator::MSX:
      return fmt::format_to(ctx.out(), "MSX");
    case Emulator::DOOM:
      return fmt::format_to(ctx.out(), "DOOM");
    case Emulator::SCUMMVM:
      return fmt::format_to(ctx.out(), "SCUMMVM");
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

#pragma once

#include <memory>

#include "darkforces_cart.hpp"
#include "doom_cart.hpp"
#include "gba_cart.hpp"
#include "gbc_cart.hpp"
#include "genesis_cart.hpp"
#include "jk_cart.hpp"
#include "mgs_cart.hpp"
#include "msx_cart.hpp"
#include "nes_cart.hpp"
#include "pico8_cart.hpp"
#include "sms_cart.hpp"
#include "snes_cart.hpp"
#include "tpm_cart.hpp"

inline std::unique_ptr<Cart> make_cart(const RomInfo &info) {
#if defined(ENABLE_MGS)
  if (info.platform != Emulator::MGS) {
    mgs::release_arena(); // its 3.8 MB of PSRAM: Jedi Knight needs the room
  }
#endif
#if defined(ENABLE_TPM)
  if (info.platform != Emulator::TPM) {
    tpm::release_arena(); // its ~5 MB, likewise
  }
#endif
#if CONFIG_TAB5_EMU_AUTOSTART_DELAY_MS > 0
  fmt::print("make_cart: psram free {} (largest {}), internal free {} (largest {})\n",
             heap_caps_get_free_size(MALLOC_CAP_SPIRAM), heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM),
             heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
             heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
#endif
  switch (info.platform) {
  case Emulator::JEDI_KNIGHT:
    return std::make_unique<JkCart>(Cart::Config{
        .info = info, .copy_romdata = false, .verbosity = espp::Logger::Verbosity::INFO});
  case Emulator::MGS:
    return std::make_unique<MgsCart>(Cart::Config{
        .info = info, .copy_romdata = false, .verbosity = espp::Logger::Verbosity::INFO});
  case Emulator::TPM:
    return std::make_unique<TpmCart>(Cart::Config{
        .info = info, .copy_romdata = false, .verbosity = espp::Logger::Verbosity::INFO});
  case Emulator::GBA:
    return std::make_unique<GbaCart>(Cart::Config{
        .info = info, .copy_romdata = false, .verbosity = espp::Logger::Verbosity::INFO});
  case Emulator::SNES:
    return std::make_unique<SnesCart>(Cart::Config{
        .info = info, .copy_romdata = false, .verbosity = espp::Logger::Verbosity::INFO}); // loads into its ROM arena
  case Emulator::SEGA_GENESIS:
  case Emulator::SEGA_MEGA_DRIVE:
    return std::make_unique<GenesisCart>(Cart::Config{
        .info = info, .copy_romdata = true, .verbosity = espp::Logger::Verbosity::INFO});
  case Emulator::PICO8:
    return std::make_unique<Pico8Cart>(Cart::Config{
        .info = info, .copy_romdata = true, .verbosity = espp::Logger::Verbosity::INFO});
  case Emulator::NES:
    return std::make_unique<NesCart>(Cart::Config{
        .info = info, .copy_romdata = true, .verbosity = espp::Logger::Verbosity::INFO});
  case Emulator::GAMEBOY:
  case Emulator::GAMEBOY_COLOR:
    return std::make_unique<GbcCart>(Cart::Config{
        .info = info, .copy_romdata = true, .verbosity = espp::Logger::Verbosity::INFO});
  case Emulator::SEGA_MASTER_SYSTEM:
  case Emulator::SEGA_GAME_GEAR:
    return std::make_unique<SmsCart>(Cart::Config{
        .info = info, .copy_romdata = true, .verbosity = espp::Logger::Verbosity::INFO});
  case Emulator::MSX:
    return std::make_unique<MsxCart>(Cart::Config{
        .info = info, .copy_romdata = false, .verbosity = espp::Logger::Verbosity::INFO});
  case Emulator::DOOM:
    return std::make_unique<DoomCart>(Cart::Config{
        .info = info, .copy_romdata = false, .verbosity = espp::Logger::Verbosity::INFO});
  case Emulator::DARK_FORCES:
    return std::make_unique<DarkForcesCart>(Cart::Config{
        .info = info, .copy_romdata = false, .verbosity = espp::Logger::Verbosity::INFO});
  default:
    return nullptr;
  }
}

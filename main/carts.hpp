#pragma once

#include <memory>

#include "gba_cart.hpp"
#include "genesis_cart.hpp"
#include "jk_cart.hpp"
#include "mgs_cart.hpp"
#include "snes_cart.hpp"

inline std::unique_ptr<Cart> make_cart(const RomInfo &info) {
  switch (info.platform) {
  case Emulator::JEDI_KNIGHT:
    return std::make_unique<JkCart>(Cart::Config{
        .info = info, .copy_romdata = false, .verbosity = espp::Logger::Verbosity::INFO});
  case Emulator::MGS:
    return std::make_unique<MgsCart>(Cart::Config{
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
  default:
    return nullptr;
  }
}

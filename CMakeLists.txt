# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.20)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set(EXTRA_COMPONENT_DIRS
  "components"
)

add_compile_definitions(BOARD_HAS_PSRAM)

# ESP-IDF 6.1 added fields to some esp_lcd callback structs; the espp BSP from
# the component registry designated-initializes them and would otherwise fail
# under -Werror=missing-field-initializers.
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=missing-field-initializers" APPEND)

### Jedi Knight: Dark Forces II (OpenJKDF2) ###
set(JK_COMPONENTS "jk")
### GENESIS (gwenesis, shared with esp-box-emu) ###
set(GENESIS_COMPONENTS genesis shared_memory)
### SNES (snes9x2005) ###
set(SNES_COMPONENTS snes)
### GBA (gpSP) ###
set(GBA_COMPONENTS gba)

### Dark Forces (The Force Engine port, shared with esp-box-emu) ###
# set(DARK_FORCES_COMPONENTS "dark_forces")

if(JK_COMPONENTS)
  add_compile_definitions(ENABLE_JK)
endif()
if(GENESIS_COMPONENTS)
  add_compile_definitions(ENABLE_GENESIS)
endif()
if(SNES_COMPONENTS)
  add_compile_definitions(ENABLE_SNES)
endif()
if(GBA_COMPONENTS)
  add_compile_definitions(ENABLE_GBA)
endif()

if(DARK_FORCES_COMPONENTS)
  add_compile_definitions(ENABLE_DARK_FORCES)
endif()

set(GAME_COMPONENTS
  ${JK_COMPONENTS}
  ${GENESIS_COMPONENTS}
  ${SNES_COMPONENTS}
  ${GBA_COMPONENTS}
  ${DARK_FORCES_COMPONENTS}
  )

set(
  COMPONENTS
  main esptool_py esp_lcd esp_psram task format display display_drivers monitor timer
  ${GAME_COMPONENTS} tab5-emu gui menu rom_info statistics
  CACHE STRING
  "List of components to include"
  FORCE
  )

project(tab5-emu)

set(CMAKE_CXX_STANDARD 23)

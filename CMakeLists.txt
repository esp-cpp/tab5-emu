# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.20)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# TEMPORARY: until espp ships the Tab5 SD-card fix (the SD IO rail must be
# switched on through the P4's LDO channel 4; espp PR "fix(m5stack-tab5): power
# the SD card IO rail"), use a local espp checkout for the Tab5 BSP when one
# is present. Set ESPP_PATH to point elsewhere; CI has no local espp and keeps
# using the component registry. Remove this block (and the .yml.in template)
# once the released espp/m5stack-tab5 contains the fix.
set(ESPP_PATH "$ENV{ESPP_PATH}")
if(NOT ESPP_PATH)
  set(ESPP_PATH "${CMAKE_CURRENT_LIST_DIR}/../espp")
endif()
set(TAB5_BSP_OVERRIDE "")
if(EXISTS "${ESPP_PATH}/components/m5stack-tab5/src/sdcard.cpp")
  file(READ "${ESPP_PATH}/components/m5stack-tab5/src/sdcard.cpp" _tab5_sdcard)
  if(_tab5_sdcard MATCHES "sd_pwr_ctrl_new_on_chip_ldo")
    message(STATUS "tab5-emu: using local espp m5stack-tab5 BSP from ${ESPP_PATH}")
    set(TAB5_BSP_OVERRIDE "\n    override_path: '${ESPP_PATH}/components/m5stack-tab5'")
  endif()
endif()
configure_file("${CMAKE_CURRENT_LIST_DIR}/components/tab5-emu/idf_component.yml.in"
               "${CMAKE_CURRENT_LIST_DIR}/components/tab5-emu/idf_component.yml" @ONLY)

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

### Dark Forces (The Force Engine port, shared with esp-box-emu) ###
# set(DARK_FORCES_COMPONENTS "dark_forces")

if(JK_COMPONENTS)
  add_compile_definitions(ENABLE_JK)
endif()

if(DARK_FORCES_COMPONENTS)
  add_compile_definitions(ENABLE_DARK_FORCES)
endif()

set(GAME_COMPONENTS
  ${JK_COMPONENTS}
  ${DARK_FORCES_COMPONENTS}
  )

set(
  COMPONENTS
  "main esptool_py esp_lcd esp_psram task format display display_drivers monitor timer ${GAME_COMPONENTS} tab5-emu gui menu rom_info statistics"
  CACHE STRING
  "List of components to include"
  )

project(tab5-emu)

set(CMAKE_CXX_STANDARD 23)

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
### Metal Gear Solid (mgs_reversing decompilation on psyz, native) ###
set(MGS_COMPONENTS mgs)
### Star Wars Episode I: The Phantom Menace (tpm-decomp on psyz, native) ###
set(TPM_COMPONENTS tpm)
### PICO-8 (femto8, shared with esp-box-emu) ###
set(PICO8_COMPONENTS pico8)

### Cores shared with esp-box-emu (each core + its port/ glue a submodule) ###
### NES (nofrendo) ###
set(NES_COMPONENTS nes)
### Game Boy / Color (gnuboy) ###
set(GBC_COMPONENTS gbc)
### Master System / Game Gear (SMS Plus) ###
set(SMS_COMPONENTS sms)
### MSX (fMSX) ###
set(MSX_COMPONENTS msx)
### Doom (PrBoom) ###
set(DOOM_COMPONENTS doom)
### Dark Forces (The Force Engine) ###
set(DARK_FORCES_COMPONENTS darkforces)
### ScummVM (esp-cpp/scummvm: Star Trek: 25th Anniversary / Judgment Rites) ###
set(SCUMMVM_COMPONENTS scummvm)

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
if(MGS_COMPONENTS)
  add_compile_definitions(ENABLE_MGS)
endif()
if(TPM_COMPONENTS)
  add_compile_definitions(ENABLE_TPM)
endif()
if(PICO8_COMPONENTS)
  add_compile_definitions(ENABLE_PICO8)
endif()

if(NES_COMPONENTS)
  add_compile_definitions(ENABLE_NES)
endif()
if(GBC_COMPONENTS)
  add_compile_definitions(ENABLE_GBC)
endif()
if(SMS_COMPONENTS)
  add_compile_definitions(ENABLE_SMS)
endif()
if(MSX_COMPONENTS)
  add_compile_definitions(ENABLE_MSX)
endif()
if(DOOM_COMPONENTS)
  add_compile_definitions(ENABLE_DOOM)
endif()
if(DARK_FORCES_COMPONENTS)
  add_compile_definitions(ENABLE_DARKFORCES)
endif()
if(SCUMMVM_COMPONENTS)
  add_compile_definitions(ENABLE_SCUMMVM)
endif()

set(GAME_COMPONENTS
  ${JK_COMPONENTS}
  ${GENESIS_COMPONENTS}
  ${SNES_COMPONENTS}
  ${GBA_COMPONENTS}
  ${MGS_COMPONENTS}
  ${TPM_COMPONENTS}
  ${PICO8_COMPONENTS}
  ${NES_COMPONENTS}
  ${GBC_COMPONENTS}
  ${SMS_COMPONENTS}
  ${MSX_COMPONENTS}
  ${DOOM_COMPONENTS}
  ${DARK_FORCES_COMPONENTS}
  ${SCUMMVM_COMPONENTS}
  )

set(
  COMPONENTS
  main esptool_py esp_lcd esp_psram task format display display_drivers monitor timer
  ${GAME_COMPONENTS} tab5-emu pool_allocator gui menu rom_info statistics settings emulink
  CACHE STRING
  "List of components to include"
  FORCE
  )

# the USB link (components/emulink) builds against this board
idf_build_set_property(EMULINK_BOARD_COMPONENT tab5-emu)

project(tab5-emu)

set(CMAKE_CXX_STANDARD 23)

# Executing from flash, .ext_ram.data (components/tab5-emu/ext_ram_data.ld)
# is PSRAM, which the image cannot hold: after linking, its bytes go into the
# flash array tab5_ext_ram_data_image and the section becomes NOBITS, before
# the .bin is made from the ELF
if(NOT CONFIG_SPIRAM_XIP_FROM_PSRAM)
  idf_build_get_property(python PYTHON)
  add_custom_command(TARGET ${CMAKE_PROJECT_NAME}.elf POST_BUILD
    COMMAND ${python} ${CMAKE_CURRENT_LIST_DIR}/tools/ext_ram_data_image.py $<TARGET_FILE:${CMAKE_PROJECT_NAME}.elf>
    # likewise the game cores' code overlay (components/tab5-emu/overlay.ld),
    # and check that only the carts reach into it
    COMMAND ${python} ${CMAKE_CURRENT_LIST_DIR}/tools/overlay_image.py $<TARGET_FILE:${CMAKE_PROJECT_NAME}.elf>
            ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.map
    COMMENT "Storing .ext_ram.data and the core code overlay in flash"
    VERBATIM)
endif()

# Executing from flash, LVGL and the GUI (they draw in software: from flash
# the launcher redraws visibly) run from PSRAM instead: their code and
# read-only data go to .tab5_res (tools/bss_to_psram.py --psram-code,
# components/tab5-emu/overlay.ld), filled from flash at boot. Done on the
# archives just before the link (a POST_BUILD step can only be added in the
# component's own directory, and LVGL is a managed component); renaming an
# archive whose sections are renamed already does nothing.
if(NOT CONFIG_SPIRAM_XIP_FROM_PSRAM)
  foreach(component lvgl__lvgl emu-ui gui menu)
    idf_component_get_property(lib ${component} COMPONENT_LIB)
    add_custom_command(TARGET ${CMAKE_PROJECT_NAME}.elf PRE_LINK
      COMMAND ${python} ${CMAKE_CURRENT_LIST_DIR}/tools/bss_to_psram.py
              ${CMAKE_OBJDUMP} ${CMAKE_OBJCOPY} $<TARGET_FILE:${lib}> --no-bss --psram-code
      COMMENT "Moving ${component}'s code to PSRAM"
      VERBATIM)
  endforeach()
endif()

# The updater app (updater/) belongs in ota_1 (see partitions.csv): build it
# first (idf.py -C updater build) and `idf.py flash`, flash_args and
# flasher_args.json (the release package) put it there with the emulator.
set(UPDATER_BIN "${CMAKE_CURRENT_LIST_DIR}/updater/build/tab5-emu-updater.bin")
if(EXISTS "${UPDATER_BIN}")
  # (a copy in the build dir: flash_args lists it next to the others, and a
  # rebuilt updater reconfigures this)
  configure_file("${UPDATER_BIN}" "${CMAKE_BINARY_DIR}/updater/tab5-emu-updater.bin" COPYONLY)
  partition_table_get_partition_info(updater_offset "--partition-type app --partition-subtype ota_1" "offset")
  esptool_py_flash_target_image(flash updater "${updater_offset}" "${CMAKE_BINARY_DIR}/updater/tab5-emu-updater.bin")
else()
  message(WARNING "no updater image (${UPDATER_BIN}): build it with idf.py -C updater build, then reconfigure, "
                  "or OTA updates and rollback have no updater to boot")
endif()

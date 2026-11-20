# Game Boy / Color: gnuboy, from the esp-cpp/gnuboy submodule (the core in
# gnuboy/gnuboy, its glue in gnuboy/port)
idf_component_register(
  INCLUDE_DIRS "gnuboy/port/include"
  SRC_DIRS "gnuboy/port" "gnuboy/gnuboy/src"
  PRIV_INCLUDE_DIRS "gnuboy/gnuboy/include"
  REQUIRES "tab5-emu" "statistics" "shared_memory"
  )

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-misleading-indentation -Wno-implicit-fallthrough -Wno-unused-function -Wno-unused-variable $<$<COMPILE_LANGUAGE:C>:-Wno-discarded-qualifiers>)
target_compile_definitions(${COMPONENT_LIB} PRIVATE GNUBOY_NO_MINIZIP GNUBOY_NO_SCREENSHOT IS_LITTLE_ENDIAN)
# the core's static state (.bss and .data) lives in PSRAM (see the post-build step; the P4
# linker script has no ldgen placeholder for external RAM); no gp-relative
# small data then
target_compile_options(${COMPONENT_LIB} PRIVATE -msmall-data-limit=0)
# executing from flash, the core's code is part of the PSRAM code overlay
# (components/tab5-emu/overlay.ld)
include(${CMAKE_CURRENT_LIST_DIR}/../../tools/overlay.cmake)
tab5_overlay_args(gbc overlay_args)
add_custom_command(TARGET ${COMPONENT_LIB} POST_BUILD
  COMMAND ${PYTHON} ${CMAKE_CURRENT_LIST_DIR}/../../tools/bss_to_psram.py
          ${CMAKE_OBJDUMP} ${CMAKE_OBJCOPY} $<TARGET_FILE:${COMPONENT_LIB}> ${overlay_args}
          # .data to PSRAM: via .rodata where XIP copies it into PSRAM (writable),
          # else into .ext_ram.data (filled at boot from flash; see the script)
          --data --data-into=$<IF:$<BOOL:${CONFIG_SPIRAM_XIP_FROM_PSRAM}>,rodata,ext_ram>
  COMMENT "Moving the gbc core's .bss and .data to PSRAM"
  VERBATIM)
# globals other cores in this firmware define too (the box doesn't build
# those): renamed in this library, its glue included
add_custom_command(TARGET ${COMPONENT_LIB} POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} --redefine-syms=${CMAKE_CURRENT_LIST_DIR}/symbol_renames.syms $<TARGET_FILE:${COMPONENT_LIB}>
  COMMENT "Renaming the globals another core also defines"
  VERBATIM)

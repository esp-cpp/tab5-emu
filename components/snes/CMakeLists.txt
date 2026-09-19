# SNES: snes9x2005 (libretro fork of Snes9x 1.43 / CATSFC), driven without
# libretro by src/snes_glue.c + src/snes.cpp
set(SNES_CORE snes9x2005/source)
file(GLOB SNES_CORE_SRCS ${CMAKE_CURRENT_LIST_DIR}/${SNES_CORE}/*.c)
# files that are #included by others, and the non-blargg APU
list(FILTER SNES_CORE_SRCS EXCLUDE REGEX "(dsp1emu|dsp2emu|dsp4emu|apu|soundux|spc700)\\.c$")
idf_component_register(
  SRCS ${SNES_CORE_SRCS} "src/snes_glue.c" "src/snes.cpp"
  INCLUDE_DIRS "include"
  PRIV_INCLUDE_DIRS "${SNES_CORE}" "snes9x2005/libretro-common/include" "src"
  REQUIRES tab5-emu statistics
)
target_compile_definitions(${COMPONENT_LIB} PRIVATE USE_BLARGG_APU LOAD_FROM_MEMORY LAGFIX __LIBRETRO__)
# the core's ~700 KB of static state lives in PSRAM (its .bss is renamed to the
# PSRAM section after the archive is built); no gp-relative small data then
target_compile_options(${COMPONENT_LIB} PRIVATE -O2 -fno-strict-aliasing -msmall-data-limit=0
  -Wno-error -w -Wno-error=incompatible-pointer-types -Wno-error=int-conversion -Wno-error=implicit-function-declaration -Wno-error=discarded-qualifiers)
add_custom_command(TARGET ${COMPONENT_LIB} POST_BUILD
  COMMAND ${PYTHON} ${CMAKE_CURRENT_LIST_DIR}/../../tools/bss_to_psram.py
          ${CMAKE_OBJDUMP} ${CMAKE_OBJCOPY} $<TARGET_FILE:${COMPONENT_LIB}>
  COMMENT "Moving the SNES core's .bss to PSRAM"
  VERBATIM)

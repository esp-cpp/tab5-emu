# GBA: gpSP (interpreter build; no dynarec on RISC-V), driven through its
# own libretro front end by a minimal libretro host in src/gba.cpp
set(GPSP gpsp)
set(GPSP_COMMON ${GPSP}/libretro/libretro-common)
idf_component_register(
  SRCS
    ${GPSP}/main.c ${GPSP}/gba_memory.c ${GPSP}/savestate.c ${GPSP}/input.c ${GPSP}/sound.c
    ${GPSP}/cheats.c ${GPSP}/memmap.c ${GPSP}/serial.c ${GPSP}/gbp.c ${GPSP}/rfu.c
    ${GPSP}/serial_proto.c ${GPSP}/libretro/libretro.c ${GPSP}/gba_cc_lut.c
    ${GPSP}/video.cc ${GPSP}/cpu.cc ${GPSP}/open_gba_bios.c
    ${GPSP_COMMON}/compat/compat_posix_string.c ${GPSP_COMMON}/compat/compat_strl.c
    ${GPSP_COMMON}/compat/fopen_utf8.c ${GPSP_COMMON}/encodings/encoding_utf.c
    ${GPSP_COMMON}/file/file_path.c ${GPSP_COMMON}/file/file_path_io.c
    ${GPSP_COMMON}/streams/file_stream.c ${GPSP_COMMON}/string/stdstring.c
    ${GPSP_COMMON}/time/rtime.c ${GPSP_COMMON}/vfs/vfs_implementation.c
    "src/gba.cpp"
  INCLUDE_DIRS "include"
  PRIV_INCLUDE_DIRS "${GPSP}" "${GPSP}/libretro" "${GPSP_COMMON}/include" "src"
  REQUIRES tab5-emu statistics
)
# ROM cache: 1 MB blocks in PSRAM (larger ROMs page in from the SD card)
target_compile_definitions(${COMPONENT_LIB} PRIVATE __LIBRETRO__ ROM_BUFFER_SIZE=12)
# the core's static state (backup RAM, VRAM, ...) lives in PSRAM (see the
# post-build step); no gp-relative small data then
target_compile_options(${COMPONENT_LIB} PRIVATE -O2 -fno-strict-aliasing -msmall-data-limit=0 -include ${CMAKE_CURRENT_LIST_DIR}/gpsp/retro_inline.h
  -Wno-error -w -Wno-error=incompatible-pointer-types -Wno-error=int-conversion
  -Wno-error=implicit-function-declaration -Wno-error=discarded-qualifiers)
add_custom_command(TARGET ${COMPONENT_LIB} POST_BUILD
  COMMAND ${PYTHON} ${CMAKE_CURRENT_LIST_DIR}/../../tools/bss_to_psram.py
          ${CMAKE_OBJDUMP} ${CMAKE_OBJCOPY} $<TARGET_FILE:${COMPONENT_LIB}>
  COMMENT "Moving the GBA core's .bss to PSRAM"
  VERBATIM)

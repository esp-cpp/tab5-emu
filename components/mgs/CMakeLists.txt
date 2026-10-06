# Metal Gear Solid, natively: the FoxdieTeam decompilation (davidmonterocrespo24's
# esp32-port branch) compiled for the ESP32-P4 against psyz, Xeeynamo's
# PlayStation SDK reimplementation with a software GPU. Mirrors the upstream
# esp32/main/CMakeLists.txt; the Tab5 platform layer (display, pads, data,
# lifecycle) is in src/, the PSY-Q pad header the upstream build took from a
# private shim directory is in shim/.

set(MGS_ROOT ${CMAKE_CURRENT_LIST_DIR}/mgs_reversing)
set(PSYZ ${CMAKE_CURRENT_LIST_DIR}/psyz/psyz)
set(PSYQ ${CMAKE_CURRENT_LIST_DIR}/psyz/decomp)

set(MGS_SRCS "src/mgs.cpp" "src/mgs_platform.cpp")
set(MGS_INCLUDE_DIRS "include")
set(MGS_PRIV_INCLUDE_DIRS "src" "shim")

if(CONFIG_MGS_ENGINE)
  # ---- the game ----------------------------------------------------------
  file(GLOB_RECURSE SRC_GAME ${MGS_ROOT}/source/*.c)
  list(FILTER SRC_GAME EXCLUDE REGEX "/overlays/")
  list(FILTER SRC_GAME EXCLUDE REGEX "/contrib/")
  list(FILTER SRC_GAME EXCLUDE REGEX "/stagevr/")
  list(FILTER SRC_GAME EXCLUDE REGEX "/snake_vr/")
  list(FILTER SRC_GAME EXCLUDE REGEX "/stage/")
  list(APPEND SRC_GAME
    ${MGS_ROOT}/source/stage/_stage.c
    ${MGS_ROOT}/port/stage_union.c) # one merged actor table for all packed stages

  # ---- the overlays that are already 100% C ------------------------------
  foreach(ovl s07a s11e s11i d18ar s08br s19br)
    file(GLOB OVL_SRC ${MGS_ROOT}/source/overlays/${ovl}/*.c)
    list(APPEND SRC_OVERLAYS ${OVL_SRC})
  endforeach()

  # ---- psyz, minus the pieces MGS supplies itself ------------------------
  file(GLOB SRC_PSYZ ${PSYZ}/src/psyz/*.c)
  list(FILTER SRC_PSYZ EXCLUDE REGEX "libgte_hw_sqrt")
  list(FILTER SRC_PSYZ EXCLUDE REGEX "libsn") # MGS has PC* in libfs/select.c

  # ---- the PSY-Q decomp libraries ----------------------------------------
  # The curated list the SOTN port proved builds (psyq_sources.cmake), plus
  # the libspu / libpress units MGS needs on top of it.
  include(${CMAKE_CURRENT_LIST_DIR}/psyq_sources.cmake)
  set(SRC_PSYQ ${GAME_SOURCE_FILES_PSYQ})
  foreach(extra
      libspu/s_ini.c libspu/s_crwa.c libspu/s_gva.c
      libspu/s_i.c   libspu/s_ih.c   libspu/s_snv.c
      libpress/dec.c)
    if(EXISTS ${PSYQ}/src/${extra})
      list(APPEND SRC_PSYQ ${PSYQ}/src/${extra})
    endif()
  endforeach()
  list(REMOVE_DUPLICATES SRC_PSYQ)

  # ---- the upstream port layer we keep as-is -----------------------------
  # (threads, vblank, virtual CD, GTE/matrix compat, software GPU binding,
  # headless hooks); the board-specific lcd / input / sd files are replaced by
  # src/mgs_platform.cpp.
  set(SRC_PORT
    ${MGS_ROOT}/port/psyz_port.c
    ${MGS_ROOT}/port/psyq_compat.c
    ${MGS_ROOT}/port/soft_render.c
    ${MGS_ROOT}/port/platform_headless.c
    ${MGS_ROOT}/port/stage_stubs.c
    ${MGS_ROOT}/port/spu_compat.c
    ${MGS_ROOT}/port/esp32_threads.c
    ${MGS_ROOT}/port/esp32_vblank.c
    ${MGS_ROOT}/port/virtual_cd.c)

  list(APPEND MGS_SRCS ${SRC_GAME} ${SRC_OVERLAYS} ${SRC_PORT} ${SRC_PSYZ} ${SRC_PSYQ})
  list(APPEND MGS_PRIV_INCLUDE_DIRS
    ${MGS_ROOT}/source # MUST come before psyz's: both ship a common.h
    ${MGS_ROOT}/source/include
    # not port/include: its libpress.h / libsn.h would shadow psyz's own for
    # psyz's sources; its gtemac.h (the one thing the game needs from it) is
    # copied into shim/
    ${PSYZ}/include
    ${PSYZ}/src)
endif()

idf_component_register(
  SRCS ${MGS_SRCS}
  INCLUDE_DIRS ${MGS_INCLUDE_DIRS}
  PRIV_INCLUDE_DIRS ${MGS_PRIV_INCLUDE_DIRS}
  REQUIRES tab5-emu
  PRIV_REQUIRES esp_timer esp_system heap statistics
  LDFRAGMENTS "linker.lf"
)

if(CONFIG_MGS_ENGINE)
  # The game is 1998 PSY-Q C. The flags are the upstream build's, minus
  # -mlongcalls (an Xtensa concern; the RISC-V toolchain reaches the whole
  # image). They are set per source on the C files only: the platform layer
  # is C++ and must not see them.
  set(MGS_C_SRCS ${SRC_GAME} ${SRC_OVERLAYS} ${SRC_PORT} ${SRC_PSYZ} ${SRC_PSYQ})
  set_source_files_properties(${MGS_C_SRCS} PROPERTIES
    COMPILE_OPTIONS
      "-std=gnu17;-w;-fno-strict-aliasing;-msmall-data-limit=0;-fjump-tables;-ftree-switch-conversion;-Wno-error;-Wno-incompatible-pointer-types;-Wno-int-conversion;-Wno-implicit-function-declaration;-Wno-builtin-declaration-mismatch;-Wno-return-mismatch;-include;string.h;-include;gtemac.h;-include;${MGS_ROOT}/port/mgs_printf.h"
    COMPILE_DEFINITIONS
      "INTEGRAL;__psyz;VERSION_PC;NON_MATCHING;PERMUTER;HARD_LINK;_internal_version_us;MGS_START_STAGE=\"${CONFIG_MGS_START_STAGE}\";MGS_THREAD_STACK_CAPS=MALLOC_CAP_SPIRAM;MGS_CODEC_SCREEN=1")
  #   -std=gnu17            gnu23 (GCC 14+'s default) rejects this code
  #   -fno-strict-aliasing  type punning everywhere: GCC at -O2 would delete
  #                         stores it "proves" dead (that is how CompMatrix
  #                         lost its rotation upstream)
  #   -Wno-...              int/long and pointer types mixed freely: the same
  #                         width on MIPS
  #   MGS_THREAD_STACK_CAPS mts task stacks in PSRAM, not internal RAM (the
  #                         Tab5's scarce memory; see src/mgs.cpp)
  #   MGS_CODEC_SCREEN=1    the real codec module draws the calls (the S3
  #                         port answered them headless)

  # The PSY-Q decomp sources were written against psyz's common.h (the stub
  # macros for not-yet-decompiled functions, LEN(), STATIC_ASSERT via psyz.h),
  # but the include order resolves <common.h> to the game's copy for them, as
  # it must for the game. Pre-include psyz's; the guards differ, so the game's
  # still follows. libgpu/sys.c also takes printf's address without declaring
  # it, so <stdio.h> comes along -- here only, since the game's libsio/dummy.c
  # defines its own getchar() and cannot see it.
  set_property(SOURCE ${SRC_PSYQ} APPEND PROPERTY COMPILE_OPTIONS
    "-include;${PSYZ}/include/common.h;-include;stdio.h")

  # Where the game's statics go (PSRAM for .bss, the PSRAM-XIP .rodata for
  # .data) and the symbols that bracket them are linker.lf's business;
  # -msmall-data-limit=0 above because nothing in PSRAM may be addressed
  # gp-relative.

  # The rasterizer's per-pixel loops run from PSRAM-XIP through the cache
  # rather than from IRAM: internal RAM is what the boot-time DMA reserve and
  # the HAL need, and the S3 port measured IRAM placement at -2%.
  set_property(SOURCE ${MGS_ROOT}/port/soft_render.c APPEND PROPERTY
    COMPILE_DEFINITIONS "SOFT_RASTER_IRAM=")
  # ...and it is the one file whose inner loops decide the frame rate: -O3
  # and no frame pointer (the IDF default keeps one for backtraces), which the
  # game's own flag list above does not give it.
  set_property(SOURCE ${MGS_ROOT}/port/soft_render.c APPEND PROPERTY
    COMPILE_OPTIONS "-O3;-fomit-frame-pointer")

  # the game's main() becomes callable from the cart
  set_property(SOURCE ${MGS_ROOT}/source/main/main.c APPEND PROPERTY
    COMPILE_DEFINITIONS "main=mgs_main")
endif()

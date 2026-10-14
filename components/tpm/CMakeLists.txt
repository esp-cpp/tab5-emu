# Star Wars Episode I: The Phantom Menace (PSX), natively: tpm-decomp
# (https://github.com/esp-cpp/tpm-decomp, submodule tpm-decomp/).
#
# With CONFIG_TPM_ENGINE, the game's executable is statically recompiled to C
# at configure time (tpm-decomp/tools/recomp.py) and built with tpm-decomp's
# runtime (BIOS, interrupts, DMA, CD-ROM, GPU ports) on psyz's software GPU,
# GTE and SPU. The executable is the user's own: CONFIG_TPM_EXE_PATH. Without
# it the component builds only the platform half (src/), which shows the
# game's load screens from the SD card.

set(TPM_DECOMP ${CMAKE_CURRENT_LIST_DIR}/tpm-decomp)
# psyz: the copy components/mgs already carries (same revision tpm-decomp pins)
set(TPM_PSYZ ${CMAKE_CURRENT_LIST_DIR}/../mgs/psyz)

set(TPM_SRCS "src/tpm.cpp" "src/tpm_files.cpp")
set(TPM_ENGINE_READY OFF)
set(TPM_GEN ${CMAKE_CURRENT_BINARY_DIR}/recomp)

if(CONFIG_TPM_ENGINE)
  set(exe "${CONFIG_TPM_EXE_PATH}")
  if(NOT IS_ABSOLUTE "${exe}")
    set(exe "${CMAKE_SOURCE_DIR}/${exe}")
  endif()
  if(NOT EXISTS "${exe}")
    message(WARNING "tpm: ${exe} not found (CONFIG_TPM_EXE_PATH): building without the game; "
                    "the cart will show the load screens only")
  elseif(NOT EXISTS "${TPM_PSYZ}/psyz/src/platform/soft_raster.inc.c")
    message(WARNING "tpm: psyz missing at ${TPM_PSYZ} (git submodule update --init components/mgs/psyz)")
  else()
    idf_build_get_property(python PYTHON)
    if(NOT python)
      set(python python3)
    endif()
    # Re-run when the executable or the recompiler changes
    # the access sites that go to the scratchpad check it first
    set(sites ${TPM_DECOMP}/config/scratch_sites.us.txt)
    set(sites_arg)
    if(EXISTS "${sites}")
      set(sites_arg --scratch-sites "${sites}")
    endif()
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
      "${exe}" "${TPM_DECOMP}/tools/recomp.py" "${TPM_DECOMP}/config/recomp_entries.us.txt"
      "${TPM_DECOMP}/recomp/hle.us.txt" "${sites}")
    execute_process(
      COMMAND ${python} ${TPM_DECOMP}/tools/recomp.py "${exe}" ${TPM_GEN} --files 16 ${sites_arg}
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(rc EQUAL 0)
      set(TPM_ENGINE_READY ON)
      string(REGEX MATCH "recomp: [0-9]+ functions[^\n]*" summary "${err}")
      message(STATUS "tpm: ${summary}")
    else()
      message(WARNING "tpm: recompiling ${exe} failed:\n${err}")
    endif()
  endif()
endif()

if(TPM_ENGINE_READY)
  list(APPEND TPM_SRCS "src/tpm_engine.cpp")
endif()

idf_component_register(
  SRCS ${TPM_SRCS}
  INCLUDE_DIRS "include"
  PRIV_INCLUDE_DIRS "src" "${TPM_DECOMP}/recomp/runtime"
  REQUIRES tab5-emu
  PRIV_REQUIRES esp_timer heap statistics shared_memory esp_app_format
)

if(TPM_ENGINE_READY)
  target_compile_definitions(${COMPONENT_LIB} PRIVATE TPM_HAVE_ENGINE)

  # ---- the game + runtime + the psyz pieces it uses, as one library ----
  set(rt ${TPM_DECOMP}/recomp/runtime)
  file(GLOB rt_srcs ${rt}/*.c)
  file(GLOB gen_srcs ${TPM_GEN}/recomp_*.c)
  set(psyz_srcs ${TPM_PSYZ}/psyz/src/psyz/psyz_spu.c) # (the GTE is the runtime's own: rt_gte.c)
  add_library(tpm_rt STATIC ${gen_srcs} ${rt_srcs} ${psyz_srcs})
  idf_build_get_property(config_dir CONFIG_DIR)
  target_include_directories(tpm_rt PRIVATE
    ${rt} ${TPM_GEN} ${TPM_PSYZ}/psyz/include ${TPM_PSYZ}/psyz/src
    ${TPM_PSYZ}/decomp/src/libspu ${config_dir})
  # IDF's own flags (the arch flags come with the toolchain file)
  idf_build_get_property(idf_opts COMPILE_OPTIONS GENERATOR_EXPRESSION)
  idf_build_get_property(idf_c_opts C_COMPILE_OPTIONS GENERATOR_EXPRESSION)
  idf_build_get_property(idf_defs COMPILE_DEFINITIONS GENERATOR_EXPRESSION)
  target_compile_options(tpm_rt PRIVATE ${idf_opts} ${idf_c_opts})
  target_compile_definitions(tpm_rt PRIVATE ${idf_defs} ESP_PLATFORM)
  target_link_libraries(tpm_rt PRIVATE idf::heap idf::esp_hw_support idf::esp_common idf::esp_rom
                                       idf::freertos idf::log)
  #   -O2                   the game's code: speed over the IDF's size setting
  #   -fno-strict-aliasing  guest memory is read through any type
  #   -msmall-data-limit=0  nothing in PSRAM may be addressed gp-relative
  target_compile_options(tpm_rt PRIVATE -O2 -w -fno-strict-aliasing -msmall-data-limit=0
                                        -Wno-error)
  # the rasterizer's inner loops decide the frame rate; they run from
  # PSRAM-XIP, not IRAM (the runtime's default, as in the MGS port: internal
  # RAM is what the boot-time DMA reserve and the HAL need)
  set_property(SOURCE ${rt}/rt_gpu.c APPEND PROPERTY COMPILE_OPTIONS "-O3;-fomit-frame-pointer")
  # ...as are the SPU's per-sample loop and the GTE's commands, on the game's
  # thread
  set_property(SOURCE ${TPM_PSYZ}/psyz/src/psyz/psyz_spu.c ${rt}/rt_gte.c APPEND PROPERTY
               COMPILE_OPTIONS "-O3;-fomit-frame-pointer")

  # ---- hide it: components/mgs links its own psyz, with the same names ----
  # One relocatable object with only the runtime's API left global.
  set(bundle ${CMAKE_CURRENT_BINARY_DIR}/tpm_rt_bundle.o)
  # tpm_rt.ld gathers the statics: .bss to PSRAM, both bracketed for the
  # reset between launches (-d: COMMON symbols get their space here)
  add_custom_command(OUTPUT ${bundle}
    COMMAND ${CMAKE_C_COMPILER} -nostdlib -r -Wl,-d -Wl,-T,${CMAKE_CURRENT_LIST_DIR}/tpm_rt.ld
            -Wl,--whole-archive $<TARGET_FILE:tpm_rt> -Wl,--no-whole-archive -o ${bundle}.r.o
    COMMAND ${CMAKE_OBJCOPY} --keep-global-symbols=${CMAKE_CURRENT_LIST_DIR}/tpm_rt.keep
            ${bundle}.r.o ${bundle}
    DEPENDS tpm_rt ${CMAKE_CURRENT_LIST_DIR}/tpm_rt.keep ${CMAKE_CURRENT_LIST_DIR}/tpm_rt.ld
    VERBATIM)
  add_custom_target(tpm_rt_bundle DEPENDS ${bundle})
  add_dependencies(${COMPONENT_LIB} tpm_rt_bundle)
  target_link_libraries(${COMPONENT_LIB} PRIVATE ${bundle})

  # bring-up: a GPU recording to replay as a benchmark (TPM_GPU_BENCH_FILE)
  if(NOT "${CONFIG_TPM_GPU_BENCH_FILE}" STREQUAL "")
    configure_file("${CONFIG_TPM_GPU_BENCH_FILE}" ${CMAKE_CURRENT_BINARY_DIR}/gpubench.bin COPYONLY)
    target_add_binary_data(${COMPONENT_LIB} ${CMAKE_CURRENT_BINARY_DIR}/gpubench.bin BINARY)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE TPM_GPU_BENCH=1)
  endif()
endif()

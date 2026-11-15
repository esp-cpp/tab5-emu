# ScummVM (esp-cpp/scummvm, branch esp32p4: the backend is in
# scummvm/backends/platform/esp32), built with the Star Trek engine (25th
# Anniversary / Judgment Rites)
set(SVM "${CMAKE_CURRENT_LIST_DIR}/scummvm")
include(${SVM}/backends/platform/esp32/sources.cmake)
list(TRANSFORM SCUMMVM_SOURCES PREPEND "scummvm/")

idf_component_register(
  SRCS ${SCUMMVM_SOURCES}
  INCLUDE_DIRS "scummvm/backends/platform/esp32/include"
  PRIV_INCLUDE_DIRS "scummvm/backends/platform/esp32/generated" "scummvm" "scummvm/engines"
  REQUIRES tab5-emu esp_timer
  )

target_compile_definitions(${COMPONENT_LIB} PRIVATE
  HAVE_CONFIG_H POSIX RELEASE_BUILD NONSTANDARD_SAVE_PATH
  ENABLE_STARTREK=STATIC_PLUGIN
  "DATA_PATH=\"/sdcard/scummvm\""
  )
target_compile_options(${COMPONENT_LIB} PRIVATE
  -Os -fno-exceptions -Wno-error -w
  -msmall-data-limit=0
  )
# static state in PSRAM like the other cores (see tools/bss_to_psram.py)
add_custom_command(TARGET ${COMPONENT_LIB} POST_BUILD
  COMMAND ${PYTHON} ${CMAKE_CURRENT_LIST_DIR}/../../tools/bss_to_psram.py
          ${CMAKE_OBJDUMP} ${CMAKE_OBJCOPY} $<TARGET_FILE:${COMPONENT_LIB}> --data
  COMMENT "Moving ScummVM's .bss and .data to PSRAM"
  VERBATIM)
# ScummVM's allocations: PSRAM, and a game's own arena (backends/platform/esp32/esp32-alloc.cpp)
add_custom_command(TARGET ${COMPONENT_LIB} POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} --redefine-syms=${SVM}/backends/platform/esp32/psram_alloc.syms $<TARGET_FILE:${COMPONENT_LIB}>
  COMMENT "Sending ScummVM's allocations to PSRAM"
  VERBATIM)
# Allocation tracking (idf.py -DSCUMMVM_TRACK_ALLOCS=1 build; =0 to turn it
# off again): each game run's leftovers go to /sdcard/saves/scummvm_leaks.txt
# (see backends/platform/esp32/esp32-alloc.cpp)
if(SCUMMVM_TRACK_ALLOCS)
  target_compile_definitions(${COMPONENT_LIB} PRIVATE SCUMMVM_ESP32_TRACK_ALLOCS)
endif()

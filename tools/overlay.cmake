# tab5_overlay_args(<core> <out-var> [keep-regex ...]): the bss_to_psram.py
# arguments that put a game core's code into the PSRAM code overlay
# (components/tab5-emu/overlay.ld) when the firmware executes from flash, plus
# any code that has to stay in flash (--overlay-keep; see the script). Empty
# with XIP from PSRAM, where all code is in PSRAM anyway.
function(tab5_overlay_args core out)
  set(args "")
  if(NOT CONFIG_SPIRAM_XIP_FROM_PSRAM)
    list(APPEND args "--overlay=${core}" "--overlay-rodata")
    foreach(keep ${ARGN})
      list(APPEND args "--overlay-keep=${keep}")
    endforeach()
  endif()
  set(${out} "${args}" PARENT_SCOPE)
endfunction()

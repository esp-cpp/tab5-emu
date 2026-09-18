# Jedi Knight: Dark Forces II via OpenJKDF2.
#
# The engine lives in the OpenJKDF2 submodule; this component compiles it as
# part of the ESP-IDF build with the ESP32 platform layer from ./platform,
# plus the C++ glue in ./src that binds it to the Tab5Emu HAL.

set(JK_ROOT ${CMAKE_CURRENT_LIST_DIR}/OpenJKDF2)
set(JK_SRC ${JK_ROOT}/src)

set(JK_SRCS "src/jk.cpp" "src/jk_prof.cpp")
set(JK_INCLUDE_DIRS "include")
set(JK_PRIV_INCLUDE_DIRS "src")

if(CONFIG_JK_ENGINE)
  # --- engine sources (mirrors OpenJKDF2/CMakeLists.txt + plat_dreamcast) ---
  file(GLOB ENGINE_SRCS
    ${JK_SRC}/*.c
    ${JK_SRC}/*/*.c
    ${JK_SRC}/General/*.cpp
    ${JK_SRC}/Platform/Common/*.c
    ${JK_SRC}/Platform/Common/*.cpp
    ${JK_SRC}/Platform/Networking/None/*.c
    ${JK_SRC}/Platform/Posix/*.c
    ${JK_SRC}/external/fcaseopen/*.c
    ${JK_SRC}/external/libsmacker/*.c
    ${JK_SRC}/external/libsmusher/src/smush.c
    ${JK_SRC}/external/libsmusher/src/codec47.c
    ${JK_SRC}/external/libsmusher/src/codec48.c
  )
  file(GLOB ENGINE_EXCLUDE
    ${JK_SRC}/main.c
    ${JK_SRC}/hook.c
    ${JK_SRC}/Tests/*.c
    ${JK_SRC}/Dw/*.c
    # multiplayer menus (TARGET_NO_MULTIPLAYER_MENUS), as upstream
    ${JK_SRC}/Gui/jkGUIMultiplayer.c
    ${JK_SRC}/Gui/jkGUIBuildMulti.c
  )
  if(ENGINE_EXCLUDE)
    list(REMOVE_ITEM ENGINE_SRCS ${ENGINE_EXCLUDE})
  endif()
  file(GLOB PLATFORM_SRCS ${JK_SRC}/Platform/ESP32/*.c)
  list(APPEND JK_SRCS ${ENGINE_SRCS} ${PLATFORM_SRCS} ${CMAKE_CURRENT_LIST_DIR}/generated/globals.c)
  list(APPEND JK_PRIV_INCLUDE_DIRS
    ${JK_SRC}
    ${JK_SRC}/external/libsmacker
    ${JK_SRC}/external/libsmusher/src
    ${JK_ROOT}/3rdparty/json/include
    ${CMAKE_CURRENT_LIST_DIR}/generated
    ${JK_SRC}/Platform/ESP32
  )
endif()

idf_component_register(
  SRCS ${JK_SRCS}
  INCLUDE_DIRS ${JK_INCLUDE_DIRS}
  PRIV_INCLUDE_DIRS ${JK_PRIV_INCLUDE_DIRS}
  REQUIRES tab5-emu statistics
  PRIV_REQUIRES esp_timer esp_system riscv
)

if(CONFIG_JK_ENGINE)
  target_compile_definitions(${COMPONENT_LIB} PRIVATE
    PLAT_MISSING_WIN32
    PLATFORM_POSIX
    NO_JK_MMAP
    LINUX_TMP
    PLATFORM_NOSOCKETS
    STDSOUND_ESP32
    TARGET_ESP32
    TARGET_RETRO_HOMEBREW
    _XOPEN_SOURCE=500
    _DEFAULT_SOURCE
    SMK_FAST
    QOL_IMPROVEMENTS
    TARGET_NO_MULTIPLAYER_MENUS
    OPENJKDF2_PROJECT_NAME=OpenJKDF2
    OPENJKDF2_PROJECT_VERSION=0.9.9
    OPENJKDF2_MAJOR_VERSION=0
    OPENJKDF2_MINOR_VERSION=9
    OPENJKDF2_PATCH_VERSION=9
    OPENJKDF2_TWEAK_VERSION=0
    OPENJKDF2_RELEASE_VERSION_STRING="v0.9.9-esp32"
    OPENJKDF2_RELEASE_VERSION_STRING_U=u"v0.9.9-esp32"
    OPENJKDF2_RELEASE_COMMIT="esp32"
    OPENJKDF2_RELEASE_COMMIT_SHORT="esp32"
    OPENJKDF2_RELEASE_COMMIT_U=u"esp32"
    OPENJKDF2_RELEASE_COMMIT_SHORT_U=u"esp32"
  )
  # Engine-only flags (the C++ glue in src/ keeps the project defaults)
  set(ENGINE_COMMON_FLAGS
    -Wno-error
    -Wno-unused-variable
    -Wno-unused-function
    -Wno-parentheses
    -Wno-missing-braces
    -Wno-format
    -Wno-sign-compare
    -Wno-implicit-fallthrough
    -Wno-misleading-indentation
    -Wno-char-subscripts
    -Wno-unused-but-set-variable
    -Wno-int-in-bool-context
    -fshort-wchar
    -fno-fast-math
    -fno-unsafe-math-optimizations
    -ffp-contract=off
    -msmall-data-limit=0
    -O2
    # relative paths -> game directory (ESP-IDF has no cwd), see jk_esp_fs.h
    -include ${JK_SRC}/Platform/ESP32/jk_esp_fs.h
  )
  set(ENGINE_C_SRCS ${ENGINE_SRCS} ${PLATFORM_SRCS} ${CMAKE_CURRENT_LIST_DIR}/generated/globals.c)
  list(FILTER ENGINE_C_SRCS EXCLUDE REGEX "\\.cpp$")
  set(ENGINE_CXX_SRCS ${ENGINE_SRCS})
  list(FILTER ENGINE_CXX_SRCS INCLUDE REGEX "\\.cpp$")
  set_source_files_properties(${ENGINE_C_SRCS} PROPERTIES COMPILE_OPTIONS
    "${ENGINE_COMMON_FLAGS};-std=gnu11;-Wno-incompatible-pointer-types;-Wno-discarded-qualifiers;-Wno-pointer-sign;-Wno-int-conversion;-Wno-implicit-function-declaration;-Wno-return-type")
  if(ENGINE_CXX_SRCS)
    set_source_files_properties(${ENGINE_CXX_SRCS} PROPERTIES COMPILE_OPTIONS
      "${ENGINE_COMMON_FLAGS};-std=gnu++17")
  endif()
endif()

if(CONFIG_JK_ENGINE AND CONFIG_JK_ESP_DEBUG)
  target_compile_definitions(${COMPONENT_LIB} PRIVATE JK_ESP_FS_DEBUG)
endif()

if(CONFIG_JK_ENGINE)
  # The engine has ~1.4 MB of static state; move it out of internal SRAM into
  # PSRAM by renaming the archive's .bss sections (see tools/bss_to_psram.py).
  add_custom_command(TARGET ${COMPONENT_LIB} POST_BUILD
    COMMAND ${PYTHON} ${CMAKE_CURRENT_LIST_DIR}/tools/bss_to_psram.py
            ${CMAKE_OBJDUMP} ${CMAKE_OBJCOPY} $<TARGET_FILE:${COMPONENT_LIB}>
    COMMENT "Moving the engine's .bss to PSRAM"
    VERBATIM)
endif()

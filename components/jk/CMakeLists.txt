# Jedi Knight: Dark Forces II via OpenJKDF2.
#
# The engine lives in the OpenJKDF2 submodule; this component compiles it as
# part of the ESP-IDF build with the ESP32 platform layer from ./platform,
# plus the C++ glue in ./src that binds it to the Tab5Emu HAL.

set(JK_ROOT ${CMAKE_CURRENT_LIST_DIR}/OpenJKDF2)
set(JK_SRC ${JK_ROOT}/src)

set(JK_SRCS "src/jk.cpp")
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
    ${JK_SRC}/external/libsmusher/src/*.c
  )
  file(GLOB ENGINE_EXCLUDE
    ${JK_SRC}/main.c
    ${JK_SRC}/hook.c
    ${JK_SRC}/Tests/*.c
    ${JK_SRC}/Dw/*.c
    ${JK_SRC}/Platform/Common/stdUpdater.cpp
    ${JK_SRC}/Platform/Common/stdHttp.c
    ${JK_SRC}/Platform/Common/stdControl.c
    # multiplayer menus (TARGET_NO_MULTIPLAYER_MENUS)
    ${JK_SRC}/Gui/jkGUIMultiplayer.c
    ${JK_SRC}/Gui/jkGUIMultiTally.c
    ${JK_SRC}/Gui/jkGUINetHost.c
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
  PRIV_REQUIRES esp_timer
)

if(CONFIG_JK_ENGINE)
  target_compile_definitions(${COMPONENT_LIB} PRIVATE
    PLAT_MISSING_WIN32
    PLATFORM_POSIX
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
  target_compile_options(${COMPONENT_LIB} PRIVATE
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
    -O2
    $<$<COMPILE_LANGUAGE:C>:-std=gnu11>
    $<$<COMPILE_LANGUAGE:C>:-Wno-incompatible-pointer-types>
    $<$<COMPILE_LANGUAGE:C>:-Wno-discarded-qualifiers>
    $<$<COMPILE_LANGUAGE:C>:-Wno-pointer-sign>
    $<$<COMPILE_LANGUAGE:C>:-Wno-int-conversion>
    $<$<COMPILE_LANGUAGE:C>:-Wno-implicit-function-declaration>
    $<$<COMPILE_LANGUAGE:C>:-Wno-return-type>
    $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++17>
  )
endif()

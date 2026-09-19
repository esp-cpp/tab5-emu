idf_component_register(
    SRC_DIRS
    "src"
    "gwenesis/src/bus"
    "gwenesis/src/cpus/M68K"
    "gwenesis/src/cpus/Z80"
    "gwenesis/src/sound"
    "gwenesis/src/vdp"
    "gwenesis/src/io"
    "gwenesis/src/savestate"
    INCLUDE_DIRS
        "include"
        "gwenesis/src"
        "gwenesis/src/bus"
        "gwenesis/src/cpus/M68K"
        "gwenesis/src/cpus/Z80"
        "gwenesis/src/sound"
        "gwenesis/src/vdp"
        "gwenesis/src/io"
        "gwenesis/src/savestate"
    REQUIRES
        shared_memory
        tab5-emu
        statistics
)
# target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-char-subscripts -Wno-attributes -Wno-implicit-fallthrough -Wno-unused-function -Wno-unused-variable -Wno-discarded-qualifiers)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-const-variable -Wno-unused-value -Ofast
  # gcc 15 (ESP-IDF 6.x) promotes these to errors; the core is upstream code
  -Wno-error=sizeof-pointer-memaccess -Wno-char-subscripts -Wno-attributes -Wno-implicit-fallthrough
  -Wno-unused-function -Wno-unused-variable -Wno-error=discarded-qualifiers -Wno-error=incompatible-pointer-types
  -Wno-error=int-conversion -Wno-error=implicit-function-declaration -Wno-error=stringop-overflow
  -Wno-error=array-bounds -Wno-error=maybe-uninitialized -Wno-error=address -Wno-error=misleading-indentation)
# target_compile_definitions(${COMPONENT_LIB} PRIVATE GWENESIS_AUDIO_ACCURATE=0 BUILD_TABLES=0 TABLES_FULL=0)
target_compile_definitions(${COMPONENT_LIB} PRIVATE GWENESIS_AUDIO_ACCURATE=0)

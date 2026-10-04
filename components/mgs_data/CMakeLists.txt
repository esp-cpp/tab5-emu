# The run-time copy of the game's initialized data (see
# components/mgs/tools/data_to_psram.py): a C file generated when libmgs.a is
# post-processed, compiled here because a component cannot compile a source
# produced from its own archive.
set(MGS_DATA_INIT ${CMAKE_BINARY_DIR}/mgs_data_init.c)
if(NOT EXISTS ${MGS_DATA_INIT})
  file(WRITE ${MGS_DATA_INIT} "void mgs_data_init(void) {}\n")
endif()
idf_component_register(SRCS ${MGS_DATA_INIT} REQUIRES mgs)
# (everything below runs only in the real configure pass, after registration)
set_source_files_properties(${MGS_DATA_INIT} PROPERTIES GENERATED TRUE)
# built after the mgs archive, whose post-build step rewrites the file (ninja
# re-checks the source's timestamp when it comes to compile it)
add_dependencies(${COMPONENT_LIB} __idf_mgs)

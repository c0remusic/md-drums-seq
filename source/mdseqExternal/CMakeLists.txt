# max-sdk-base turns this directory's project into an external, written to C74_LIBRARY_OUTPUT_DIRECTORY.
set_property(DIRECTORY PROPERTY max::external YES)
project(mdseq)

target_sources(mdseq PRIVATE mdseq.cpp updates.h updates.cpp)
target_link_libraries(mdseq PRIVATE mdseqCore)
# A newer release, asked of GitHub over HTTPS (updates.cpp)
if(WIN32)
	target_link_libraries(mdseq PRIVATE winhttp)
endif()

# The Max class, and the file's name with it. Max never reloads a class it has loaded, so a development build takes a
# new name (-DMDSEQ_CLASS=mdseq_dbg2) to be loaded by a running Live without restarting it; releases are "mdseq".
set_target_properties(mdseq PROPERTIES OUTPUT_NAME "${MDSEQ_CLASS}")
target_compile_definitions(mdseq PRIVATE MDSEQ_CLASS_NAME="${MDSEQ_CLASS}" MDSEQ_VERSION="${CMAKE_PROJECT_VERSION}")
# A development build also starts with a demonstration Pattern, to be heard and measured without clicking, and
# logs what it sees to %TEMP%\mdseq-log.txt.
if(NOT MDSEQ_CLASS STREQUAL "mdseq")
	target_compile_definitions(mdseq PRIVATE MDSEQ_DEV=1)
endif()

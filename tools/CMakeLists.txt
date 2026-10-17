# The device, written next to the external so that it loads it from its own folder, and checked as far as Python can
# without Max.
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# The vocabulary between the patch and the external as the core defines it (mdseqProtocol.h), written as JSON before
# the device: make_device.py builds the patch from it, and check_device.py checks the device against it.
add_executable(mdseqVocabulary mdseqVocabulary.cpp)
target_link_libraries(mdseqVocabulary PRIVATE mdseqCore)
if(MSVC)
	target_compile_options(mdseqVocabulary PRIVATE /W4 /WX)
endif()

set(_vocabulary "${CMAKE_CURRENT_BINARY_DIR}/vocabulary.json")
add_custom_command(
	OUTPUT "${_vocabulary}"
	COMMAND mdseqVocabulary "${_vocabulary}"
	DEPENDS mdseqVocabulary
	COMMENT "Writing the vocabulary between the patch and the external"
	VERBATIM
)

set(_device "${C74_LIBRARY_OUTPUT_DIRECTORY}/MD Drums Seq.amxd")
add_custom_command(
	OUTPUT "${_device}"
	COMMAND Python3::Interpreter -I "${CMAKE_CURRENT_SOURCE_DIR}/make_device.py" "${_device}" "${_vocabulary}"
		"${MDSEQ_CLASS}"
	DEPENDS make_device.py "${_vocabulary}" "${CMAKE_BINARY_DIR}/CMakeCache.txt"
	COMMENT "Writing the device MD Drums Seq.amxd"
	VERBATIM
)
add_custom_target(mdseqDevice ALL DEPENDS "${_device}")

# A release's zip, md-drums-seq-<version>-win64.zip in the build folder, on request: cmake --build build --config
# Release --target zip_release. package.py refuses a development build.
add_custom_target(zip_release
	COMMAND Python3::Interpreter -I "${CMAKE_CURRENT_SOURCE_DIR}/package.py" "${C74_LIBRARY_OUTPUT_DIRECTORY}"
		"${CMAKE_PROJECT_VERSION}" "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}"
	COMMENT "Writing the release's zip"
	VERBATIM
)
add_dependencies(zip_release mdseqDevice mdseq)

add_test(NAME mdseqDeviceCheck
	COMMAND Python3::Interpreter -I "${CMAKE_CURRENT_SOURCE_DIR}/check_device.py" "${_device}" "${_vocabulary}")

# The rule both tools hold the patch's messages to, on messages the device does not send.
add_test(NAME mdseqSelectorCheck
	COMMAND Python3::Interpreter -I "${CMAKE_CURRENT_SOURCE_DIR}/check_selectors.py" "${_vocabulary}")

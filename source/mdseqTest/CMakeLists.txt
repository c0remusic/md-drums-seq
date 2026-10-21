foreach(_test mdseqSequencerTest mdseqConditionTest mdseqViewTest mdseqEditorTest mdseqStoreTest mdseqPrintTest
	mdseqPrinterTest mdseqProtocolTest mdseqKitTest mdseqUpdatesTest mdseqPushTest mdseqCaptureTest)
	add_executable(${_test} ${_test}.cpp mdseqTestSupport.h)
	target_link_libraries(${_test} PRIVATE mdseqCore)
	if(MSVC)
		target_compile_options(${_test} PRIVATE /W4 /WX)
	endif()
	add_test(NAME ${_test} COMMAND ${_test})
endforeach()

# The sequencer's core, without Max (ADR 0002).
add_library(mdseqCore STATIC
	mdseqTypes.h
	mdseqEdit.h
	mdseqEdit.cpp
	mdseqCondition.h
	mdseqCondition.cpp
	mdseqSequencer.h
	mdseqSequencer.cpp
	mdseqText.h
	mdseqText.cpp
	mdseqView.h
	mdseqView.cpp
	mdseqEditor.h
	mdseqEditor.cpp
	mdseqStore.h
	mdseqStore.cpp
	mdseqPrint.h
	mdseqPrint.cpp
	mdseqPrinter.h
	mdseqPrinter.cpp
	mdseqProtocol.h
	mdseqProtocol.cpp
	mdseqKit.h
	mdseqKit.cpp
	mdseqUpdates.h
	mdseqUpdates.cpp
	mdseqPush.h
	mdseqPush.cpp
	mdseqCapture.h
	mdseqCapture.cpp
)
target_include_directories(mdseqCore PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(mdseqCore PUBLIC cxx_std_17)
if(MSVC)
	target_compile_options(mdseqCore PRIVATE /W4 /WX)
endif()

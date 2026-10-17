cmake_minimum_required(VERSION 3.25)

# max-sdk-base has to come before the first project(): it hooks every project() of the tree, and makes an external
# of each directory that sets max::external (source/mdseqExternal). Pinned for reproducible builds.
include(FetchContent)
FetchContent_Declare(
	max-sdk-base
	GIT_REPOSITORY https://github.com/Cycling74/max-sdk-base.git
	GIT_TAG c03a2922a2a8ff149165a1e2ea134e9321d6e202
)
# The external lands next to the device that loads it.
set(C74_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/device")
set(MDSEQ_CLASS "mdseq" CACHE STRING "The external's Max class and file name (a new one per development build)")
FetchContent_MakeAvailable(max-sdk-base)

# The version a release carries (tools/package.py names its zip after it), shown in the Max console as the device loads.
project(md-drums-seq VERSION 0.1.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

add_subdirectory(source/mdseq)
add_subdirectory(source/mdseqTest)
add_subdirectory(source/mdseqExternal)
add_subdirectory(tools)

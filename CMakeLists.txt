cmake_minimum_required (VERSION 3.8)

project("BoardMouse")

# Add all headers and code files
file(GLOB_RECURSE CODE_FILES "src/*.cpp" "src/*.h")

add_executable(BoardMouse ${CODE_FILES})

set_target_properties(BoardMouse PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(BoardMouse PROPERTIES CXX_STANDARD 20)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	# ...
else()
	set(GENERAL_EXTRA_ARGS "-mbmi2")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GENERAL_EXTRA_ARGS}")
endif()
cmake_minimum_required (VERSION 3.8)

project("BoardMouse")

# Add all headers and code files
file(GLOB_RECURSE CODE_FILES "src/*.cpp" "src/*.h")

add_executable(BoardMouse ${CODE_FILES})

set_target_properties(BoardMouse PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(BoardMouse PROPERTIES CXX_STANDARD 20)

set(GENERAL_EXTRA_ARGS "")
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	# MSVC shouldn't need to be told to support BMI2
	if (NOT (CMAKE_BUILD_TYPE MATCHES DEBUG))
		set(GENERAL_EXTRA_ARGS "/O2 /Oi /Ot /Oy /GL /GS-")
		message("BoardMouse: Adding MSVC parameters: \"${GENERAL_EXTRA_ARGS}\"")
	endif()
else()
	message("BoardMouse: Adding \"-mbmi\" and \"-mbmi2\" flag")
	set(GENERAL_EXTRA_ARGS "-mbmi2 -mbmi")
	if (NOT (CMAKE_BUILD_TYPE MATCHES DEBUG))
		set(GENERAL_EXTRA_ARGS "${GENERAL_EXTRA_ARGS} -O3")
		if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
			set(GENERAL_EXTRA_ARGS "${GENERAL_EXTRA_ARGS} -Ofast -fno-sanitize")
			message("BoardMouse: Adding Clang parameters: \"${GENERAL_EXTRA_ARGS}\"")
		endif()
	endif()
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GENERAL_EXTRA_ARGS}")
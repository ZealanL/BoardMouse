cmake_minimum_required (VERSION 3.8)

project("BoardMouse")

# Add all headers and code files
file(GLOB_RECURSE CODE_FILES "src/*.cpp" "src/*.h")

add_executable(BoardMouse ${CODE_FILES})

set_target_properties(BoardMouse PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(BoardMouse PROPERTIES CXX_STANDARD 20)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	# MSVC shouldn't need to be told to support BMI2
else()
	message("BoardMouse: Adding \"-mbmi2\" flag")
	set(GENERAL_EXTRA_ARGS "-mbmi2")
	if (NOT (CMAKE_BUILD_TYPE MATCHES DEBUG))
		set(GENERAL_EXTRA_ARGS "${GENERAL_EXTRA_ARGS} -O3")
		if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
			message("BoardMouse: Adding Clang optimization parameters")
			set(GENERAL_EXTRA_ARGS "${GENERAL_EXTRA_ARGS} -Ofast -fno-sanitize")
		endif()
	endif()
	
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GENERAL_EXTRA_ARGS}")
endif()
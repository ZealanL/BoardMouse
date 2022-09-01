#pragma once

// Framework.h is the lowest-level include header to be used by anything and everything

#pragma region STL Includes
#define _USE_MATH_DEFINES

#include <algorithm>
#include <bitset>
#include <cassert>
#include <chrono>
#include <chrono>
#include <deque>
#include <fstream>
#include <functional>
#include <immintrin.h>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <queue>
#include <random>
#include <set>
#include <sstream>
#include <stack>
#include <stdint.h>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Remove need for std namespace scope for very common datatypes
using std::vector;
using std::map;
using std::unordered_map;
using std::set;
using std::multiset;
using std::unordered_set;
using std::list;
using std::stack;
using std::deque;
using std::string;
using std::wstring;
using std::pair;

// Integer typedefs
typedef int8_t	int8;	typedef uint8_t	 uint8;
typedef int16_t int16;	typedef uint16_t uint16;
typedef int32_t int32;	typedef uint32_t uint32;
typedef int64_t int64;	typedef uint64_t uint64;
typedef uint8_t byte;
#pragma endregion

// Current millisecond time
#define CUR_MS() (std::chrono::duration_cast<std::chrono::miliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count())

#pragma region Logic Macros
#define MAX(a, b) ((a > b) ? a : b)
#define MIN(a, b) ((a < b) ? a : b)

#define CLAMP(val, min, max) MIN(MAX(val, min), max)
#pragma endregion

#pragma region Printing and Strings
#define LOG(s) { std::cout << std::dec << s << std::endl; }
#define STR(s) ([=]{ std::stringstream __macroStream; __macroStream << s; return __macroStream.str(); }())

// DLOG (debug-log) only works in debug builds
#ifdef _DEBUG
#define DLOG(s) LOG(s)
#else
#define DLOG(s) {}
#endif
#pragma endregion

#pragma region Assertions
#define ASSERT assert
#define SASSERT static_assert
#pragma endregion

#define FINLINE __forceinline

// Make sure we are 64-bit
SASSERT(sizeof(void*) == 8, "BoardMouse can only run as a 64-bit program");
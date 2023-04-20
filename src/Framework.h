#pragma once

// Framework.h is the lowest-level include header to be used by anything and everything

#define BM_VERSION "v0.0"

#pragma region STL Includes
#define _USE_MATH_DEFINES

#include <algorithm>
#include <bitset>
#include <cassert>
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

typedef uint8_t byte;
#pragma endregion

// Current millisecond time
#define CUR_MS() (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count())

#pragma region Logic Macros
#define MAX(a, b) ((a > b) ? a : b)
#define MIN(a, b) ((a < b) ? a : b)

#define CLAMP(val, min, max) MIN(MAX(val, min), max)
#pragma endregion

#pragma region Printing and Strings
#define LOG(s) { std::cout << STR(s << std::endl); }
#define STR(s) ([&]{ std::stringstream __macroStream; __macroStream << s; return __macroStream.str(); }())

// DLOG (debug-log) only works in debug builds
#ifdef _DEBUG
#define DLOG(s) LOG(s)
#else
#define DLOG(s) {}
#endif

#define ERR_CLOSE(s) { \
	LOG("FATAL ERROR: " << s); \
	assert(false); \
	exit(EXIT_FAILURE); \
}

#pragma endregion

#pragma region Assertions
#define ASSERT assert
#define SASSERT static_assert
#pragma endregion

#define FINLINE inline

// Make sure we are 64-bit
SASSERT(sizeof(void*) == 8, "BoardMouse can only run as a 64-bit program");

// Get a bit of an integer
#define INTBIT(x, i) (((uint64_t)x >> (uint64_t)i) & 1)

inline vector<string> __SPLIT_STR(const string& str, const string& delim) {
	vector<string> results;

	size_t startPos = 0;
	size_t endPos;

	string tokenBuffer;
	while ((endPos = str.find(delim, startPos)) != string::npos) {
		tokenBuffer = str.substr(startPos, endPos - startPos);
		startPos = endPos + delim.size();
		results.push_back(tokenBuffer);
	}

	results.push_back(str.substr(startPos));
	return results;
}

#define SPLIT_STR __SPLIT_STR

#if defined(_MSC_VER) && !defined(__clang__)
FINLINE uint32_t __INTRIN_CTZ(uint64_t val) {
	unsigned long result;
	_BitScanForward64(&result, val);
	return result;
}
FINLINE uint64_t __INTRIN_PEXT(uint64_t data, uint64_t mask) {
	return _pext_u64(data, mask);
}
FINLINE uint64_t __INTRIN_PDEP(uint64_t data, uint64_t mask) {
	return _pdep_u64(data, mask);
}
FINLINE uint64_t __INTRIN_BLSR(uint64_t val) {
	return _blsr_u64(val);
}
#else
FINLINE uint32_t __INTRIN_CTZ(uint64_t val) {
	return __builtin_ctzll(val);
}
FINLINE uint64_t __INTRIN_PEXT(uint64_t data, uint64_t mask) {
	return __builtin_ia32_pext_di(data, mask);
}
FINLINE uint64_t __INTRIN_PDEP(uint64_t data, uint64_t mask) {
	return __builtin_ia32_pdep_di(data, mask);
}
FINLINE uint64_t __INTRIN_BLSR(uint64_t val) {
	return val & (val - 1);
}
#endif

#define INTRIN_CTZ __INTRIN_CTZ
#define INTRIN_PEXT __INTRIN_PEXT
#define INTRIN_PDEP __INTRIN_PDEP
#define INTRIN_BLSR __INTRIN_BLSR
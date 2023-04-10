#pragma once
#include "../BitBoard/BitBoard.h"
#include "../Pos/Pos.h"

namespace Util {
	/*
	// Get the position index of a position in algebraic notation
	constexpr uint8_t ANI(uint16_t chars);

	// Returns ANI but as a bitmask
	constexpr uint64_t ANI_BM(uint16_t chars);
	*/

	constexpr uint8_t ANI(char letter, char number) {
		int x = letter - ((letter >= 'a') ? 'a' : 'A');
		int y = number - '1';
		return POSI(x, y);
	}

	constexpr uint8_t ANI(uint64_t bothChars) {
		return ANI(bothChars >> 8, bothChars & 0xFF);
	}

	template<typename T>
	constexpr uint64_t ANI_BM(T chars) {
		return 1ull << ANI(chars);
	}
}
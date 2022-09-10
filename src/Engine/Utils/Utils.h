#pragma once
#include "../BitBoard/BitBoard.h"
#include "../Pos/Pos.h"

namespace Util {
	// Get the position index of a position in algebraic notation
	constexpr uint8_t ANI(uint16_t chars);

	// Returns ANI but as a bitmask
	constexpr uint64_t ANI_BM(uint16_t chars);
}
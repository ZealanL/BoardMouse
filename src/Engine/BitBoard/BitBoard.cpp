#include "BitBoard.h"

void BitBoard::Set(uint64 index, bool val) {
	ASSERT(index < BD_SQUARE_AMOUNT);

	if (val) {
		data |= (1ull << val);
	} else {
		data &= ~(1ull << val);
	}
}

BitBoard BitBoard::FlippedX() {

	// Algorithm from https://www.chessprogramming.org/Flipping_Mirroring_and_Rotating

	constexpr uint64 MASKS[3] = {
		0x5555555555555555,
		0x3333333333333333,
		0x0f0f0f0f0f0f0f0f
	};

	BitBoard result = *this;
	result = ((result >> 1) & MASKS[0]) + 2		* (result & MASKS[0]);
	result = ((result >> 2) & MASKS[1]) + 4		* (result & MASKS[1]);
	result = ((result >> 4) & MASKS[2]) + 16	* (result & MASKS[2]);
	return result;
}

BitBoard BitBoard::FlippedY() {
	return _byteswap_uint64(data);
}

BitBoard BitBoard::ExtractBits(uint64 mask) {
	return _pext_u64(data, mask);
}
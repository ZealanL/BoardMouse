#include "BitBoard.h"

void BitBoard::Set(uint64 index, bool val) {
	ASSERT(index < BD_SQUARE_AMOUNT);

	if (val) {
		data |= (1ull << index);
	} else {
		data &= ~(1ull << index);
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

BitBoard BitBoard::MapBits(uint64 mask) {
	return _pdep_u64(data, mask);
}

// TODO: This can probably be made faster
void BitBoard::Iterate(std::function<void(uint64)> func) const {
	BitBoard bb = *this;
	uint64 bitCount = __popcnt64(bb);
	unsigned long curIndex = 0;

	for (int i = 0; i < bitCount; i++) {
		// Determine how many bits to walk forward
		_BitScanReverse64(&curIndex, bb);
		
		// Call with current index
		func(curIndex);

		// Remove the bit we just reached and continue
		bb -= 1ull << curIndex;
	}
}

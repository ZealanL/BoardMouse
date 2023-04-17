#include "BitBoard.h"

#include "../Pos/Pos.h"

void BitBoard::Set(uint64_t index, bool val) {
	ASSERT(index < BD_SQUARE_AMOUNT);

	if (val) {
		data |= (1ull << index);
	} else {
		data &= ~(1ull << index);
	}
}

BitBoard BitBoard::FlippedX() const {

	// Algorithm from https://www.chessprogramming.org/Flipping_Mirroring_and_Rotating

	constexpr uint64_t MASKS[3] = {
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

BitBoard BitBoard::FlippedY() const {
	return _byteswap_uint64(data);
}

BitBoard BitBoard::ExtractBits(uint64_t mask) const {
	return _pext_u64(data, mask);
}

BitBoard BitBoard::MapBits(uint64_t mask) const {
	return _pdep_u64(data, mask);
}

uint64_t BitBoard::BitCount() const {
	return __popcnt64(data);
}

void BitBoard::Iterate(std::function<void(uint64_t)> func) const {
	uint64_t dataCopy = this->data;
	while (dataCopy) {
		uint32_t i = INTRIN_CTZ(dataCopy);
		func(i);
		dataCopy &= dataCopy - 1;
	}
}

std::ostream& operator<<(std::ostream& stream, const BitBoard& bitBoard) {
	stream << " {" << std::endl;
	for (int y = 0; y < 8; y++) {
		stream << "\t";
		for (int x = 0; x < 8; x++) {
			Pos p = Pos(x, y);
			stream << (int)bitBoard[p];

			if (x < 7) {
				stream << ", ";
			}
		}
		stream << std::endl;
	}

	stream << "} ";
	return stream;
}
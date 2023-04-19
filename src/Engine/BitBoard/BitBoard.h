#pragma once
#include "../ChessDef.h"

// Represents a chessboard with a bit for each square
// Extremely useful for fast chess calculations, thanks to bitwise operators
struct BitBoard {
	uint64_t data;

	BitBoard(uint64_t data64 = 0) : data(data64) {}

	FINLINE static BitBoard Filled() {
		return ~0ull;
	}

	FINLINE bool operator[](uint64_t index) const {
		return data & (1ull << index);
	}

	FINLINE void Set(uint64_t index, bool val) {
		ASSERT(index < BD_SQUARE_AMOUNT);

		if (val) {
			data |= (1ull << index);
		} else {
			data &= ~(1ull << index);
		}
	}

	FINLINE BitBoard FlippedX() const {
		// Algorithm from https://www.chessprogramming.org/Flipping_Mirroring_and_Rotating

		constexpr uint64_t MASKS[3] = {
			0x5555555555555555,
			0x3333333333333333,
			0x0f0f0f0f0f0f0f0f
		};

		BitBoard result = *this;
		result = ((result >> 1) & MASKS[0]) + 2 * (result & MASKS[0]);
		result = ((result >> 2) & MASKS[1]) + 4 * (result & MASKS[1]);
		result = ((result >> 4) & MASKS[2]) + 16 * (result & MASKS[2]);
		return result;
	}

	FINLINE BitBoard FlippedY() const {
		return _byteswap_uint64(data);
	}

	// Gets all bits within the mask and appends them in order to a new bitboard
	// Extremely helpful for sliding piece occlusion lookups
	FINLINE BitBoard ExtractBits(uint64_t mask) const {
		return INTRIN_PEXT(data, mask);
	}

	// Opposite of ExtractBits: takes all of our bits and places them within the mask area, in order
	FINLINE BitBoard MapBits(uint64_t mask) const {
		return INTRIN_PDEP(data, mask);
	}

	FINLINE uint64_t BitCount() const {
		return __popcnt64(data);
	}

	FINLINE operator uint64_t&() {
		return data;
	}

	FINLINE operator uint64_t() const {
		return data;
	}

	// Runs a function on any 1-bit in this mask
	// Function is called with current index as the argument
	template <typename F>
	FINLINE void Iterate(F&& func) const {
		uint64_t dataCopy = this->data;
		while (dataCopy) {
			uint32_t i = INTRIN_CTZ(dataCopy);
			func(i);
			dataCopy = INTRIN_BSLR(dataCopy);
		}
	}

	friend std::ostream& operator<<(std::ostream& stream, const BitBoard& bitBoard);
};

SASSERT(sizeof(BitBoard) == sizeof(uint64_t));

// BitBoard mask for all squares along the edges of the board
#define BB_MASK_BORDER BitBoard(0xFF818181818181FF)
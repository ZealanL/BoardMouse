#pragma once
#include "../ChessDef.h"

// Represents a chessboard with a bit for each square
// Extremely useful for fast chess calculations, thanks to bitwise operators
struct BitBoard {
	uint64_t data;

	BitBoard(uint64_t data64 = 0) : data(data64) {}

	bool operator[](uint64_t index) const {
		return data & (1ull << index);
	}

	void Set(uint64_t index, bool val);

	BitBoard FlippedX() const;
	BitBoard FlippedY() const;

	// Gets all bits within the mask and appends them in order to a new bitboard
	// Extremely helpful for sliding piece occlusion lookups
	BitBoard ExtractBits(uint64_t mask) const;

	// Opposite of ExtractBits: takes all of our bits and places them within the mask area, in order
	BitBoard MapBits(uint64_t mask) const;

    uint64_t BitCount() const;

	operator uint64_t&() {
		return data;
	}

	operator uint64_t() const {
		return data;
	}

	// Runs a function on any 1-bit in this mask
	// Function is called with current index as the argument
	void Iterate(std::function<void(uint64_t)> func) const;
};

SASSERT(sizeof(BitBoard) == sizeof(uint64_t));

// BitBoard mask for all squares along the edges of the board
#define BB_MASK_BORDER BitBoard(0xFF818181818181FF)
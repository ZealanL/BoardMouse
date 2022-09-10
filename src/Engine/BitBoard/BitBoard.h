#pragma once
#include "../ChessDef.h"

// Represents a chessboard with a bit for each square
// Extremely useful for fast chess calculations, thanks to bitwise operators
struct BitBoard {
	uint64 data;

	BitBoard(uint64 data64 = 0) : data(data64) {}

	bool operator[](uint64 index) const {
		return data & (1ull << index);
	}

	void Set(uint64 index, bool val);

	BitBoard FlippedX();
	BitBoard FlippedY();

	// Gets all bits within the mask and appends them in order to a new bitboard
	// Extremely helpful for sliding piece occlusion lookups
	BitBoard ExtractBits(uint64 mask);

	// Opposite of ExtractBits: takes all of our bits and places them within the mask area, in order
	BitBoard MapBits(uint64 mask);

	operator uint64&() {
		return data;
	}

	operator uint64() const {
		return data;
	}

	// Runs a function on any 1-bit in this mask
	// Function is called with current index as the argument
	void Iterate(std::function<void(uint64)> func) const;
};

SASSERT(sizeof(BitBoard) == sizeof(uint64));

// BitBoard mask for all squares along the edges of the board
#define BB_MASK_BORDER BitBoard(0xFF818181818181FF)
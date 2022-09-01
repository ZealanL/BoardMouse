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

	BitBoard ExtractBits(uint64 mask);

	operator uint64&() {
		return data;
	}

	operator uint64() const {
		return data;
	}
};

SASSERT(sizeof(BitBoard) == sizeof(uint64));
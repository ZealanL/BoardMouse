#pragma once
#include "../ChessDef.h"

// A position on the chess board
struct Pos {
	byte index; // Positional index

	Pos(byte index = 0) : index(index) {}
	Pos(int x, int y) {
		index = x + (y * BD_SIZE);
	}

	// Returns false if index is out-of-range
	bool IsValid();
};
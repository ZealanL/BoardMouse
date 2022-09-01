#pragma once
#include "../ChessDef.h"

// A position on the chess board
struct Pos {
	byte index; // Positional index

	// Returns false if index is out-of-range
	bool IsValid();
};
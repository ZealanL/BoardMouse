#pragma once
#include "../Pos/Pos.h"

struct Move {
	Pos from, to;

	// Piece we moved
	uint8_t originalPiece;

	// Usually set to the piece we moved, except when promoting pawns
	uint8_t resultPiece;

	bool isCapture = false;

	Move() = default;

	FINLINE bool IsValid() const {
		return from != to;
	}

	friend std::ostream& operator<<(std::ostream& stream, const Move& move);
};
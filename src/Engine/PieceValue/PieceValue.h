#pragma once
#include "../BitBoard/BitBoard.h"
#include "../Pos/Pos.h"

// Value of a piece or position in centipawns
typedef int64_t Value;

// Number used for representing centipawn value of a checkmate internally
// Just needs to be a big number that will never be reached otherwise
#define CHECKMATE_VALUE (1000 * 1000)

namespace PieceValue {
	Value CalcPieceSquareValue(uint8_t pieceType, uint8_t team, Pos pos, bool isEndGame);

	// Value added for each move available to a given piece
	constexpr Value MOBILITY_BONUS[PT_AMOUNT] = {
		// Pawn
		0, // Moves and attacks are actually different so this isn't useful anyway

		// Rook
		3,

		// Knight
		5,

		// Bishop
		4,

		// Queen
		2,

		// King
		1
	};
}
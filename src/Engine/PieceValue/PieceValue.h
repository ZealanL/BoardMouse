#pragma once
#include "../BitBoard/BitBoard.h"
#include "../Pos/Pos.h"

// Value of a piece or position in centipawns
typedef int64_t Value;

namespace PieceValue {
	Value CalcPieceValue(uint8_t pieceType, uint8_t team, Pos pos, bool isEndGame);
}
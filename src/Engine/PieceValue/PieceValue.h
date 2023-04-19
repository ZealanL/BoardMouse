#pragma once
#include "../BitBoard/BitBoard.h"
#include "../Pos/Pos.h"

namespace PieceValue {
	float CalcPieceValue(uint8_t pieceType, uint8_t team, Pos pos, bool isEndGame);
}
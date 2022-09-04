#pragma once
#include "../BitBoard/BitBoard.h"
#include "../Pos/Pos.h"

namespace LookupGen {
	// Can be called only once, initializes all lookup data
	void InitOnce();
	BitBoard GetRookMoves(Pos rookPos, BitBoard occupiedMask);
	BitBoard GetBishopMoves(Pos bishopPos, BitBoard occupiedMask);
	BitBoard GetKingMoves(Pos kingPos);
	BitBoard GetKnightMoves(Pos knightPos);
	BitBoard GetPawnMoves(Pos pawnPos, uint8 team);
	BitBoard GetPawnAttacks(Pos pawnPos, uint8 team);
}
#pragma once
#include "../BitBoard/BitBoard.h"
#include "../Pos/Pos.h"

namespace LookupGen {
	// Can be called only once, initializes all lookup data
	void InitOnce();

	void GetRookMoves(Pos rookPos, BitBoard occupiedMask, BitBoard& outBaseMoves, BitBoard& outOccludedMoves);
	void GetBishopMoves(Pos bishopPos, BitBoard occupiedMask, BitBoard& outBaseMoves, BitBoard& outOccludedMoves);
	void GetQueenMoves(Pos queenPos, BitBoard occupiedMask, BitBoard& outBaseMoves, BitBoard& outOccludedMoves);

	BitBoard GetKingMoves(Pos kingPos);
	BitBoard GetKnightMoves(Pos knightPos);

	BitBoard GetPawnMoves(Pos pawnPos, uint8_t team);
	BitBoard GetPawnAttacks(Pos pawnPos, uint8_t team);

	// NOTE: Line extends to the edge of the board
	BitBoard GetLineMask(Pos from, Pos to);

	// Like GetLineMask, but DOESN'T extent do the end of the board
	// Like GetBetweenMask, but includes from and to
	BitBoard GetPartialLineMask(Pos from, Pos to);

	BitBoard GetBetweenMask(Pos from, Pos to);
}
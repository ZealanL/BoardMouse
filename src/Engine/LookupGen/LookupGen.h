#pragma once
#include "../BitBoard/BitBoard.h"
#include "../Pos/Pos.h"

// The number of entries in a slider's occlusion lookup array
// Since the bit where the slider is present is ignored, there are 7 possible blocking spaces
// NOTE: To save memory, I could not check any bits along the edge (making this 64^2 instead of 128^2), 
//	however this would require another mask operation for every lookup
#define SLIDER_OCCLUSION_LOOKUP_ENTRY_COUNT (128*128)

namespace LookupGen {

	// Base moves for each piece
	// Memory size: Negligible
	extern BitBoard
		kingMoveLookup[BD_SQUARE_AMOUNT],
		knightMoveLookup[BD_SQUARE_AMOUNT],
		rookMoveLookup[BD_SQUARE_AMOUNT],
		bishopMoveLookup[BD_SQUARE_AMOUNT];

	// Lookup for the move or attack possibilities for any pawn in any position, for either team
	// NOTE: Ignores occlusion and attack availability, that is done elsewhere
	// Memory sizes: Negligible
	extern BitBoard
		pawnMoveLookup[TEAM_AMOUNT][BD_SQUARE_AMOUNT],
		pawnAttackLookup[TEAM_AMOUNT][BD_SQUARE_AMOUNT];

	// Memory size: ~16.8MB for both
	// NOTE: If the edgemask is used (as discussed above), memory size would be only ~4.2MB for both
	//	However, I'd prefer to use an extra 12 MB of memory to remove a lookup operation!
	extern BitBoard
		rookOcclusionLookup[BD_SQUARE_AMOUNT][SLIDER_OCCLUSION_LOOKUP_ENTRY_COUNT],
		bishopOcclusionLookup[BD_SQUARE_AMOUNT][SLIDER_OCCLUSION_LOOKUP_ENTRY_COUNT];

	// Masks of the path between two squares (not including those squares)
	// Only works for paths that are diagonal or straight
	// Memory size: Negligible
	extern BitBoard betweenMasks[BD_SQUARE_AMOUNT][BD_SQUARE_AMOUNT];

	// Masks of the line connecting two squares, from one side of the board to the other
	// Only works for paths that are diagonal or straight
	// Memory size: Negligible
	extern BitBoard lineMasks[BD_SQUARE_AMOUNT][BD_SQUARE_AMOUNT];

	// Masks of the line connecting two squares, but doesn't extend beyond the two points
	// Only works for paths that are diagonal or straight
	// Memory size: Negligible
	extern BitBoard partialLineMasks[BD_SQUARE_AMOUNT][BD_SQUARE_AMOUNT];

	// Masks of all moves for any piece from the current location (aka queen|knight), including the current location
	// Memory size: Negligible
	extern BitBoard updateMasks[BD_SQUARE_AMOUNT];

	// Mask of the entire rank
	// Memory size: Negligible
	extern BitBoard rankMasks[BD_SQUARE_AMOUNT];

	// Values for each piece on each square
	// Memory size: Negligible
	extern float pieceValues[TEAM_AMOUNT][PT_AMOUNT][BD_SQUARE_AMOUNT];

	// Can be called only once, initializes all lookup data
	void InitOnce();

	FINLINE BitBoard GetRookBaseMoves(Pos rookPos) {
		return rookMoveLookup[rookPos];
	}

	FINLINE void GetRookMoves(Pos rookPos, BitBoard occupiedMask, BitBoard& outBaseMoves, BitBoard& outOccludedMoves) {
		BitBoard baseMoves = GetRookBaseMoves(rookPos);
		outBaseMoves = baseMoves;
		BitBoard occlusionBits = occupiedMask.ExtractBits(baseMoves);
		outOccludedMoves = rookOcclusionLookup[rookPos][occlusionBits];
	}

	FINLINE BitBoard GetBishopBaseMoves(Pos rookPos) {
		return bishopMoveLookup[rookPos];
	}

	FINLINE void GetBishopMoves(Pos bishopPos, BitBoard occupiedMask, BitBoard& outBaseMoves, BitBoard& outOccludedMoves) {
		BitBoard baseMoves = GetBishopBaseMoves(bishopPos);
		outBaseMoves = baseMoves;
		BitBoard occlusionBits = occupiedMask.ExtractBits(baseMoves);
		outOccludedMoves = bishopOcclusionLookup[bishopPos][occlusionBits];
	}

	FINLINE BitBoard GetQueenBaseMoves(Pos queenPos) {
		BitBoard
			baseMoves_R = rookMoveLookup[queenPos],
			baseMoves_B = bishopMoveLookup[queenPos];
		return baseMoves_R | baseMoves_B;
	}

	FINLINE void GetQueenMoves(Pos queenPos, BitBoard occupiedMask, BitBoard& outBaseMoves, BitBoard& outOccludedMoves) {
		// TODO: Maybe use seperate lookup(?)

		BitBoard
			baseMoves_R = rookMoveLookup[queenPos],
			baseMoves_B = bishopMoveLookup[queenPos];
		outBaseMoves = baseMoves_R | baseMoves_B;

		BitBoard
			occlusionBits_R = occupiedMask.ExtractBits(baseMoves_R),
			occlusionBits_B = occupiedMask.ExtractBits(baseMoves_B);
		outOccludedMoves =
			rookOcclusionLookup[queenPos][occlusionBits_R] | bishopOcclusionLookup[queenPos][occlusionBits_B];
	}

	FINLINE BitBoard GetKingMoves(Pos kingPos) {
		return kingMoveLookup[kingPos];
	}

	FINLINE BitBoard GetKnightMoves(Pos knightPos) {
		return knightMoveLookup[knightPos];
	}

	FINLINE BitBoard GetPawnMoves(Pos pawnPos, uint8_t team) {
		return pawnMoveLookup[team][pawnPos];
	}

	FINLINE BitBoard GetPawnAttacks(Pos pawnPos, uint8_t team) {
		return pawnAttackLookup[team][pawnPos];
	}

	FINLINE BitBoard GetLineMask(Pos from, Pos to) {
		return lineMasks[from.index][to.index];
	}

	FINLINE BitBoard GetPartialLineMask(Pos from, Pos to) {
		return partialLineMasks[from.index][to.index];
	}

	FINLINE BitBoard GetBetweenMask(Pos from, Pos to) {
		return betweenMasks[from.index][to.index];
	}

	FINLINE BitBoard GetUpdateMask(Pos pos) {
		return updateMasks[pos];
	}

	FINLINE BitBoard GetRankMask(Pos pos) {
		// TODO: Non-lookup is maybe faster?
		return rankMasks[pos];
	}

	FINLINE float GetPieceValue(uint8_t pieceType, Pos pos, uint8_t team) {
		return pieceValues[team][pieceType][pos];
	}
}
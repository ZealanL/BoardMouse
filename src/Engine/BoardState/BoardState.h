#pragma once
#include "../BitBoard/BitBoard.h"
#include "../Pos/Pos.h"

// Stores all info for the state of a chess game
struct BoardState {
	struct {

		BitBoard piecePositions[PT_AMOUNT];
		BitBoard occupy;

		bool canCastleL, canCastleR;
	} teamData[TEAM_AMOUNT];
	
	// Normally blank, has a single bit on when en passant is possible
	BitBoard enPassantMask;
};
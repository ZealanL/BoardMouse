#pragma once
#include "../BitBoard/BitBoard.h"
#include "../Pos/Pos.h"

// Stores all info for the state of a chess game
struct BoardState {
	struct {

		// Has a single bit for the position of the king
		BitBoard kingPos;

		// What squares we have pieces in
		BitBoard occupy;

		// What squares we can attack
		BitBoard attack;

		bool canCastleL, canCastleR;
	} teamData[TEAM_AMOUNT];
	
	// What pieces are at what positions
	uint8 pieceTypes[BD_SQUARE_AMOUNT];

	// Normally blank, has a single bit on when en passant is possible
	BitBoard enPassantMask;
};
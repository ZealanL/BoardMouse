#pragma once
#include "../BitBoard/BitBoard.h"
#include "../Pos/Pos.h"
#include "../PieceValue/PieceValue.h"
#include "../Zobrist/Zobrist.h"
#include "../Move/Move.h"

#define HALF_MOVE_DRAW_COUNT 100

#define UPDATE_VALUES
#define UPDATE_HASHES

// Stores all info for the state of a chess game
struct BoardState {
	struct TeamData {
		Pos kingPos; 

		// What squares we have pieces in
		BitBoard occupy;

		BitBoard pieceSets[PT_AMOUNT];

		// What squares we can attack
		BitBoard attack;

		// What pieces block a sliding piece from attacking our king
		// Only counts if the piece is the only blocker of that path
		// NOTE: Includes enemy pieces
		BitBoard pinnedPieces;

		// Pieces that check the enemy king
		BitBoard checkers;

		// If checkers != 0, the first piece to check the king is here
		Pos firstCheckingPiecePos;

		bool canCastle_Q; // Can castle queen-side (right/+x)
		bool canCastle_K; // Can castle king-side (left/-x)

		float totalValue; // Value of all of our pieces
	};
	TeamData teamData[TEAM_AMOUNT];
	
	// Number of half-moves since a pawn advanced or a piece was captured
	// When counter reaches HALF_MOVE_DRAW_COUNT, its a draw
	uint8_t halfMovesSincePawnOrCapture;

	// What move number we are on, starts at 1
	uint16_t moveNum = 1;

	// Team who's turn it is
	uint8_t turnTeam = TEAM_WHITE;

	// Normally blank, has a single bit on when en passant is possible
	BitBoard enPassantToMask;

	// Position of the pawn that can be captured by en passant
	// NOTE: Only valid if enPassantToMask != 0
	Pos enPassantPawnPos;

	// Which type of piece is at which position
	uint8_t pieceTypes[BD_SQUARE_AMOUNT];

	// The Zobrist hash of this board state, updated incrementally as moves are made
	ZobristHash hash;

	// Values of each piece
	Value pieceValues[BD_SQUARE_AMOUNT];

	// Updates all persistent values, call this when you modify the board beyond ExecuteMove()
	void ForceUpdateAll();

	// Execute a move on the board, update accordingly
	void ExecuteMove(Move move);

	// Update a team's attack and pin masks, within an update mask
	void UpdateAttacksPinsValues(uint8_t team);

	// Execute a move that does nothing and just switches whos turn it is
	void ExecuteNullMove();

	FINLINE bool IsEndgame() {
		// TODO: Improve
		return !teamData[TEAM_WHITE].pieceSets[PT_QUEEN] && !teamData[TEAM_BLACK].pieceSets[PT_QUEEN];
	}

	friend std::ostream& operator <<(std::ostream& stream, const BoardState& boardState);
};
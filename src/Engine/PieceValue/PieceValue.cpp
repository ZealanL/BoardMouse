#include "PieceValue.h"

Value PieceValue::CalcPieceValue(uint8_t pieceType, uint8_t team, Pos pos, bool isEndGame) {
	uint8_t 
		x = pos.X(),
		y = pos.Y();

	// Flip to match
	if (team == TEAM_BLACK)
		y = BD_SIZE - y - 1;
	
	uint8_t
		mirrorX = x < 4 ? x : (BD_SIZE - x - 1);

	constexpr Value PIECE_BASE_VALUES[PT_AMOUNT] = {
		100, // Pawn
		550, // Rook
		310, // Knight
		330, // Bishop
		950, // Queen
		0 // King
	};

	// From https://www.chessprogramming.org/Simplified_Evaluation_Function
	constexpr Value PIECE_BONUS[PT_AMOUNT][BD_SQUARE_AMOUNT / 2] = {
		{ // Pawn
			 0,   0,   0,   0,
			50,  50,  50,  50,
			10,  10,  20,  30,
			 5,   5,  10,  25,
			 0,   0,   0,  20,
			 5,  -5, -10,   0,
			 5,  10,  10, -20,
			 0,   0,   0,   0
		},

		{ // Rook
			  0,   0,   0,   0,
			  5,  10,  10,  10,
			 -5,   0,   0,   0,
			 -5,   0,   0,   0,
			 -5,   0,   0,   0,
			 -5,   0,   0,   0,
			 -5,   0,   0,   0,
			  0,   0,   0,   5
		},

		{ // Knight
			-50, -40, -30, -30,
			-40, -20,   0,   0,
			-30,   0,  10,  15,
			-30,   5,  15,  20,
			-30,   0,  15,  20,
			-30,   5,  10,  15,
			-40, -20,   0,   5,
			-50, -40, -30, -30
		},

		{ // Bishop
			-20, -10, -10, -10,
			-10,   0,   0,   0,
			-10,   0,   5,  10,
			-10,   5,   5,  10,
			-10,   0,  10,  10,
			-10,  10,  10,  10,
			-10,   5,   0,   0,
			-20, -10, -10, -10
		},

		{ // Queen
			-20, -10, -10,  -5,
			-10,   0,   0,   0,
			-10,   0,   5,   5,
			 -5,   0,   5,   5,
			  0,   0,   5,   5,
			-10,   5,   5,   5,
			-10,   0,   5,   0,
			-20, -10, -10,  -5
		},

		{ // King
			-30, -40, -40, -50,
			-30, -40, -40, -50,
			-30, -40, -40, -50,
			-30, -40, -40, -50,
			-20, -30, -30, -40,
			-10, -20, -20, -20,
			 20,  20,   0,   0,
			 20,  30,  10,   0
		}
	};

	constexpr float BONUS_SCALE = 1.0f;

	int mirrorIndex = mirrorX + (y * BD_SIZE / 2);
	return PIECE_BASE_VALUES[pieceType] + (PIECE_BONUS[pieceType][mirrorIndex] * BONUS_SCALE);
}
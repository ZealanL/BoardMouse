#include "PieceValue.h"

Value PieceValue::CalcPieceSquareValue(uint8_t pieceType, uint8_t team, Pos pos, bool isEndGame) {
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

	// Bonus for each square for a given piece
	// Last dimension is if this is an endgame or not
	// From https://www.chessprogramming.org/Simplified_Evaluation_Function
	constexpr Value PIECE_BONUS[PT_AMOUNT][2][BD_SQUARE_AMOUNT / 2] = {
		{ // Pawn
			{ // Middlegame
				// Push pawns to the center
				 0,   0,   0,   0,
				50,  50,  50,  50,
				10,  10,  20,  30,
				 5,   5,  10,  25,
				 0,   0,   5,  25,
				 5,  -5,   5,  10,
				 5,   0,   0, -35,
				 0,   0,   0,   0
			},

			{ // Endgame
				// Pawns in the center matters a bit less
				// Prioritize promotion - 7th rank pawn is worth a lot
				 0,   0,   0,   0,
			   150, 150, 150, 150,
				80,  80,  80,  80,
				40,  40,  40,  40,
				15,  15,  15,  15,
				 5,   5,   5,   5,
				 3,   3,   3,   3,
				 0,   0,   0,   0
			}
		},

		{ // Rook
			{ // Middlegame
				// Being on the edge is kind of lame
			     0,   0,   0,   0,
			     5,  10,  10,  10,
			    -5,   0,   0,   0,
			    -5,   0,   0,   0,
			    -5,   0,   0,   0,
			    -5,   0,   0,   0,
			    -5,   0,   0,   0,
			     0,   0,   0,   5
			},

			{ // Endgame
				// 7th rank is best, 6th or 8th is still good
				 3,   5,   5,   5,
				 7,  15,  15,  15,
				 5,  10,  10,  10,
				 3,   5,   5,   5,
				 0,   0,   0,   0,
				 0,   0,   0,   0,
				 0,   0,   0,   0,
				 5,   5,   5,   5
			}
		},

		{ // Knight
			// Same for both
			{
				-50, -40, -30, -30,
				-40, -20,   0,   0,
				-30,   0,  10,  15,
				-30,   5,  15,  20,
				-30,   0,  15,  20,
				-30,   5,  15,  15,
				-40, -20,   0,   0,
				-50, -40, -30, -30
			},
			{
				-50, -40, -30, -30,
				-40, -20,   0,   0,
				-30,   0,  10,  15,
				-30,   5,  15,  20,
				-30,   0,  15,  20,
				-30,   5,  10,  15,
				-40, -20,   0,   5,
				-50, -40, -30, -30
			},
		},

		{ // Bishop
			// Same for both
			{
				-20, -10, -10, -10,
				-10,   0,   0,   0,
				-10,   0,   5,  10,
				-10,   5,   5,  10,
				-10,   0,  10,  10,
				-10,  10,  10,  10,
				-10,   5,   0,   0,
				-20, -10, -10, -10
			},
			{
				-20, -10, -10, -10,
				-10,   0,   0,   0,
				-10,   0,   5,  10,
				-10,   5,   5,  10,
				-10,   0,  10,  10,
				-10,  10,  10,  10,
				-10,   5,   0,   0,
				-20, -10, -10, -10
			},
		},

		{ // Queen
			{ // Middlegame
				// Stay away from the edges, center is kinda good
				-20, -10, -10,  -5,
				-10,   0,   0,   0,
				-10,   0,   5,   5,
				 -5,   0,   5,   5,
				  0,   0,   5,   5,
				-10,   5,   5,   5,
				-10,   0,   5,   5,
				-20, -10, -10,   0
			},

			{ // Endgame
				// Get out there!
				  0,   0,   0,   0,
				  0,   0,   5,   5,
				  0,   5,  10,  12,
				  0,   5,  10,  12,
				  0,   5,  10,  12,
				  0,   5,   5,   5,
				  0,   0,   5,   5,
				  0,   0, -10, -10
			}
		},

		{ // King
			{ // Middlegame
				// Stay away from the action, castling is optimal
				-30, -40, -40, -50,
				-30, -40, -40, -50,
				-30, -40, -40, -50,
				-30, -40, -40, -50,
				-20, -30, -30, -40,
				-10, -20, -20, -20,
				 20,  20,   0,   0,
				 10,  15,  10,   0
			},

			{ // Endgame
				// Get out there!
				  0,   0,   0,   0,
				  0,   0,   5,   5,
				  0,   5,   7,   7,
				  0,   5,   7,   7,
				  0,   5,   7,   7,
				  0,   5,   5,   5,
				  0,   0,   5,   5,
				  0,   0,   0,   0
			}
		}
	};

	constexpr float BONUS_SCALE = 1.0f;

	int mirrorIndex = mirrorX + ((7 - y) * BD_SIZE / 2);
	return PIECE_BASE_VALUES[pieceType] + (PIECE_BONUS[pieceType][isEndGame][mirrorIndex] * BONUS_SCALE);
}
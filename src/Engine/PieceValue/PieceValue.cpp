#include "PieceValue.h"

float PieceValue::CalcPieceValue(uint8_t pieceType, uint8_t team, Pos pos, bool isEndGame) {
	uint8_t 
		x = pos.X(),
		y = pos.Y();

	// Flip to match
	if (team == TEAM_BLACK)
		y = BD_SIZE - y - 1;
	
	float 
		xRatio = x / (float)(BD_SIZE - 1),
		yRatio = y / (float)(BD_SIZE - 1);

	float 
		edgeRatioX = abs(0.5f - xRatio) * 2,
		edgeRatioY = abs(0.5f - yRatio) * 2;

	float edgeRatio2D = MAX(edgeRatioX, edgeRatioY);
	
	constexpr float PIECE_BASE_VALUES[PT_AMOUNT] = {
		1.f, // Pawn
		5.5f, // Rook
		3.1f, // Knight
		3.3f, // Bishop
		9.5f, // Queen
		1.f // King
	};

	float val = PIECE_BASE_VALUES[pieceType];

#if 1
	constexpr int centipawnMaps[PT_AMOUNT][BD_SQUARE_AMOUNT] = {
		{
			0,  0,  0,  0,  0,  0,  0,  0,
			50, 50, 50, 50, 50, 50, 50, 50,
			10, 10, 20, 30, 30, 20, 10, 10,
			 5,  5, 10, 25, 25, 10,  5,  5,
			 0,  0,  0, 20, 20,  0,  0,  0,
			 5, -5,-10,  0,  0,-10, -5,  5,
			 5, 10, 10,-20,-20, 10, 10,  5,
			 0,  0,  0,  0,  0,  0,  0,  0
		},

		{
			  0,  0,  0,  0,  0,  0,  0,  0,
			  5, 10, 10, 10, 10, 10, 10,  5,
			 -5,  0,  0,  0,  0,  0,  0, -5,
			 -5,  0,  0,  0,  0,  0,  0, -5,
			 -5,  0,  0,  0,  0,  0,  0, -5,
			 -5,  0,  0,  0,  0,  0,  0, -5,
			 -5,  0,  0,  0,  0,  0,  0, -5,
			  0,  0,  0,  5,  5,  0,  0,  0
		},

		{
			-50,-40,-30,-30,-30,-30,-40,-50,
			-40,-20,  0,  0,  0,  0,-20,-40,
			-30,  0, 10, 15, 15, 10,  0,-30,
			-30,  5, 15, 20, 20, 15,  5,-30,
			-30,  0, 15, 20, 20, 15,  0,-30,
			-30,  5, 10, 15, 15, 10,  5,-30,
			-40,-20,  0,  5,  5,  0,-20,-40,
			-50,-40,-30,-30,-30,-30,-40,-50
		},

		{
			-20,-10,-10, -5, -5,-10,-10,-20,
			-10,  0,  0,  0,  0,  0,  0,-10,
			-10,  0,  5,  5,  5,  5,  0,-10,
			 -5,  0,  5,  5,  5,  5,  0, -5,
			  0,  0,  5,  5,  5,  5,  0, -5,
			-10,  5,  5,  5,  5,  5,  0,-10,
			-10,  0,  5,  0,  0,  0,  0,-10,
			-20,-10,-10, -5, -5,-10,-10,-20
		},

		{
			-30,-40,-40,-50,-50,-40,-40,-30,
			-30,-40,-40,-50,-50,-40,-40,-30,
			-30,-40,-40,-50,-50,-40,-40,-30,
			-30,-40,-40,-50,-50,-40,-40,-30,
			-20,-30,-30,-40,-40,-30,-30,-20,
			-10,-20,-20,-20,-20,-20,-20,-10,
			 20, 20,  0,  0,  0,  0, 20, 20,
			 20, 30, 10,  0,  0, 10, 30, 20
		}
	};
	return val;
	return val + (centipawnMaps[pieceType][Pos(x, y)] / 100.f);
#else

	float bonus = 0;

	switch (pieceType) {
	case PT_PAWN:
	{
		// Pawns should be in the middle, the closer to promotion the better
		bonus = ((1 - edgeRatio2D) * 0.8f);
	}
	break;
	case PT_ROOK:
	{
		// Rooks should be on the top and bottom of the board, but not on the sides as much
		bonus = (edgeRatioY * 0.35f) + ((1 - edgeRatioX) * 0.15f);
	}
	break;
	case PT_KNIGHT:
	{
		// Knights should be centralized
		bonus = ((1 - edgeRatio2D) * 0.3f);
	}
	break;
	case PT_BISHOP:
	{
		// Bishops should be in the center-ish
		bonus = ((1 - edgeRatio2D) * 0.25f);
	}
	break;
	case PT_QUEEN:
	{
		if (isEndGame) {
			// Queen should be centralized
			bonus = ((1 - edgeRatio2D) * 0.5f);
		} else {
			// Queen should be a tiny bit centralized
			bonus = ((1 - edgeRatio2D) * 0.08f);
		}
	}
	break;
	default: // King
	{
		if (isEndGame) {
			// King should be centralized
			bonus = ((1 - edgeRatio2D) * 0.7f);
		} else {
			// King should be back and to the side
			float backBonus = ((1 - yRatio) * 1.3f);
			float edgeBonus = (edgeRatioX * 0.5f);
			if (yRatio == 0 && edgeRatioX == 1)
				edgeBonus *= 0.65f; // We don't actually want to be in the exact corner

			bonus = backBonus + edgeBonus;
		}
	}
	}
	return val + (val * bonus);
#endif
}
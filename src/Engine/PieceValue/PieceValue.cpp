#include "PieceValue.h"

Value PieceValue::CalcPieceValue(uint8_t pieceType, uint8_t team, Pos pos, bool isEndGame) {
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

	return (Value)(val * 100);
}
#include "MoveRating.h"

#include "../LookupGen/LookupGen.h"

void MoveRating::RateMoves(BoardState& boardState, MoveList& moves) {
	uint8_t team = boardState.turnTeam;
	auto& td = boardState.teamData[team];
	auto& etd = boardState.teamData[!team];
	for (Move& move : moves) {

		Value rating = 0;

		// Captures always have a decent bonus
		if (move.flags | Move::FL_CAPTURE)
			rating += 100;
			
		// If normal capture, add change in value to rating
		if (etd.occupy[move.to]) {
			rating += boardState.pieceValues[move.to];
			if (etd.attack[move.to]) {
				// This piece will probably be lost
				rating -= boardState.pieceValues[move.from];
			}
		}

		{ // Add improvement of piece-square value to rating
			// TODO: Support endgame
			Value
				fromSquareValue = LookupGen::GetPieceSquareValue(move.originalPiece, move.from, !team, false),
				toSquareValue = LookupGen::GetPieceSquareValue(move.originalPiece, move.to, !team, false);

			rating += toSquareValue - fromSquareValue;
		}

		move.moveRating = CLAMP(rating, INT16_MIN, INT16_MAX);
	}
}
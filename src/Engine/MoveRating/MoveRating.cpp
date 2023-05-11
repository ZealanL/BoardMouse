#include "MoveRating.h"

#include "../LookupGen/LookupGen.h"

void MoveRating::RateMoves(BoardState& boardState, MoveList& moves, ButterflyBoard& butterflyBoard) {

	constexpr Value
		CAPTURE_BASE_BONUS = 100,
		PROMOTION_QUEEN_BASE_BONUS = 1000;

	uint8_t team = boardState.turnTeam;
	auto& td = boardState.teamData[team];
	auto& etd = boardState.teamData[!team];

	bool isEndgame = boardState.IsEndgame();

	for (Move& move : moves) {
		Value rating = 0;

		{ // Add base bonuses
			if (move.flags | Move::FL_CAPTURE)
				rating += CAPTURE_BASE_BONUS;

			if ((move.flags | Move::FL_PROMOTION) && (move.resultPiece == PT_QUEEN))
				rating += PROMOTION_QUEEN_BASE_BONUS;
		}

		bool
			isNormalCapture = etd.occupy[move.to],
			fromAttacked	= etd.attack[move.from],
			toAttacked		= etd.attack[move.to],
			fromDefended	= td.attack[move.from],
			toDefended		= td.attack[move.to];

		// If normal capture, add captured piece value to rating
		if (isNormalCapture) {
			rating += boardState.pieceValues[move.to];
		}

		// Prioritize the evasion of captures
		if (fromAttacked) {
			Value evasionValue = boardState.pieceValues[move.from];
			if (fromDefended)
				evasionValue /= 2; // We are defended, so moving is less important
			rating += evasionValue;
		}

		// If we move to a square the enemy attacks, this piece will probably be lost
		if (toAttacked) {
			rating -= boardState.pieceValues[move.from];
		}

		{ // Add improvement of piece-square value to rating
			Value
				fromSquareValue = LookupGen::GetPieceSquareValue(move.originalPiece, move.from, !team, isEndgame),
				toSquareValue = LookupGen::GetPieceSquareValue(move.originalPiece, move.to, !team, isEndgame);

			rating += toSquareValue - fromSquareValue;
		}

		// Add butterfly bonus
		rating += butterflyBoard.data[team][move.from][move.to];

		move.moveRating = CLAMP(rating, INT16_MIN, INT16_MAX);
	}
}
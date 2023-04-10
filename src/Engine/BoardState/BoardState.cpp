#include "BoardState.h"
#include "../LookupGen/LookupGen.h"

void BoardState::UpdateAttacksAndPins(uint8_t team) {
	auto& td = teamData[team];
	auto& etd = teamData[!team];
	BitBoard combinedOccupy = td.occupy | etd.occupy;

	// Clear attacks/pins
	td.attack = etd.pinnedPieces = 0;

	const auto fnUpdatePins = [&](Pos pinnerPos) {
		BitBoard betweenMask = LookupGen::GetBetweenMask(pinnerPos, etd.kingPos);
		BitBoard pinnedEnemyPieces = betweenMask & etd.occupy;

		if (pinnedEnemyPieces.BitCount() == 1) {
			etd.pinnedPieces |= pinnedEnemyPieces;
		}
	};

	// Loop over all of our pieces
	td.occupy.Iterate(
		[&](uint64_t i) {
			uint8_t pieceType = pieceTypes[i];

			switch (pieceType) {
			case PT_PAWN:
				td.attack |= LookupGen::GetPawnAttacks(i, team);
				break;
			case PT_KNIGHT:
				td.attack |= LookupGen::GetKnightMoves(i);
				break;
			case PT_ROOK:
			{
				BitBoard baseMoves, occludedMoves;
				LookupGen::GetRookMoves(i, etd.occupy, baseMoves, occludedMoves);

				if (baseMoves & etd.kingPosMask)
					fnUpdatePins(i);

				td.attack |= occludedMoves;
			}
			break;
			case PT_BISHOP:
			{
				BitBoard baseMoves, occludedMoves;
				LookupGen::GetBishopMoves(i, etd.occupy, baseMoves, occludedMoves);

				if (baseMoves & etd.kingPosMask)
					fnUpdatePins(i);

				td.attack |= occludedMoves;
			}
			case PT_QUEEN:
			{
				BitBoard baseMoves, occludedMoves;
				LookupGen::GetQueenMoves(i, etd.occupy, baseMoves, occludedMoves);

				if (baseMoves & etd.kingPosMask)
					fnUpdatePins(i);

				td.attack |= occludedMoves;
			}
			default: // King
				td.attack |= LookupGen::GetKingMoves(i);
				break;
			}
		}
	);
}

void BoardState::ExecuteMove(Move move, uint8_t team) {
	auto& td = teamData[team];
	auto& etd = teamData[!team];

	BitBoard fromMaskInv = ~(1ull << move.from), toMask = (1ull << move.to);

	ASSERT(td.occupy[move.from] && !td.occupy[move.to]);

	// Update moved piece
	pieceTypes[move.to] = move.resultPiece;

	// En passant capture
	if (move.resultPiece == PT_PAWN) {
		if (toMask & enPassantMask) {
			// Remove piece behind our pawn
			etd.occupy &= ~(team == TEAM_WHITE ? toMask >> BD_SIZE : toMask << BD_SIZE);
			enPassantMask = 0;
		} else if (abs(move.to - move.from) > BD_SIZE + 1) {
			// Double pawn move, set en passant mask behind us
			enPassantMask = (team == TEAM_WHITE ? toMask >> BD_SIZE : toMask << BD_SIZE);
		} else {
			enPassantMask = 0;
		}
	} else {
		enPassantMask = 0;

		if (move.resultPiece == PT_KING) {
			{ // Update king pos
				td.kingPos = move.to;
				td.kingPosMask = toMask;
				td.canCastle_K = td.canCastle_Q = false;
			}

			int xDelta = move.to.X() - move.from.X();
			if ((xDelta & 1) == 0) {
				int castleIndex = xDelta > 0;
				uint8_t castleFromX = (xDelta > 0 ? BD_SIZE - 1 : 0);

				// We are castling!
				// Move the rook...
				Pos rookFromPos = Pos((xDelta > 0 ? BD_SIZE - 1 : 0), move.from.Y());
				Pos rookToPos = move.from.index + xDelta / 2;
				pieceTypes[rookToPos] = PT_ROOK;
				td.occupy &= ~(1ull << rookFromPos);
				td.occupy |= (1ull << rookToPos);
			}
		}
	}

	// Update occupy
	td.occupy &= fromMaskInv;
	td.occupy |= toMask;
	etd.occupy &= ~toMask;

	UpdateAttacksAndPins(team);
}
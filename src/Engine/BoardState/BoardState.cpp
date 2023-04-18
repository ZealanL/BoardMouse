#include "BoardState.h"
#include "../LookupGen/LookupGen.h"

void BoardState::UpdateAttacksAndPinsPartial(uint8_t team, BitBoard updateMask) {

	BitBoard invUpdateMask = ~updateMask;

	auto& td = teamData[team];
	auto& etd = teamData[!team];
	BitBoard combinedOccupy = td.occupy | etd.occupy;

	// Clear attacks/pins/checking pieces
	td.attack &= invUpdateMask; 
	etd.pinnedPieces &= invUpdateMask;
	td.checkers &= invUpdateMask;

	const auto fnUpdatePins = [&](Pos pinnerPos) {
		BitBoard betweenMask = LookupGen::GetBetweenMask(pinnerPos, etd.kingPos);
		BitBoard pinnedEnemyPieces = betweenMask & etd.occupy;

		if (pinnedEnemyPieces.BitCount() == 1) {
			etd.pinnedPieces |= pinnedEnemyPieces;
		}
	};

	// Loop over all of our pieces
	BitBoard occupyUpdate = td.occupy & updateMask;
	occupyUpdate.Iterate(
		[&](uint64_t i) {
			uint8_t pieceType = pieceTypes[i];

			BitBoard moves;

			switch (pieceType) {
			case PT_PAWN:
			{
				moves = LookupGen::GetPawnAttacks(i, team);
			}
			break;
			case PT_KNIGHT:
			{
				moves = LookupGen::GetKnightMoves(i);
			}
			break;
			case PT_ROOK:
			{
				BitBoard baseMoves;
				LookupGen::GetRookMoves(i, combinedOccupy & ~etd.kingPosMask, baseMoves, moves);

				if (baseMoves & etd.kingPosMask)
					fnUpdatePins(i);
			}
			break;
			case PT_BISHOP:
			{
				BitBoard baseMoves;
				LookupGen::GetBishopMoves(i, combinedOccupy & ~etd.kingPosMask, baseMoves, moves);

				if (baseMoves & etd.kingPosMask)
					fnUpdatePins(i);
			}
			break;
			case PT_QUEEN:
			{
				BitBoard baseMoves;
				LookupGen::GetQueenMoves(i, combinedOccupy & ~etd.kingPosMask, baseMoves, moves);

				if (baseMoves & etd.kingPosMask)
					fnUpdatePins(i);
			}
			break;
			default: // King
				moves = LookupGen::GetKingMoves(i);
				td.attack |= LookupGen::GetKingMoves(i);
			}

			td.attack |= moves;
			if (moves & etd.kingPosMask) {
				if (!td.checkers)
					td.firstCheckingPiecePos = i;
				
				td.checkers.Set(i, true);
			}
		}
	);
}

// TODO: Creates a bug somehow
#define USE_PARTIAL_UPDATES

void BoardState::ExecuteMove(Move move) {
	auto& td = teamData[turnTeam];
	auto& etd = teamData[!turnTeam];

	BitBoard fromMaskInv = ~(1ull << move.from), toMask = (1ull << move.to);

#ifdef _DEBUG
	if (!td.occupy[move.from] || td.occupy[move.to]) {
		ERR_CLOSE("Tried to execute invalid move " << move << " for team " << TEAM_NAMES[turnTeam] << " on BoardState " << *this);
	}
#endif

	// Update moved piece
	pieceTypes[move.to] = move.resultPiece;

#ifdef USE_PARTIAL_UPDATES
	BitBoard updateMask = LookupGen::GetUpdateMask(move.from) | LookupGen::GetUpdateMask(move.to);
#endif

	// En passant capture
	if (move.resultPiece == PT_PAWN) {
		if (toMask == enPassantToMask) {
			// Remove piece behind our pawn
			etd.occupy.Set(enPassantPawnPos, 0);
#ifdef USE_PARTIAL_UPDATES
			updateMask |= LookupGen::GetUpdateMask(enPassantPawnPos);
			updateMask |= LookupGen::GetUpdateMask(
				enPassantPawnPos.index + (turnTeam == TEAM_WHITE ? -BD_SIZE : BD_SIZE)
			);
#endif
			enPassantToMask = 0;
		} else if (abs(move.to - move.from) > BD_SIZE + 1) {
			// Double pawn move, set en passant mask behind us
			enPassantToMask = (turnTeam == TEAM_WHITE ? toMask >> BD_SIZE : toMask << BD_SIZE);
			enPassantPawnPos = move.to;
		} else {
			enPassantToMask = 0;
		}
	} else {
		enPassantToMask = 0;

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
#ifdef USE_PARTIAL_UPDATES
				updateMask |= LookupGen::GetUpdateMask(rookFromPos);
				updateMask |= LookupGen::GetUpdateMask(rookToPos);
#endif
			}
		}
	}

	// Update occupy
	td.occupy &= fromMaskInv;
	td.occupy |= toMask;
	etd.occupy &= ~toMask;
#ifdef USE_PARTIAL_UPDATES
	UpdateAttacksAndPinsPartial(turnTeam, updateMask);
#else
	UpdateAttacksAndPins(turnTeam);
#endif;
	turnTeam = !turnTeam;
}

std::ostream& operator <<(std::ostream& stream, const BoardState& boardState) {
	stream << "{\n";
	for (int y = BD_SIZE - 1; y >= 0; y--) {
		stream << "\t";
		for (int x = 0; x < BD_SIZE; x++) {

			if (x > 0)
				stream << ' ';

			Pos pos = Pos(x, y);

			bool isValid = true;
			int squareTeam = -1;

			for (int i = 0; i < 2; i++) {
				if (boardState.teamData[i].occupy[pos]) {
					if (squareTeam == -1) {
						squareTeam = i;
					} else {
						isValid = false;
					}
				}
			}

			if (isValid) {
				if (squareTeam != -1) {
					char pieceChar = PT_CHARS[boardState.pieceTypes[pos]];
					if (squareTeam == TEAM_WHITE)
						pieceChar = toupper(pieceChar);
					
					stream << pieceChar;
				} else {
					stream << '.';
				}
			} else {
				stream << '?';
			}
		}
		stream << std::endl;
	}
	stream << "}";

	return stream;
}
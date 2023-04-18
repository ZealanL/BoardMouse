#include "BoardState.h"
#include "../LookupGen/LookupGen.h"

void BoardState::UpdateAttacksAndPins(uint8_t team
#ifdef USE_PARTIAL_UPDATES
	, BitBoard updateMask
#endif
	) {

#ifdef USE_PARTIAL_UPDATES
	BitBoard invUpdateMask = ~updateMask;
#endif

	auto& td = teamData[team];
	auto& etd = teamData[!team];
	BitBoard combinedOccupy = td.occupy | etd.occupy;

	// Clear attacks/pins/checking pieces
#ifdef USE_PARTIAL_UPDATES
	td.attack &= invUpdateMask; 
	etd.pinnedPieces &= invUpdateMask;
	td.checkers &= invUpdateMask;
#else
	td.attack = etd.pinnedPieces = td.checkers = 0;
#endif

	const auto fnUpdatePins = [&](Pos pinnerPos) {
		BitBoard betweenMask = LookupGen::GetBetweenMask(pinnerPos, etd.kingPos);
		BitBoard pinnedPieces = betweenMask & combinedOccupy;

		if (pinnedPieces.BitCount() == 1)
			etd.pinnedPieces |= pinnedPieces;
	};

	// Loop over all of our pieces
	BitBoard occupyUpdate = td.occupy;
#ifdef USE_PARTIAL_UPDATES
	occupyUpdate &= updateMask;
#endif
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

void BoardState::ExecuteMove(Move move) {
	auto& td = teamData[turnTeam];
	auto& etd = teamData[!turnTeam];

	BitBoard fromMaskInv = ~(1ull << move.from), toMask = (1ull << move.to);

#ifdef _DEBUG
	if (!td.occupy[move.from] || td.occupy[move.to]) {
		ERR_CLOSE("Tried to execute invalid move " << move << " for team " << TEAM_NAMES[turnTeam] << " on BoardState " << *this);
	}
#endif

	uint8_t piece = pieceTypes[move.from];

	// Update moved piece
	pieceTypes[move.to] = move.resultPiece;

#ifdef USE_PARTIAL_UPDATES
	BitBoard updateMask = LookupGen::GetUpdateMask(move.from) | LookupGen::GetUpdateMask(move.to);
#endif

	// En passant capture
	if (piece == PT_PAWN) {
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

		if (piece == PT_KING) {
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

	// Order: Queen-side, King-side
	constexpr uint8_t CASTLING_ROOK_LOCS[TEAM_AMOUNT][2] = {
		{ // White 
			ANI('A1'), ANI('H1')
		},

		{ // Black
			ANI('A8'), ANI('H8')
		}
	};

	if (td.canCastle_Q && move.from == CASTLING_ROOK_LOCS[turnTeam][0])
		td.canCastle_Q = false;

	if (td.canCastle_K && move.from == CASTLING_ROOK_LOCS[turnTeam][1])
		td.canCastle_K = false;

	if (etd.canCastle_Q && move.to == CASTLING_ROOK_LOCS[!turnTeam][0])
		etd.canCastle_Q = false;

	if (etd.canCastle_K && move.to == CASTLING_ROOK_LOCS[!turnTeam][1])
		etd.canCastle_K = false;

	// Update occupy
	td.occupy &= fromMaskInv;
	td.occupy |= toMask;
	etd.occupy &= ~toMask;
#ifdef USE_PARTIAL_UPDATES
	UpdateAttacksAndPins(turnTeam, updateMask);
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
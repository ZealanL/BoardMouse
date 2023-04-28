#include "BoardState.h"
#include "../LookupGen/LookupGen.h"

template <uint8_t TEAM>
FINLINE void _UpdateAttacksAndPins(BoardState& board) {

	auto& td = board.teamData[TEAM];
	auto& etd = board.teamData[!TEAM];
	BitBoard combinedOccupy = td.occupy | etd.occupy;

	// Clear attacks/pins/checking pieces
	td.attack = etd.pinnedPieces = td.checkers = 0;

	const auto fnUpdatePins = [&](Pos pinnerPos) {
		BitBoard betweenMask = LookupGen::GetBetweenMask(pinnerPos, etd.kingPos);
		BitBoard pinnedPieces = betweenMask & combinedOccupy;

		if (pinnedPieces.BitCount() == 1)
			etd.pinnedPieces |= pinnedPieces;
	};

	BitBoard kingPosMask = etd.pieceSets[PT_KING];

	td.pieceSets[PT_PAWN].Iterate(
		[&](uint64_t i) {
			BitBoard moves = LookupGen::GetPawnAttacks(i, TEAM);

			td.attack |= moves;
			if (moves & kingPosMask) {
				if (!td.checkers)
					td.firstCheckingPiecePos = i;

				td.checkers.Set(i, true);
			}
		}
	);

	td.pieceSets[PT_ROOK].Iterate(
		[&](uint64_t i) {
			BitBoard moves, baseMoves;
			LookupGen::GetRookMoves(i, combinedOccupy & ~kingPosMask, baseMoves, moves);

			if (baseMoves & kingPosMask) {
				fnUpdatePins(i);
				if (moves & kingPosMask) {
					if (!td.checkers)
						td.firstCheckingPiecePos = i;

					td.checkers.Set(i, true);
				}
			}

			td.attack |= moves;
			
		}
	);

	td.pieceSets[PT_KNIGHT].Iterate(
		[&](uint64_t i) {
			BitBoard moves = LookupGen::GetKnightMoves(i);
			td.attack |= moves;
			if (moves & kingPosMask) {
				if (!td.checkers)
					td.firstCheckingPiecePos = i;

				td.checkers.Set(i, true);
			}
		}
	);

	td.pieceSets[PT_BISHOP].Iterate(
		[&](uint64_t i) {
			BitBoard moves, baseMoves;
			LookupGen::GetBishopMoves(i, combinedOccupy & ~kingPosMask, baseMoves, moves);

			if (baseMoves & kingPosMask) {
				fnUpdatePins(i);
				if (moves & kingPosMask) {
					if (!td.checkers)
						td.firstCheckingPiecePos = i;

					td.checkers.Set(i, true);
				}
			}

			td.attack |= moves;
		}
	);

	td.pieceSets[PT_QUEEN].Iterate(
		[&](uint64_t i) {
			BitBoard moves, baseMoves;
			LookupGen::GetQueenMoves(i, combinedOccupy & ~kingPosMask, baseMoves, moves);

			if (baseMoves & kingPosMask) {
				fnUpdatePins(i);

				if (moves & kingPosMask) {
					if (!td.checkers)
						td.firstCheckingPiecePos = i;

					td.checkers.Set(i, true);
				}
			}

			td.attack |= moves;
		}
	);

	{ // King
		td.attack |= LookupGen::GetKingMoves(td.kingPos);
	}
}

void BoardState::UpdateAttacksAndPins(uint8_t team) {
	if (team == TEAM_WHITE) {
		_UpdateAttacksAndPins<TEAM_WHITE>(*this);
	} else {
		_UpdateAttacksAndPins<TEAM_BLACK>(*this);
	}
}

void BoardState::ExecuteMove(Move move) {
	auto& td = teamData[turnTeam];
	auto& etd = teamData[!turnTeam];

	BitBoard 
		fromMaskInv = ~(1ull << move.from), 
		toMask = (1ull << move.to),
		toMaskInv = ~toMask;

#ifdef _DEBUG
	bool isBad = false;
	if (!td.occupy[move.from] || td.occupy[move.to]) {
		isBad = true;
	} else if (etd.kingPos == move.to) {
		isBad = true;
	}

	if (isBad) {
		ERR_CLOSE("Tried to execute invalid move " << move << " for team " << TEAM_NAMES[turnTeam] << " on BoardState " << *this);
	}
#endif

#ifdef UPDATE_HASHES
	// Remove castling rights from hash
	hash ^= LookupGen::HashCastleRights(turnTeam, td.canCastle_Q, td.canCastle_K);
	hash ^= LookupGen::HashCastleRights(!turnTeam, etd.canCastle_Q, etd.canCastle_K);
#endif

	Pos newEnPassantPos = 0;
	uint64_t newEnPassantMask = 0;

	// En passant capture
	if (move.originalPiece == PT_PAWN) {
		if (toMask == enPassantToMask) {
			// Remove piece behind our pawn
			etd.occupy.Set(enPassantPawnPos, 0);
			etd.pieceSets[PT_PAWN].Set(enPassantPawnPos, 0);

#ifdef UPDATE_HASHES
			// Remove pawn hash
			hash ^= LookupGen::HashPiece(PT_ROOK, enPassantPawnPos, !turnTeam);
#endif

#ifdef UPDATE_VALUES
			// Remove pawn value
			td.totalValue -= LookupGen::GetPieceValue(PT_PAWN, enPassantPawnPos, !turnTeam);
#endif

		} else if (abs(move.to - move.from) > BD_SIZE + 1) {
			// Double pawn move, set en passant mask behind us
			newEnPassantMask = (turnTeam == TEAM_WHITE ? toMask >> BD_SIZE : toMask << BD_SIZE);
			newEnPassantPos = move.to;
		}
	} else if (move.originalPiece == PT_KING) {
		{ // Update king pos
			td.kingPos = move.to;
			td.canCastle_K = td.canCastle_Q = false;
		}

		int xDelta = move.to.X() - move.from.X();
		if (xDelta && ((xDelta & 1) == 0)) {
			int castleIndex = xDelta > 0;
			uint8_t castleFromX = (xDelta > 0 ? BD_SIZE - 1 : 0);

			// We are castling!
			// Move the rook...
			Pos rookFromPos = Pos((xDelta > 0 ? BD_SIZE - 1 : 0), move.from.Y());
			Pos rookToPos = move.from.index + xDelta / 2;
			BitBoard rookFlipMask = (1ull << rookFromPos) | (1ull << rookToPos);

			td.pieceSets[PT_ROOK] ^= rookFlipMask;
			td.occupy ^= rookFlipMask;

#ifdef UPDATE_VALUES
			// Update rook value
			td.totalValue -= pieceValues[rookFromPos];
			size_t newValue = LookupGen::GetPieceValue(PT_ROOK, rookToPos, turnTeam);
			pieceValues[move.to] = newValue;
			td.totalValue += newValue;
#endif

#ifdef UPDATE_HASHES
			// Update rook hash
			hash ^= LookupGen::HashPiece(PT_ROOK, rookFromPos, turnTeam);
			hash ^= LookupGen::HashPiece(PT_ROOK, rookToPos, turnTeam);
#endif
		}
	}

	if (newEnPassantMask != enPassantToMask) {

#ifdef UPDATE_HASHES
		// Update en passant hashes
		hash ^= LookupGen::HashEnPassant(enPassantToMask != 0, enPassantPawnPos);
		hash ^= LookupGen::HashEnPassant(newEnPassantMask != 0, newEnPassantPos);
#endif

		enPassantToMask = newEnPassantMask;
		enPassantPawnPos = newEnPassantPos;
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

#ifdef UPDATE_VALUES
	{ // Update piece values
		if (etd.occupy[move.to])
			etd.totalValue -= pieceValues[move.to];

		td.totalValue -= pieceValues[move.from];
		Value newValue = LookupGen::GetPieceValue(move.resultPiece, move.to, turnTeam);
		pieceValues[move.to] = newValue;
		td.totalValue += newValue;
	}
#endif

	constexpr uint64_t
		CASTLING_ALL_ROOK_LOCS = ANI_BM('A1') | ANI_BM('H1') | ANI_BM('A8') | ANI_BM('H8'),
		CASTLING_ALL_ROOK_LOCS_INV = ~CASTLING_ALL_ROOK_LOCS;

	if (fromMaskInv & CASTLING_ALL_ROOK_LOCS_INV) {
		if (td.canCastle_Q && move.from == CASTLING_ROOK_LOCS[turnTeam][0]) {
			td.canCastle_Q = false;
		}

		if (td.canCastle_K && move.from == CASTLING_ROOK_LOCS[turnTeam][1]) {
			td.canCastle_K = false;
		}
	}

	if (toMask & CASTLING_ALL_ROOK_LOCS) {
		if (etd.canCastle_Q && move.to == CASTLING_ROOK_LOCS[!turnTeam][0]) {
			etd.canCastle_Q = false;
		}

		if (etd.canCastle_K && move.to == CASTLING_ROOK_LOCS[!turnTeam][1]) {
			etd.canCastle_K = false;
		}
	}

#ifdef UPDATE_HASHES
	// Re-add castling rights from hash
	hash ^= LookupGen::HashCastleRights(turnTeam, td.canCastle_Q, td.canCastle_K);
	hash ^= LookupGen::HashCastleRights(!turnTeam, etd.canCastle_Q, etd.canCastle_K);
#endif

	// Update occupy
	td.occupy &= fromMaskInv;
	td.occupy |= toMask;
	td.pieceSets[move.originalPiece].Set(move.from, 0);
	td.pieceSets[move.resultPiece].Set(move.to, 1);

#ifdef UPDATE_HASHES
	hash ^= LookupGen::HashPiece(move.originalPiece, move.from, turnTeam);
	hash ^= LookupGen::HashPiece(move.resultPiece, move.to, turnTeam);
#endif

	if (etd.occupy & toMask) {
		uint8_t capturedPieceType = pieceTypes[move.to];
		etd.pieceSets[capturedPieceType] &= toMaskInv;
#ifdef UPDATE_HASHES
		hash ^= LookupGen::HashPiece(capturedPieceType, move.to, !turnTeam);
#endif
		etd.occupy &= toMaskInv;
	}

	// Update piece types
	pieceTypes[move.to] = move.resultPiece;

	UpdateAttacksAndPins(turnTeam);

#ifdef UPDATE_HASHES
	hash ^= LookupGen::turnHashKey;
#endif

	turnTeam = !turnTeam;
}

void BoardState::ForceUpdateAll() {
#ifdef UPDATE_HASHES
	hash = 0;
#endif

	for (uint8_t team = 0; team < TEAM_AMOUNT; team++) {

		// Update attacks and pins
		UpdateAttacksAndPins(team);

		auto& td = teamData[team];
#ifdef UPDATE_VALUES
		td.totalValue = 0;
#endif

#ifdef _DEBUG
		for (int i = 0; i < PT_AMOUNT; i++) {
			BitBoard pieceSetOutsideOccupy = td.pieceSets[i] & (~td.occupy);
			ASSERT(pieceSetOutsideOccupy == 0);
		}
#endif

		td.occupy.Iterate(
			[&](uint64_t i) {
				for (int j = 0; j < PT_AMOUNT; j++) {
					if (td.pieceSets[j][i]) {
						pieceTypes[i] = j;
#ifdef UPDATE_VALUES
						Value val = LookupGen::GetPieceValue(j, i, team);
						pieceValues[i] = val;
						td.totalValue += val;
#endif
#ifdef UPDATE_HASHES
						hash ^= LookupGen::HashPiece(j, i, team);
#endif
					}
				}
					
			}
		);

#ifdef UPDATE_HASHES
		hash ^= LookupGen::HashCastleRights(team, td.canCastle_Q, td.canCastle_K);
#endif
	}

#ifdef UPDATE_HASHES
	hash ^= LookupGen::HashEnPassant(enPassantToMask != 0, enPassantPawnPos);
	if (turnTeam == TEAM_BLACK)
		hash ^= LookupGen::HashTurn();
#endif
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

					bool pieceFound = false;
					char pieceChar = '?';
					for (int i = 0; i < TEAM_AMOUNT; i++) {
						for (int j = 0; j < PT_AMOUNT; j++) {
							if (boardState.teamData[i].pieceSets[j][pos]) {

								if (pieceFound) {
									pieceChar = '?';
								} else {
									pieceChar = PT_CHARS[j];
								}

								pieceFound = true;
							}
						}
					}
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
	stream << "} (" << TEAM_NAMES[boardState.turnTeam] << " to move)";

	return stream;
}
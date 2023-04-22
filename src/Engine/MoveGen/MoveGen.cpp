#include "MoveGen.h"

#include "../LookupGen/LookupGen.h"

enum {
	CASTLE_SIDE_QUEEN,	// Low X
	CASTLE_SIDE_KING,	// High X

	CASTLE_SIDE_AMOUNT
};

// These squares must be empty in order to castle
constexpr uint64_t CASTLE_EMPTY_MASKS[TEAM_AMOUNT][CASTLE_SIDE_AMOUNT] = {
		{ // White
			{ ANI_BM('B1') | ANI_BM('C1') | ANI_BM('D1') }, // Far
			{ ANI_BM('F1') | ANI_BM('G1') }, // Near
		},

		{ // Black
			{ ANI_BM('B8') | ANI_BM('C8') | ANI_BM('D8') }, // Far
			{ ANI_BM('F8') | ANI_BM('G8') }, // Near
		}
};

// These squares cannot be in attack from the enemy in order to castle
constexpr uint64_t CASTLE_SAFETY_MASKS[TEAM_AMOUNT][CASTLE_SIDE_AMOUNT] = {
	{ // White
		{ ANI_BM('C1') | ANI_BM('D1') }, // Far
		{ ANI_BM('F1') | ANI_BM('G1') }, // Near
	},

	{ // Black
		{ ANI_BM('C8') | ANI_BM('D8') }, // Far
		{ ANI_BM('F8') | ANI_BM('G8') }, // Near
	}
};

template <uint8_t PIECE_TYPE, typename CALLBACK>
FINLINE void AddMovesFromBB(Pos from, BitBoard toBB, CALLBACK callback) {
	toBB.Iterate([&](Pos i) {

		// Promotion check
		if constexpr (PIECE_TYPE == PT_PAWN) {
			uint8_t y = i.Y();
			if (y == 0 || y == (BD_SIZE - 1)) {
				// Promotion
				BoardState::Move
					pKnight = { from, i, PT_PAWN, PT_KNIGHT },
					pBishop = { from, i, PT_PAWN, PT_BISHOP },
					pRook = { from, i, PT_PAWN, PT_ROOK },
					pQueen = { from, i, PT_PAWN, PT_QUEEN };
				callback(pKnight);
				callback(pBishop);
				callback(pRook);
				callback(pQueen);
				return;
			}
		}

		BoardState::Move move = { from, i, PIECE_TYPE, PIECE_TYPE };
		callback(move);
	});
}

template <uint8_t TEAM, bool EN_PASSANT_AVAILABLE, bool ONLY_KING_MOVES, typename CALLBACK>
FINLINE void _GetMoves(const BoardState& board, uint64_t checkersAmount, CALLBACK callback) {
	auto& td = board.teamData[TEAM];
	auto& etd = board.teamData[!TEAM];

	BitBoard
		teamOccupy = td.occupy,
		teamOccupyInv = ~teamOccupy,
		enemyOccupy = etd.occupy;
	BitBoard combinedOccupy = teamOccupy | enemyOccupy;

	if constexpr (!ONLY_KING_MOVES) {

		BitBoard checkBlockPathMask = BitBoard::Filled();
		if (checkersAmount == 1)
			checkBlockPathMask = LookupGen::GetPartialLineMask(etd.firstCheckingPiecePos, td.kingPos) | etd.checkers;

		BitBoard normalMoveMask = checkBlockPathMask & teamOccupyInv;

		BitBoard pinnedPieces = td.pinnedPieces;

		uint8_t teamDir = (TEAM == TEAM_WHITE) ? 1 : -1;
		uint8_t pawnRowY = (TEAM == TEAM_WHITE) ? 1 : 6;

		BitBoard pieces;
		
		pieces = td.pieceSets[PT_PAWN] & teamOccupy;
		pieces.Iterate(
			[&](uint64_t _i) {
				Pos i = _i;

				// Removes the forward move if the square in front is occupied
				BitBoard forwardMove = LookupGen::GetPawnMoves(i, TEAM) & ~combinedOccupy;

				int moveFromY = i.Y();

				// Add double-forward move if we are in the pawn row and nothing is in the way
				bool inPawnRow = (moveFromY == pawnRowY);
				if (inPawnRow) // Compiler should make this branchless
					forwardMove |= ((TEAM == TEAM_WHITE ? (forwardMove << BD_SIZE) : (forwardMove >> BD_SIZE)) & ~combinedOccupy);

				// Only adds attacks that are to squares with an enemy piece
				BitBoard baseAttacks = LookupGen::GetPawnAttacks(i, TEAM);
				BitBoard attacks = baseAttacks & enemyOccupy;

				if constexpr (EN_PASSANT_AVAILABLE) {
					if (baseAttacks & board.enPassantToMask) {
						if (!pinnedPieces[board.enPassantPawnPos]) {
							if (td.kingPos.Y() == moveFromY) {
								// We need to make sure this en passant capture wont reveal a check

								int enemyPiecesOnRankAmount = BitBoard(LookupGen::GetRankMask(i) & etd.occupy).BitCount();

								if (enemyPiecesOnRankAmount > 1) {
									int kingPosX = td.kingPos.X();
									int moveFromX = i.X();
									int moveToX = board.enPassantPawnPos.X();
									int potentialRevealedDirX = (moveFromX < kingPosX) ? 1 : -1;
									int stopX = (potentialRevealedDirX > 0) ? BD_SIZE : -1;
									bool wouldRevealKingAttack = false;

									// TODO: Make more efficient
									for (int x = kingPosX - potentialRevealedDirX; x != stopX; x -= potentialRevealedDirX) {
										if (x == moveFromX || x == moveToX)
											continue;

										Pos pos = Pos(x, moveFromY);
										if (combinedOccupy[pos]) {
											if (etd.occupy[pos]) {
												wouldRevealKingAttack = etd.pieceSets[PT_ROOK][pos] || etd.pieceSets[PT_QUEEN][pos];
											}

											break;
										}
									}

									if (!wouldRevealKingAttack) {
										attacks |= board.enPassantToMask;
									}

								} else {
									attacks |= board.enPassantToMask;
								}

							} else {
								attacks |= board.enPassantToMask;
							}
						}
					}
				}

				BitBoard moves = (forwardMove | attacks) & checkBlockPathMask;

				if constexpr (EN_PASSANT_AVAILABLE) {
					// FIX FOR SPECIAL CASE: Allow en passant to capture a checking pawn
					if (checkersAmount && etd.firstCheckingPiecePos == board.enPassantPawnPos) {
						moves |= attacks & board.enPassantToMask;
					}
				}

				if (pinnedPieces[i])
					moves &= LookupGen::GetLineMask(i, td.kingPos);

				AddMovesFromBB<PT_PAWN>(i, moves, callback);
			}
		);

		pieces = td.pieceSets[PT_ROOK];
		pieces.Iterate(
			[&](uint64_t i) {
				BitBoard baseMoves, moves;
				LookupGen::GetRookMoves(i, combinedOccupy, baseMoves, moves);
				moves &= normalMoveMask;
				if (pinnedPieces[i])
					moves &= LookupGen::GetLineMask(i, td.kingPos);

				AddMovesFromBB<PT_ROOK>(i, moves, callback);
			}
		);

		pieces = td.pieceSets[PT_KNIGHT];
		pieces.Iterate(
			[&](uint64_t i) {
				BitBoard moves = LookupGen::GetKnightMoves(i) & normalMoveMask;
				if (pinnedPieces[i])
					moves &= LookupGen::GetLineMask(i, td.kingPos);

				AddMovesFromBB<PT_KNIGHT>(i, moves, callback);
			}
		);

		pieces = td.pieceSets[PT_BISHOP];
		pieces.Iterate(
			[&](uint64_t i) {
				BitBoard baseMoves, moves;
				LookupGen::GetBishopMoves(i, combinedOccupy, baseMoves, moves);
				moves &= normalMoveMask;
				if (pinnedPieces[i])
					moves &= LookupGen::GetLineMask(i, td.kingPos);

				AddMovesFromBB<PT_BISHOP>(i, moves, callback);
			}
		);

		pieces = td.pieceSets[PT_QUEEN];
		pieces.Iterate(
			[&](uint64_t i) {
				BitBoard baseMoves, moves;
				LookupGen::GetQueenMoves(i, combinedOccupy, baseMoves, moves);
				moves &= normalMoveMask;
				if (pinnedPieces[i])
					moves &= LookupGen::GetLineMask(i, td.kingPos);

				AddMovesFromBB<PT_QUEEN>(i, moves, callback);
			}
		);
	}

	{ // King
		BitBoard moves = LookupGen::GetKingMoves(td.kingPos) & teamOccupyInv & ~etd.attack;

		// Castling
		if (checkersAmount == 0) {
			for (int i = 0; i < 2; i++) {
				bool canCastle = i ? td.canCastle_K : td.canCastle_Q;
				if (canCastle) {
					BitBoard castleEmptyMask = CASTLE_EMPTY_MASKS[TEAM][i];
					if ((combinedOccupy & castleEmptyMask) == 0) {
						BitBoard castleSafetyMask = CASTLE_SAFETY_MASKS[TEAM][i];
						if ((castleSafetyMask & etd.attack) == 0) {
							BoardState::Move castleMove = {
								td.kingPos,
								td.kingPos + (i ? 2 : -2),
								PT_KING,
								PT_KING
							};
							callback(castleMove);
						}
					}
				}
			}
		}

		AddMovesFromBB<PT_KING>(td.kingPos, moves, callback);
	}
}

#define KM_G(team, enPassant) \
	if (onlyKingMoves) \
		_GetMoves<team, enPassant, 1>(board, checkersAmount, callback); \
	else \
		_GetMoves<team, enPassant, 0>(board, checkersAmount, callback);

template <typename T>
void _GetMovesWrapper(const BoardState& board, T callback) {
	int checkersAmount = board.teamData[!board.turnTeam].checkers.BitCount();
	bool onlyKingMoves = checkersAmount > 1;
	if (board.turnTeam == TEAM_WHITE) {
		if (board.enPassantToMask) {
			KM_G(TEAM_WHITE, true);
		} else {
			KM_G(TEAM_WHITE, false);
		}
	} else {
		if (board.enPassantToMask) {
			KM_G(TEAM_BLACK, true);
		} else {
			KM_G(TEAM_BLACK, false);
		}
	}
}

void MoveGen::GetMoves(const BoardState& board, vector<BoardState::Move>& movesOut) {
	_GetMovesWrapper(board, [&](const BoardState::Move& move) { movesOut.push_back(move); });
}

void MoveGen::GetMoves(const BoardState& board, MoveCallbackFn callback) {
	_GetMovesWrapper(board, callback);
}
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
FINLINE void AddMovesFromBB(Pos from, BitBoard toBB, BitBoard enemyOccupy, CALLBACK callback) {
	toBB.Iterate([&](Pos i) {
		bool isCapture = enemyOccupy[i];

		// Promotion check
		if constexpr (PIECE_TYPE == PT_PAWN) {
			uint8_t y = i.Y();
			if (y == 0 || y == (BD_SIZE - 1)) {
				// Promotion
				BoardState::Move
					pKnight = { from, i, PT_PAWN, PT_KNIGHT, isCapture },
					pBishop = { from, i, PT_PAWN, PT_BISHOP, isCapture },
					pRook = { from, i, PT_PAWN, PT_ROOK, isCapture },
					pQueen = { from, i, PT_PAWN, PT_QUEEN, isCapture };
				callback(pKnight);
				callback(pBishop);
				callback(pRook);
				callback(pQueen);
				return;
			}
		}

		BoardState::Move move = { from, i, PIECE_TYPE, PIECE_TYPE, isCapture };
		callback(move);
	});
}

template <uint8_t TEAM, bool EN_PASSANT_AVAILABLE, bool ONLY_KING_MOVES, bool JUST_COUNT, typename CALLBACK>
FINLINE void _GetMoves(const BoardState& board, uint64_t checkersAmount, CALLBACK callbackOrCount) {
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
						bool isLegal = true;

						if (pinnedPieces[board.enPassantPawnPos]) {
							// If the target pawn is pinned, make sure capturing it blocks the pin
							BitBoard enemyPawnPinMask = LookupGen::GetLineMask(board.enPassantPawnPos, td.kingPos);
							isLegal = board.enPassantToMask & enemyPawnPinMask;
						}

						if (td.kingPos.Y() == moveFromY && isLegal) {
							// We need to make sure this en passant capture wont reveal a sideways check

							int enemyPiecesOnRankAmount = BitBoard(LookupGen::GetRankMask(i) & etd.occupy).BitCount();

							if (enemyPiecesOnRankAmount > 1) {
								int kingPosX = td.kingPos.X();
								int moveFromX = i.X();
								int moveToX = board.enPassantPawnPos.X();
								int potentialRevealedDirX = (moveFromX < kingPosX) ? 1 : -1;
								int stopX = (potentialRevealedDirX > 0) ? BD_SIZE : -1;

								// TODO: Make more efficient
								for (int x = kingPosX - potentialRevealedDirX; x != stopX; x -= potentialRevealedDirX) {
									if (x == moveFromX || x == moveToX)
										continue;

									Pos pos = Pos(x, moveFromY);
									if (combinedOccupy[pos]) {
										if (etd.occupy[pos]) {
											isLegal = !(etd.pieceSets[PT_ROOK][pos] || etd.pieceSets[PT_QUEEN][pos]);
										}
										break;
									}
								}
							}
						}
						
						if (isLegal) {
							attacks |= board.enPassantToMask;
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

				if constexpr (JUST_COUNT) {
					callbackOrCount += moves.BitCount();
				} else {
					BitBoard captureBB = etd.occupy;

					if constexpr (EN_PASSANT_AVAILABLE) {
						captureBB |= board.enPassantToMask;
					}

					AddMovesFromBB<PT_PAWN>(i, moves, captureBB, callbackOrCount);
				}
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

				if constexpr (JUST_COUNT) {
					callbackOrCount += moves.BitCount();
				} else {
					AddMovesFromBB<PT_ROOK>(i, moves, etd.occupy, callbackOrCount);
				}
			}
		);

		pieces = td.pieceSets[PT_KNIGHT];
		pieces.Iterate(
			[&](uint64_t i) {
				BitBoard moves = LookupGen::GetKnightMoves(i) & normalMoveMask;
				if (pinnedPieces[i])
					moves &= LookupGen::GetLineMask(i, td.kingPos);

				if constexpr (JUST_COUNT) {
					callbackOrCount += moves.BitCount();
				} else {
					AddMovesFromBB<PT_KNIGHT>(i, moves, etd.occupy, callbackOrCount);
				}
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

				if constexpr (JUST_COUNT) {
					callbackOrCount += moves.BitCount();
				} else {
					AddMovesFromBB<PT_BISHOP>(i, moves, etd.occupy, callbackOrCount);
				}
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

				if constexpr (JUST_COUNT) {
					callbackOrCount += moves.BitCount();
				} else {
					AddMovesFromBB<PT_QUEEN>(i, moves, etd.occupy, callbackOrCount);
				}
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
							if constexpr (JUST_COUNT) {
								callbackOrCount++;
							} else {
								BoardState::Move castleMove = {
									td.kingPos,
									td.kingPos + (i ? 2 : -2),
									PT_KING,
									PT_KING,
									false
								};
								callbackOrCount(castleMove);
							}
						}
					}
				}
			}
		}

		if constexpr (JUST_COUNT) {
			callbackOrCount += moves.BitCount();
		} else {
			AddMovesFromBB<PT_KING>(td.kingPos, moves, etd.occupy, callbackOrCount);
		}
	}
}

#define KM_G(team, enPassant, justCount) \
	if (onlyKingMoves) \
		_GetMoves<team, enPassant, 1, JUST_COUNT>(board, checkersAmount, callbackOrCount); \
	else \
		_GetMoves<team, enPassant, 0, JUST_COUNT>(board, checkersAmount, callbackOrCount);

template <bool JUST_COUNT, typename T>
void _GetMovesWrapper(const BoardState& board, T callbackOrCount) {
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
	_GetMovesWrapper<false>(board, [&](const BoardState::Move& move) { movesOut.push_back(move); });
}

void MoveGen::GetMoves(const BoardState& board, MoveCallbackFn callback) {
	_GetMovesWrapper<false>(board, callback);
}

void MoveGen::CountMoves(const BoardState& board, uint64_t& moveCount) {
	_GetMovesWrapper<true>(board, std::ref(moveCount));
}
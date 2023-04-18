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

template <bool IS_PAWN>
void AddMovesFromBB(Pos from, BitBoard toBB, uint8_t piece, vector<BoardState::Move>& out) {
	toBB.Iterate([&](Pos i) {

		// Promotion check
		if (IS_PAWN) {
			uint8_t y = i.Y();
			if (y == 0 || y == (BD_SIZE - 1)) {
				// Promotion
				out.push_back({ from, i, PT_KNIGHT });
				out.push_back({ from, i, PT_BISHOP });
				out.push_back({ from, i, PT_ROOK });
				out.push_back({ from, i, PT_QUEEN });
				return;
			}
		}

		out.push_back({ from, i, piece });
		});
}

template <bool EN_PASSANT_AVAILABLE>
void _GetMoves(BoardState& board, vector<BoardState::Move>& movesOut) {

	uint8_t team = board.turnTeam;

	auto& td = board.teamData[team];
	auto& etd = board.teamData[!team];

	BitBoard teamOccupy = td.occupy;
	BitBoard enemyOccupy = etd.occupy;
	BitBoard combinedOccupy = teamOccupy | enemyOccupy;

	uint8_t teamDir = (team == TEAM_WHITE) ? 1 : -1;
	uint8_t pawnRowY = (team == TEAM_WHITE) ? 1 : 6;

	uint8_t checkersAmount = etd.checkers.BitCount();
	bool multipleCheckers = checkersAmount > 1;

	BitBoard checkBlockPathMask = BitBoard::Filled();
	if (checkersAmount == 1)
		checkBlockPathMask = LookupGen::GetPartialLineMask(etd.firstCheckingPiecePos, td.kingPos) | etd.checkers;

	teamOccupy.Iterate(
		[&](uint64_t _i) {
			Pos i = _i;

			BitBoard moves;

			uint8_t piece = board.pieceTypes[i];

			if (multipleCheckers && piece != PT_KING)
				return;

			switch (piece) {
			case PT_PAWN:
			{
				// Removes the forward move if the square in front is occupied
				BitBoard forwardMove = LookupGen::GetPawnMoves(i, team) & ~combinedOccupy;

				int moveFromY = i.Y();

				// Add double-forward move if we are in the pawn row and nothing is in the way
				bool inPawnRow = (moveFromY == pawnRowY);
				if (inPawnRow) // Compiler should make this branchless
					forwardMove |= ((team == TEAM_WHITE ? (forwardMove << BD_SIZE) : (forwardMove >> BD_SIZE)) & ~combinedOccupy);

				// Only adds attacks that are to squares with an enemy piece
				BitBoard baseAttacks = LookupGen::GetPawnAttacks(i, team);
				BitBoard attacks = baseAttacks & etd.occupy;

				if constexpr (EN_PASSANT_AVAILABLE) {
					if (baseAttacks & board.enPassantToMask) {
						if (!td.pinnedPieces[board.enPassantPawnPos]) {
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
												uint8_t enemyPieceType = board.pieceTypes[pos];
												wouldRevealKingAttack = (enemyPieceType == PT_ROOK || enemyPieceType == PT_QUEEN);
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

				moves = (forwardMove | attacks) & checkBlockPathMask;

				if constexpr (EN_PASSANT_AVAILABLE) {
					// FIX FOR SPECIAL CASE: Allow en passant to capture a checking pawn
					if (checkersAmount && etd.firstCheckingPiecePos == board.enPassantPawnPos) {
						moves |= attacks & board.enPassantToMask;
					}
				}
			}
			break;
			case PT_KNIGHT:
			{
				moves = LookupGen::GetKnightMoves(i) & ~teamOccupy & checkBlockPathMask;
			}
			break;
			case PT_ROOK:
			{
				BitBoard baseMoves, occludedMoves;
				LookupGen::GetRookMoves(i, combinedOccupy, baseMoves, occludedMoves);
				moves = occludedMoves & ~teamOccupy & checkBlockPathMask;
			}
			break;
			case PT_BISHOP:
			{

				BitBoard baseMoves, occludedMoves;
				LookupGen::GetBishopMoves(i, combinedOccupy, baseMoves, occludedMoves);
				moves = occludedMoves & ~teamOccupy & checkBlockPathMask;
			}
			break;
			case PT_QUEEN:
			{
				BitBoard baseMoves, occludedMoves;
				LookupGen::GetQueenMoves(i, combinedOccupy, baseMoves, occludedMoves);
				moves = occludedMoves & ~teamOccupy & checkBlockPathMask;
			}
			break;

			default:
			{ // King
				moves = LookupGen::GetKingMoves(i) & ~teamOccupy & ~etd.attack;

				// Castling
				if (checkersAmount == 0) {
					for (int i = 0; i < 2; i++) {
						bool canCastle = i ? td.canCastle_K : td.canCastle_Q;
						if (canCastle) {
							BitBoard castleEmptyMask = CASTLE_EMPTY_MASKS[team][i];
							if ((combinedOccupy & castleEmptyMask) == 0) {
								BitBoard castleSafetyMask = CASTLE_SAFETY_MASKS[team][i];
								if ((castleSafetyMask & etd.attack) == 0) {
									BoardState::Move castleMove;
									castleMove.from = td.kingPos;
									castleMove.to = castleMove.from + (i ? 2 : -2);
									castleMove.resultPiece = PT_KING;
									movesOut.push_back(castleMove);
								}
							}
						}
					}
				}
			}
			}

			if (td.pinnedPieces[i])
				moves &= LookupGen::GetLineMask(i, td.kingPos);

			if (piece == PT_PAWN) {
				AddMovesFromBB<true>(i, moves, piece, movesOut);
			} else {
				AddMovesFromBB<false>(i, moves, piece, movesOut);
			}
		}
	);
}

void MoveGen::GetMoves(BoardState& board, vector<BoardState::Move>& movesOut) {
	if (board.enPassantToMask) {
		_GetMoves<true>(board, movesOut);
	} else {
		_GetMoves<false>(board, movesOut);
	}
}
#include "MoveGen.h"

#include "../Util/Util.h"
#include "../LookupGen/LookupGen.h"

using namespace Util;

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

void AddMovesFromBB(Pos from, BitBoard toBB, uint8_t piece, vector<BoardState::Move>& out) {
	toBB.Iterate([&](Pos i) {
		ASSERT(i != from);
		out.push_back({ from, i, piece });
	});
}

void MoveGen::GetMovesForTeam(BoardState& board, uint8_t team, vector<BoardState::Move>& movesOut) {
	BitBoard teamOccupy = board.teamData[team].occupy;
	BitBoard enemyOccupy = board.teamData[!team].occupy;
	BitBoard combinedOccupy = teamOccupy | enemyOccupy;

	auto& teamData = board.teamData[team];
	auto& enemyTeamData = board.teamData[!team];

	uint8_t teamDir = (team == TEAM_WHITE) ? 1 : -1;
	uint8_t pawnRowY = (team == TEAM_WHITE) ? 1 : 6;

	uint8_t checkersAmount = enemyTeamData.checkers.BitCount();
	bool multipleCheckers = checkersAmount > 1;

	BitBoard checkBlockPathMask = BitBoard::Filled();
	if (checkersAmount == 1)
		checkBlockPathMask = LookupGen::GetLineMask(enemyTeamData.firstCheckingPiecePos, teamData.kingPos);

	teamOccupy.Iterate(
		[&](uint64_t _i) {
			Pos i = _i;

			// TODO: switch/case instead of if/else

			BitBoard moves;

			uint8_t piece = board.pieceTypes[i];

			if (multipleCheckers && piece != PT_KING)
				return;

			if (piece == PT_PAWN) {
				// Removes the forward move if the square in front is occupied
				BitBoard forwardMove = LookupGen::GetPawnMoves(i, team) & ~combinedOccupy;

				// Add double-forward move if we are in the pawn row and nothing is in the way
				bool inPawnRow = (i.Y() == pawnRowY);
				if (inPawnRow) // Compiler should make this branchless
					forwardMove |= ((team == TEAM_WHITE ? (forwardMove << BD_SIZE) : (forwardMove >> BD_SIZE)) & ~combinedOccupy);

				// Only adds attacks that are to squares with an enemy piece
				BitBoard attacks = LookupGen::GetPawnAttacks(i, team) & (enemyOccupy | board.enPassantToMask);

				moves = (forwardMove | attacks) & checkBlockPathMask;
				
				// FIX FOR SPECIAL CASE: Allow en passant to capture a checking pawn
				if (checkersAmount && enemyTeamData.firstCheckingPiecePos == board.enPassantPawnPos) {
					moves |= attacks & board.enPassantToMask;
				}
			} else if (piece == PT_KNIGHT) {
				moves = LookupGen::GetKnightMoves(i) & ~teamOccupy, piece, movesOut;
			} else if (piece == PT_ROOK) {

				BitBoard baseMoves, occludedMoves;
				LookupGen::GetRookMoves(i, combinedOccupy, baseMoves, occludedMoves);
				moves = occludedMoves & ~teamOccupy & checkBlockPathMask;

			} else if (piece == PT_BISHOP) {

				BitBoard baseMoves, occludedMoves;
				LookupGen::GetBishopMoves(i, combinedOccupy, baseMoves, occludedMoves);
				moves = occludedMoves & ~teamOccupy & checkBlockPathMask;

			} else if (piece == PT_QUEEN) {
				BitBoard baseMoves, occludedMoves;
				LookupGen::GetQueenMoves(i, combinedOccupy, baseMoves, occludedMoves);
				moves = occludedMoves & ~teamOccupy & checkBlockPathMask;
			} else { // King
				moves = LookupGen::GetKingMoves(i) & ~teamOccupy & ~enemyTeamData.attack;

				// Castling
				if (checkersAmount == 0) {
					for (int i = 0; i < 2; i++) {
						bool canCastle = i ? teamData.canCastle_K : teamData.canCastle_Q;
						BitBoard castleEmptyMask = CASTLE_EMPTY_MASKS[team][i];
						if ((combinedOccupy & castleEmptyMask) == 0) {
							BitBoard castleSafetyMask = CASTLE_SAFETY_MASKS[team][i];
							if ((castleSafetyMask & enemyTeamData.attack) == 0) {
								BoardState::Move castleMove;
								castleMove.from = teamData.kingPos;
								castleMove.to = castleMove.from + (i ? 2 : -2);
								castleMove.resultPiece = PT_KING;
								movesOut.push_back(castleMove);
							}
						}
					}
				}
			}

			if (teamData.pinnedPieces[i]) {
				moves &= LookupGen::GetLineMask(i, teamData.kingPos);
			}

			AddMovesFromBB(i, moves, piece, movesOut);
		}
	);
}
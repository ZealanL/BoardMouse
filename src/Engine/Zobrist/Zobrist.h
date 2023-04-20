#include "../BitBoard/BitBoard.h"
#include "../Pos/Pos.h"

namespace Zobrist {

	extern uint64_t
		pieceHashKeys[TEAM_AMOUNT][PT_AMOUNT][BD_SQUARE_AMOUNT],
		castleHashKeys[TEAM_AMOUNT][2],
		enPassantHashKeys[BD_SQUARE_AMOUNT],
		turnHashKeys[TEAM_AMOUNT];

	void InitOnce();
	uint64_t HashBitBoard(BitBoard board, uint8_t piecesType, uint8_t team);

	FINLINE uint64_t HashCastleRights(uint8_t team, bool canCastle_Q, bool canCastle_K) {
		uint64_t hash = 0;
		if (canCastle_Q)
			hash ^= castleHashKeys[team][0];

		if (canCastle_K)
			hash ^= castleHashKeys[team][1];
		return hash;
	}

	FINLINE uint64_t HashEnPassant(bool enPassantAvailable, Pos enPassantPos) {
		uint64_t hash = 0;
		if (enPassantAvailable)
			hash ^= enPassantHashKeys[enPassantPos];
		return hash;
	}

	FINLINE uint64_t HashTurn(uint8_t turnTeam) {
		return turnHashKeys[turnTeam];
	}

	FINLINE uint64_t HashPiece(uint8_t pieceType, Pos pos, uint8_t team) {
		return pieceHashKeys[team][pieceType][pos];
	}
}
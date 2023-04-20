#include "Zobrist.h"

uint64_t
	Zobrist::pieceHashKeys[TEAM_AMOUNT][PT_AMOUNT][BD_SQUARE_AMOUNT] = {},
	Zobrist::castleHashKeys[TEAM_AMOUNT][2] = {},
	Zobrist::enPassantHashKeys[BD_SQUARE_AMOUNT] = {},
	Zobrist::turnHashKeys[TEAM_AMOUNT] = {};

uint64_t GenerateKey() {
	// TODO: Use better random engine
	static thread_local auto randEngine = std::default_random_engine();

	uint32_t a = randEngine();
	uint32_t b = randEngine();
	return (uint64_t)a | ((uint64_t)b << 32ull);
}

template <typename T>
void GenerateKeys(T keys, size_t amount) {
	// TODO: Bad practice
	uint64_t* flatArray = (uint64_t*)keys;
	for (size_t i = 0; i < amount; i++)
		flatArray[i] = GenerateKey();
}

void Zobrist::InitOnce() {
	LOG("Generating Zobrist hash keys...");

	{ // Only run once
		static bool once = true;
		if (!once)
			return;
		once = false;
	}

	GenerateKeys(pieceHashKeys, sizeof(pieceHashKeys) / sizeof(uint64_t));
	GenerateKeys(castleHashKeys, sizeof(castleHashKeys) / sizeof(uint64_t));
	GenerateKeys(enPassantHashKeys, sizeof(enPassantHashKeys) / sizeof(uint64_t));
	GenerateKeys(turnHashKeys, sizeof(turnHashKeys) / sizeof(uint64_t));

	LOG(" > Done.");
}

uint64_t Zobrist::HashBitBoard(BitBoard board, uint8_t piecesType, uint8_t team) {
	uint64_t hash = 0;

	board.Iterate(
		[&](uint64_t i) {
			hash ^= pieceHashKeys[team][piecesType][i];
		}
	);

	return hash;
}
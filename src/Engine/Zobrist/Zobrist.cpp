#include "Zobrist.h"

ZobristHash Zobrist::GenerateKey() {
	// TODO: Use better random engine
	static thread_local auto randEngine = std::default_random_engine();

	uint32_t a = randEngine();
	uint32_t b = randEngine();
	return (uint64_t)a | ((uint64_t)b << 32ull);
}
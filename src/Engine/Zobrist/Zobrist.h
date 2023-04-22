#include "../BitBoard/BitBoard.h"
#include "../Pos/Pos.h"

typedef uint64_t ZobristHash;

namespace Zobrist {
	ZobristHash GenerateKey();
}
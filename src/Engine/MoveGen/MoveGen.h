#pragma once
#include "../BoardState/BoardState.h"

typedef std::function<void(const Move& move)> MoveCallbackFn;

namespace MoveGen {
	void GetMoves(const BoardState& board, vector<Move>& movesOut);
	void GetMoves(const BoardState& board, MoveCallbackFn callbackFn);
	void CountMoves(const BoardState& board, uint64_t& moveCount);
}
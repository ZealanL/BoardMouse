#pragma once
#include "../BoardState/BoardState.h"

typedef std::function<void(const BoardState::Move& move)> MoveCallbackFn;

namespace MoveGen {
	void GetMoves(const BoardState& board, vector<BoardState::Move>& movesOut);
	void GetMoves(const BoardState& board, MoveCallbackFn callbackFn);
	void CountMoves(const BoardState& board, uint64_t& moveCount);
}
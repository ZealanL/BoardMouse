#pragma once
#include "../BoardState/BoardState.h"

typedef std::function<void(BoardState::Move& move)> MoveCallbackFn;

namespace MoveGen {
	void GetMoves(BoardState& board, vector<BoardState::Move>& movesOut);
	void GetMoves(BoardState& board, MoveCallbackFn callbackFn);
}

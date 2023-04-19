#pragma once
#include "../BoardState/BoardState.h"

namespace MoveGen {
	void GetMoves(BoardState& board, vector<BoardState::Move>& movesOut);
}

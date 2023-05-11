#pragma once
#include "../BoardState/BoardState.h"
#include "../ButterflyBoard/ButterflyBoard.h"

namespace MoveRating {
	void RateMoves(BoardState& boardState, MoveList& moves, ButterflyBoard& butterflyBoard);
}
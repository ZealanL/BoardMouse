#pragma once
#include "../BoardState/BoardState.h"

namespace MoveOrdering {
	// NOTE: Better moves will be placed towards end of the list
	void SortMoves(MoveList& moves);
}
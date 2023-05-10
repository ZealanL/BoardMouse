#include "MoveOrdering.h"

void MoveOrdering::SortMoves(MoveList& moves) {
	// Insertion sort
	size_t moveCount = moves.size;
	for (size_t i = 1; i < moveCount; i++) {
		for (size_t j = i; j > 0;) {
			Move prev = moves[j - 1];
			Move cur = moves[j];

			if (prev.moveRating > cur.moveRating) {
				// Swap
				moves[j - 1] = cur;
				moves[j] = prev;
				j--;
			} else {
				break;
			}
		}
	}
}
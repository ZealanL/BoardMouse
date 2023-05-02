#include "MoveOrdering.h"

// Returns true if a should be searched before b
FINLINE bool operator >(const Move& a, const Move& b) {
	// TODO: Not very effective
	return a.isCapture > b.isCapture;
}

void MoveOrdering::SortMoves(MoveList& moves) {
	// Insertion sort

	size_t moveCount = moves.size;
	for (size_t i = 1; i < moveCount; i++) {
		for (size_t j = i; j > 0;) {
			Move prev = moves[j - 1];
			Move cur = moves[j];

			if (prev > cur) {
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
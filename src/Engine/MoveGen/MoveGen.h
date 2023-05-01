#pragma once
#include "../BoardState/BoardState.h"

typedef std::function<void(const Move& move)> MoveCallbackFn;

#define MAX_MOVES 320
struct MoveList {
	Move data[MAX_MOVES];
	size_t size = 0;

	FINLINE void Add(const Move& move) {
		data[size] = move;
		size++;
		ASSERT(size <= MAX_MOVES);
	}

	FINLINE Move& operator[](size_t index) {
		ASSERT((index < size) && (index < MAX_MOVES));
		return data[index];
	}

	FINLINE const Move& operator[](size_t index) const {
		ASSERT((index < size) && (index < MAX_MOVES));
		return data[index];
	}

	FINLINE const Move* begin() const { return data; }
	FINLINE Move* begin() { return data; }

	FINLINE const Move* end() const { return data + size; }
	FINLINE Move* end() { return data + size; }
};

namespace MoveGen {
	void GetMoves(const BoardState& board, MoveList& movesOut);
	void GetMoves(const BoardState& board, MoveCallbackFn callbackFn);
	void CountMoves(const BoardState& board, uint64_t& moveCount);
}
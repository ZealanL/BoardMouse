#pragma once
#include "../Pos/Pos.h"

struct Move {
	Pos from, to;

	// Piece we moved
	uint8_t originalPiece;

	// Usually set to the piece we moved, except when promoting pawns
	uint8_t resultPiece;

	bool isCapture = false;

	Move() = default;

	FINLINE bool IsValid() const {
		return from != to;
	}

	friend std::ostream& operator<<(std::ostream& stream, const Move& move);
};

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
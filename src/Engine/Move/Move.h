#pragma once
#include "../Pos/Pos.h"

struct Move {
	Pos from, to;

	// Piece we moved
	uint8_t originalPiece;

	// Usually set to the piece we moved, except when promoting pawns
	uint8_t resultPiece;

	enum {
		FL_EN_PASSANT = (1 << 0),
		FL_CAPTURE = (1 << 1),
		FL_CASTLE = (1 << 2),
		FL_PROMOTION = (1 << 3),
	};
	uint8_t flags;

	// Original index of this move, set during move generation
	// The same position will always generate moves in the same order, so this index can be used to reference a specific move 
	uint16_t trueIndex;

	// For move ordering, determined after generation
	uint16_t moveRating;

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
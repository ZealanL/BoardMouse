#pragma once
#include "../PieceValue/PieceValue.h"
#include "../Zobrist/Zobrist.h"

struct TransposEntry {
	ZobristHash fullHash;
	Value eval;
	uint16_t depth;

	// Moves are always generated in the same order, so we just need to store the index
	uint16_t bestMoveIndex;

	// TODO: Store type for proper usage with alpha-beta pruning
	// Is this an alpha eval, beta eval, or exact eval?

	TransposEntry() {
		// I would allow fullHash to be uninitialized, but there's a risk that the memory could have been previously another hash.
		// To fix this, I could zero transposition table memory whenever resized, but what if another engine's memory is used or something?
		fullHash = NULL;
	}

	FINLINE bool IsValid() {
		return fullHash != NULL;
	}
};

#define TRANSPOS_BUCKET_SIZE 4

struct TransposBucket {
	TransposEntry entries[TRANSPOS_BUCKET_SIZE];

	FINLINE TransposEntry& operator[](size_t index) {
		ASSERT(index < TRANSPOS_BUCKET_SIZE);
		return entries[index];
	}
};

// Number of buckets for every megabyte of memory
#define TRANSPOS_BUCKET_COUNT_MB (1000 * 1000 / sizeof(TransposBucket))

// TODO: Multithreading support (just give each thread its own portion of the buckets)
struct TransposTable {
	vector<TransposBucket> buckets;
	
	TransposTable() {
		buckets = {};
	}

	void Init(size_t bucketCount);
	void Reset();

	FINLINE TransposEntry* Find(ZobristHash hash) {
		// TODO: Maybe force bucketCount to a power of two and store the number of bits
		//	That way, we can bit shift instead of expensive modulo on two runtime values
		size_t bucketIndex = hash % buckets.size();

		TransposBucket& bucket = buckets[bucketIndex];

		// Try to find matching entry
		for (size_t i = 0; i < TRANSPOS_BUCKET_SIZE; i++) {
			TransposEntry& entry = bucket[i];
			if (entry.fullHash == hash)
				return &entry;
		}

		// No entry exists for this position
		// Try to find entry to replace
		// TODO: Maybe merge this with the first loop for efficiency
		for (size_t i = 0; i < TRANSPOS_BUCKET_SIZE; i++) {
			TransposEntry& entry = bucket[i];
			if (!entry.IsValid())
				return &entry;
		}

		// TODO: Prioritize replacing the oldest
		return &(bucket[0]);
	}
};

namespace Transpos {
	extern TransposTable main;
}
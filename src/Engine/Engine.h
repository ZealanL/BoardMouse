#pragma once
#include "BoardState/BoardState.h"
#include "LookupGen/LookupGen.h"

#define MAX_SEARCH_DEPTH 256
#define MAX_EXTENDED_DEPTH 16

namespace Engine {

	struct Stats {
		// Evaluation (absolute, positive = white is winning, negative = black is winning)
		Value eval;

		// The last depth of the completed iteration
		uint16_t completedDepth;

		// Number of positions reached at depth 0
		uint64_t leafNodesEvaluated; 

		// Number of times we found a position within the transposition table
		uint64_t transposHits;

		// Number of times we used the previous result from the transposition table 
		//	instead of continuing our current search
		uint64_t transposOverrides;

		// Number of times the transposition table returned a position with a matching hash,
		//	but an invalid move index (hash collision)
		uint64_t transposBadMoveIndices;

		// Number of checkmates found in the current search (for either color)
		uint64_t matesFound;

		// Number of stalemates found in the current search (for either color)
		uint64_t stalematesFound;

		Stats() = default;
	};

	struct Settings {
		// Maximum extra plies to search if reached a capture or check at depth 0
		uint16_t maxExtendedDepth = 6;
	};

	enum {
		STATE_INITIALIZING,
		STATE_READY,
		STATE_SEARCHING
	};

	Settings& GetSettings();

	uint8_t GetState();
	void SetState(uint8_t state);
	BoardState GetPosition();
	vector<Move> GetCurrentPV();

	Stats GetStats(); // NOTE: Stats are reset every new search

	void SetPosition(const BoardState& state);

	enum {
		SEARCH_COULDNT_START,
		SEARCH_INTERRUPTED,
		SEARCH_COMPLETED
	};

	uint8_t DoSearch(uint16_t depth, size_t maxTimeMS = -1);
	uint8_t DoPerftSearch(uint16_t depth);
	void StopSearch();
}
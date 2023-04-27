#pragma once
#include "../Engine/BoardState/BoardState.h"

namespace FEN {
	constexpr const char* STARTPOS = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

	void Parse(vector<string> parts, BoardState& boardStateOut);
	inline void Parse(string parts, BoardState& boardStateOut) {
		Parse(SPLIT_STR(parts, " "), boardStateOut);
	}
};
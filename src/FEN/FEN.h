#pragma once
#include "../Engine/BoardState/BoardState.h"

namespace FEN {
	void Parse(vector<string> parts, BoardState& boardStateOut);
	inline void Parse(string parts, BoardState& boardStateOut) {
		Parse(SPLIT_STR(parts, " "), boardStateOut);
	}
};
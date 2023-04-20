#pragma once
#include "../Engine/BoardState/BoardState.h"

namespace FEN {
	void Parse(string str, BoardState& boardStateOut);
};
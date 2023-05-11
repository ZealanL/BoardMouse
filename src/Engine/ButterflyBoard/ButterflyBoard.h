#pragma once

#include "../PieceValue/PieceValue.h"

// Bitflags that can be OR'd together
enum {
	BUTTERFLY_VAL_BETA_CUTOFF = 128,
	BUTTERFLY_VAL_ALPHA_BEST = 64,
};

struct ButterflyBoard {
	Value data[TEAM_AMOUNT][BD_SQUARE_AMOUNT][BD_SQUARE_AMOUNT] = {};

	FINLINE void Reset() {
		memset(data, 0, sizeof(data));
	}
};
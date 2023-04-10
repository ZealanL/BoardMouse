#pragma once
#include "../BoardState/BoardState.h"

// TODO: There can maybe be more than this?
#define MAX_MOVES_PER_TURN 600

namespace MoveGen {
	void GetMovesForTeam(BoardState& board, uint8_t team, vector<BoardState::Move>& movesOut);
}

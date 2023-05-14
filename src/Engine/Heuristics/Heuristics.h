#pragma once
#include "../BoardState/BoardState.h"

namespace Heuristics {
	// A bonus value for how much of the opponent's side we control, 
	//	as well as how much of the center we control, measured using our attack
	template <uint8_t TEAM>
	FINLINE Value GetSpaceControlBonus(const BoardState& boardState, bool isEndgame) {
		constexpr BitBoard
			WHITE_SIDE_MASK = BB_MASK_RANK(1) | BB_MASK_RANK(2) | BB_MASK_RANK(3) | BB_MASK_RANK(4),
			BLACK_SIDE_MASK = ~WHITE_SIDE_MASK;

		// Max base value = 32
		constexpr Value SPACE_BONUS_MULTIPLIER[2] = {
			// Middlegame/opening
			8,

			// Endgame
			2
		};


		// 4x2 block in middle of board
		constexpr BitBoard
			CENTER_CONTROL_MASK =
			(BB_MASK_FILE(2) | BB_MASK_FILE(3) | BB_MASK_FILE(4) | BB_MASK_FILE(5)) &
			(BB_MASK_RANK(3) | BB_MASK_RANK(4));

		// Max base value = 8
		constexpr Value CENTER_CONTROL_BONUS_MULTIPLIER[2] = {
			// Middlegame/opening
			16,

			// Endgame
			4
		};

		BitBoard enemySideMask = (TEAM == TEAM_WHITE) ? BLACK_SIDE_MASK : WHITE_SIDE_MASK;
		BitBoard spaceMask = boardState.teamData[TEAM].attack & enemySideMask;

		BitBoard centerControlMask = boardState.teamData[TEAM].attack & CENTER_CONTROL_MASK;

		return
			(spaceMask.BitCount() * SPACE_BONUS_MULTIPLIER[isEndgame]) +
			(centerControlMask.BitCount() * CENTER_CONTROL_BONUS_MULTIPLIER[isEndgame]);
	}
}
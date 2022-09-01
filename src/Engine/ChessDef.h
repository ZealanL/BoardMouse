#pragma once
#include "../Framework.h"

// Constant defines/enums/similar for chess

// Piece types enum
enum {
	PT_PAWN,
	PT_KNIGHT,
	PT_BISHOP,
	PT_ROOK,
	PT_QUEEN,
	PT_KING,

	PT_AMOUNT
};

// Size of x/y dimension of the board
#define BD_SIZE 8

// Highest index of x/y dimension on the board
#define BD_MAXI (BOARD_SIZE - 1)

// Amount of squares on the board
#define BD_SQUARE_AMOUNT (BOARD_SIZE*BOARD_SIZE)
#pragma once
#include "../Framework.h"

// Constant defines/enums/similar for chess

// Piece types enum
enum {
	PT_PAWN,
	PT_ROOK,
	PT_KNIGHT,
	PT_BISHOP,
	PT_QUEEN,
	PT_KING,

	PT_AMOUNT
};

// NOTE: Lowercase
constexpr const char PT_CHARS[PT_AMOUNT + 1] = "prnbqk";

// Teams enum
enum {
	TEAM_WHITE = 0,
	TEAM_BLACK = 1,

	TEAM_AMOUNT
};

constexpr const char* TEAM_NAMES[TEAM_AMOUNT] = { "WHITE", "BLACK" };

// Size of x/y dimension of the board
#define BD_SIZE 8

// Highest index of x/y dimension on the board
#define BD_MAXI (BD_SIZE - 1)

// Amount of squares on the board
#define BD_SQUARE_AMOUNT (BD_SIZE*BD_SIZE)
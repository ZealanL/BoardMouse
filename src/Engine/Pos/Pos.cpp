#include "Pos.h"

bool Pos::IsValid() {
	return index < BD_SQUARE_AMOUNT;
}
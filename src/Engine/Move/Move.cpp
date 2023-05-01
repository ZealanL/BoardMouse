#include "Move.h"

std::ostream& operator<<(std::ostream& stream, const Move& move) {
	if (move.IsValid()) {
		stream << move.from << move.to;
		if (move.originalPiece != move.resultPiece)
			stream << PT_CHARS[move.resultPiece];
	} else {
		stream << "[INVALID MOVE]";
	}

	return stream;
}
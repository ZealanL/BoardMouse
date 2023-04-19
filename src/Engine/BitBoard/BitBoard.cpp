#include "BitBoard.h"

#include "../Pos/Pos.h"

std::ostream& operator<<(std::ostream& stream, const BitBoard& bitBoard) {
	stream << " {" << std::endl;
	for (int y = 0; y < 8; y++) {
		stream << "\t";
		for (int x = 0; x < 8; x++) {
			Pos p = Pos(x, y);
			stream << (int)bitBoard[p];

			if (x < 7) {
				stream << ", ";
			}
		}
		stream << std::endl;
	}

	stream << "} ";
	return stream;
}
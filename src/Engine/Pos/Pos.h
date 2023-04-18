#pragma once
#include "../ChessDef.h"

// Converts coordinates (negative supported) to a board index/index offset
#define POSI(x, y) ((x)+(y)*BD_SIZE)

// A position on the chess board
struct Pos {
	// Board square index
	byte index; 

	Pos(byte index = 0) : index(index) {}
	Pos(uint8_t x, uint8_t y) {
		index = x + (y * BD_SIZE);
	}

	// Returns false if index is out-of-range
	bool IsValid();

	byte X() const { return index % BD_SIZE; }
	byte Y() const { return index / BD_SIZE; }

	Pos Offset(Pos offset) const {
		return Pos(index + offset.index);
	}

	operator byte() const {
		return index;
	}

	operator byte&() {
		return index;
	}

	static constexpr uint8_t Read(char letter, char number) {
		int x = letter - ((letter >= 'a') ? 'a' : 'A');
		int y = number - '1';
		return POSI(x, y);
	}

	static constexpr uint8_t Read(int16_t bothChars) {
		return Read(bothChars >> 8, bothChars & 0xFF);
	}

	static Pos Read(string str) {
		ASSERT(str.size() >= 2);
		return Read(str[0], str[1]);
	}

	friend std::ostream& operator <<(std::ostream& stream, const Pos& pos) {
		stream << (char)('a' + pos.X()) << (char)('1' + pos.Y());
		return stream;
	}
};

// Convert string/chars to Pos
#define ANI Pos::Read

#define ANI_BM(chars) (1ull << (uint8_t)Pos::Read(chars))
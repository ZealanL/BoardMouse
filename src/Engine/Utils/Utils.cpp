#include "Utils.h"

constexpr uint8_t Util::ANI(uint16_t chars) {
	char c1 = chars / 0xFF;
	char c2 = chars & 0xFF;
	int x = c1 - ((c1 >= 'a') ? 'a' : 'A');
	int y = c2 - '1';
	return POSI(x, y);
}

constexpr uint64_t Util::ANI_BM(uint16_t chars) {
	return 1ull << ANI(chars);
}

#include "LookupGen.h"

void InitOnce() {
	{ // Only run once
		static bool once = true;
		if (!once)
			return;
		once = false;
	}

	// TODO: ...
}
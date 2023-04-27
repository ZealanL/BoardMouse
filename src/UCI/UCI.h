#pragma once
#include "../Framework.h"

namespace UCI {
	void Init();

	// Returns true if command was successfully processed
	bool ProcessCommand(vector<string> parts);
}
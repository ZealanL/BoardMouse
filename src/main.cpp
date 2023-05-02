#include "UCI/UCI.h"
#include "Engine/LookupGen/LookupGen.h"
#include "Engine/Transpos/Transpos.h"
#include "Engine/Engine.h"
#include "FEN/FEN.h"

int main() {

	LOG("BoardMouse " BM_VERSION " by ZealanL");
	LOG(" > Built on " __DATE__);

	LookupGen::InitOnce();
	Transpos::Init(TRANSPOS_BUCKET_COUNT_MB * 50);
	Engine::SetState(Engine::STATE_READY);

	// Initialize with starting position
	BoardState startPosState;
	FEN::Parse(FEN::STARTPOS, startPosState);
	Engine::SetPosition(startPosState);

	UCI::Init();

	LOG("Ready!");
	LOG("============================");

	string line;
	while (std::getline(std::cin, line)) {
		vector<string> parts = SPLIT_STR(line, " ");
		if (!parts.empty())
			UCI::ProcessCommand(parts);
	}

	return EXIT_SUCCESS;
}
#include "UCI.h"
#include "../Engine/BoardState/BoardState.h"
#include "../FEN/FEN.h"
#include "../Engine/Engine.h"
#include "../Engine/MoveGen/MoveGen.h"

std::condition_variable engineUpdateConVar;
std::mutex engineUpdateWaitMutex;

struct {
	bool isPerft = false;
	uint16_t depth = MAX_SEARCH_DEPTH;
	size_t maxTimeMS = -1;
} searchParams;

std::thread engineThread;
void EngineSearchLoop() {
	DLOG(" > Engine search loop started!");
	while (true) {
		std::unique_lock<std::mutex> updateMutexLock = std::unique_lock<std::mutex>(engineUpdateWaitMutex);
		engineUpdateConVar.wait(updateMutexLock);
		
		if (searchParams.isPerft) {
			Engine::DoPerftSearch(searchParams.depth);
		} else {
			uint8_t searchResult = Engine::DoSearch(searchParams.depth, searchParams.maxTimeMS);
			if (searchResult != Engine::SEARCH_COULDNT_START) {
				vector<Move> moves = Engine::GetCurrentPV();
				if (moves.empty()) {
					assert(false);
					LOG("bestmove none");
				} else {
					LOG("bestmove " << moves.front());
				}
			}
		}
	}
}

void StartEngine(bool isPerft, uint16_t depth, size_t maxTimeMS) {
	searchParams.isPerft = isPerft;
	searchParams.depth = CLAMP(depth, 1, MAX_SEARCH_DEPTH);
	searchParams.maxTimeMS = maxTimeMS = MAX(maxTimeMS, 0);
	engineUpdateConVar.notify_one();
}

void StopEngine() {
	Engine::StopSearch();

	// Wait for engine to actually stop
	while (Engine::GetState() == Engine::STATE_SEARCHING)
		std::this_thread::yield();
}

void UCI::Init() {
	engineThread = std::thread(EngineSearchLoop);
	DLOG("Starting engine search loop on thread " << engineThread.get_id() << "...");
	engineThread.detach();
}

bool UCI::ProcessCommand(vector<string> parts) {
	string firstPart = parts.front();

	if (firstPart == "uci") {
		LOG("id name BoardMouse " BM_VERSION)
		LOG("id author ZealanL")
		LOG("uciok");
		return true;
	} else if (firstPart == "isready") {
		LOG("readyok");
		return true;
	} else if (firstPart == "quit") {
		exit(EXIT_SUCCESS);
		return true;
	} else if (firstPart == "position") {
		if (parts.size() < 2)
			return false;

		BoardState newPosition;
		int nextIndex = 2;
		string secondPart = parts[1];
		if (secondPart == "fen") {
			int fenPartAmount = 0;
			for (; nextIndex < parts.size(); nextIndex++) {
				if (parts[nextIndex] != "moves") {
					fenPartAmount++;
				} else {
					break;
				}
			}

			if (fenPartAmount > 0) {
				try {
					FEN::Parse(std::vector<string>(parts.begin() + 2, parts.begin() + 2 + fenPartAmount), newPosition);
				} catch (std::exception& e) {
					ASSERT(false);
					return false;
				}
			} else {
				return false;
			}

		} else if (secondPart == "startpos") {
			FEN::Parse(FEN::STARTPOS, newPosition);
		} else {
			return false;
		}

		if (nextIndex < parts.size() && parts[nextIndex] == "moves") {
			for (int i = nextIndex + 1; i < parts.size(); i++) {
				string moveStr = parts[i];
				MoveList legalMoves;
				MoveGen::GetMoves(newPosition, legalMoves);

				bool moveFound = false;
				for (auto& move : legalMoves) {
					if (STR(move) == moveStr) {
						newPosition.ExecuteMove(move);
						moveFound = true;
					}
				}

				if (!moveFound)
					break;
			}
		}

		Engine::SetPosition(newPosition);
		return true;
	} else if (firstPart == "go") {

		uint16_t depth = MAX_SEARCH_DEPTH;
		size_t maxTimeMS = -1;

		std::unordered_map<string, int64_t> argMap;

		for (int i = 1; i < parts.size(); i++) {
			string part = parts[i];
			if (isalpha(part.front())) {
				if (i < parts.size() - 1 && isdigit(parts[i + 1].front())) {
					int64_t val;
					try {
						val = std::stoll(parts[i + 1]);
					} catch (std::exception& e) {
						val = -1;
					}
					argMap[part] = val;
				} else {
					argMap[part] = -1;
				}
			}
		}

		bool isPerft = false;
		for (const pair<string, int64_t>& arg : argMap) {
			if (arg.first == "depth") {
				depth = arg.second;
			} else if (arg.first == "movetime") {
				maxTimeMS = arg.second;
			} else if (arg.first == "perft") {
				isPerft = true;
				depth = arg.second;
				break; // Don't parse any other arguments
			}
		}

		// TODO: Support the variety of other arguments

		StopEngine();
		StartEngine(isPerft, depth, maxTimeMS);
		return true;

	} else if (firstPart == "d") {
		LOG(Engine::GetPosition());
	} else if (firstPart == "stop") {
		StopEngine();
	} else {
		// TODO: Support more commands
	}

	return false;
}
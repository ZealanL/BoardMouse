#include "Engine.h"

#include "Transpos/Transpos.h"
#include "MoveGen/MoveGen.h"
#include "MoveOrdering/MoveOrdering.h"

Move g_CurPV[MAX_SEARCH_DEPTH] = {};
uint16_t g_CurPVLength = 0;

uint8_t g_CurState = Engine::STATE_INITIALIZING;
BoardState g_Position = BoardState();
Engine::Stats g_Stats = Engine::Stats();
std::mutex infoMutex = std::mutex();

// NOTE: infoMutex does not apply to this, as this variable needs to be checked very frequently
bool g_StopSearch = false;

// NOTE: No mutex for this
Engine::Settings g_Settings;

Engine::Settings& Engine::GetSettings() {
	return g_Settings;
}

uint8_t Engine::GetState() {
	infoMutex.lock();
	uint8_t result = g_CurState;
	infoMutex.unlock();
	return result;
}

void Engine::SetState(uint8_t state) {
	infoMutex.lock();
	g_CurState = state;
	infoMutex.unlock();
}

BoardState Engine::GetPosition() {
	infoMutex.lock();
	BoardState result = g_Position;
	infoMutex.unlock();
	return result;
}

vector<Move> Engine::GetCurrentPV() {
	infoMutex.lock();
	if (g_CurPVLength == 0) {
		infoMutex.unlock();
		return {};
	} else {
		ASSERT(g_CurPVLength <= MAX_SEARCH_DEPTH);
		auto result = vector<Move>(g_CurPV, g_CurPV + g_CurPVLength);
		infoMutex.unlock();
		return result;
	}
}

Engine::Stats Engine::GetStats() {
	infoMutex.lock();
	Stats result = g_Stats;
	infoMutex.unlock();
	return result;
}

void Engine::SetPosition(const BoardState& boardState) {
	infoMutex.lock();
	// NOTE: It's perfectly fine if we are in a search when this happens.
	//	That search will continue with the previous state.
	g_Position = boardState;
	infoMutex.unlock();
}

void Engine::StopSearch() {
	infoMutex.lock();
	if (g_CurState == STATE_SEARCHING) {
		g_StopSearch = true;
	}
	infoMutex.unlock();
}

enum class MinMaxResult {
	NONE,
	NEW_BEST,
	PRUNE_BRANCH
};

template <uint8_t TEAM>
FINLINE MinMaxResult UpdateMinMax(Value eval, Value& min, Value& max) {
	if ((TEAM == TEAM_WHITE) ? (eval >= max) : (eval <= min)) {
		// Branch will not be chosen because it already has a move that is worse than any move in another branch
		// We can just skip this whole branch
		return MinMaxResult::PRUNE_BRANCH;
	}

	if ((TEAM == TEAM_WHITE) ? (eval > min) : (eval < max)) {
		(TEAM == TEAM_WHITE ? min : max) = eval;
		return MinMaxResult::NEW_BEST;
	} else {
		return MinMaxResult::NONE;
	}
}

// NOTE: Value is relative to who's turn it is
template <uint8_t TEAM>
Value MinMaxSearchRecursive(
	BoardState& boardState, Value alpha, Value beta, uint16_t depth, uint16_t extendedDepth, uint16_t pvIndex, 
	bool nullMoveLast = false
) {

	if (g_StopSearch)
		return alpha;

	ASSERT(boardState.turnTeam == TEAM);

	TransposEntry* entry = Transpos::Find(boardState.hash);
	bool entryHashMatches = (entry->fullHash == boardState.hash);
	if (entryHashMatches)
		g_Stats.transposHits++;

	if (entryHashMatches && entry->depth >= depth) {
		// We've already evaluated this move at >= the current search depth, just use that
		g_Stats.transposOverrides++;

		Value eval = entry->eval;
		if (eval >= beta) {
			// Fail high
			return beta;
		}

		alpha = MAX(alpha, eval);
	} else {
		// No transposition entry for this position is as deep

		if (depth == 0) {

			// Update values for this team
			boardState.UpdateAttacksPinsValues(TEAM);

			g_Stats.leafNodesEvaluated++;
			Value 
				whiteVal = boardState.teamData[TEAM_WHITE].totalValue,
				blackVal = boardState.teamData[TEAM_BLACK].totalValue;
			return (TEAM == TEAM_WHITE) ? (whiteVal - blackVal) : (blackVal - whiteVal);

			// NOTE: We won't bother setting a transposition entry for a zero-depth evaluation
		} else {
			// NOTE: Moves will be iterated backwards
			MoveList moves;
			MoveGen::GetMoves(boardState, moves);
			MoveOrdering::SortMoves(moves);

			size_t moveCount = moves.size;

			if (moveCount == 0) {
				if (boardState.teamData[!TEAM].checkers != 0) {
					// Checkmate!
					// Other team wins
					g_Stats.matesFound++;
					return (TEAM == TEAM_WHITE) ? -CHECKMATE_VALUE : CHECKMATE_VALUE;
				} else {
					// Stalemate
					g_Stats.stalematesFound++;
					return 0;
				}
			} else {
				size_t lastBestMoveIndex;
				if (entryHashMatches) {
					lastBestMoveIndex = entry->bestMoveIndex;

					if (lastBestMoveIndex < moveCount) {
						// Explore the best move first
						moves.Add(moves[lastBestMoveIndex]);
						moveCount++;
					} else {
						// Hash collision!
						// This entry no longer matches
						g_Stats.transposBadMoveIndices++;
						lastBestMoveIndex = -1;
						entryHashMatches = false;
					}
				} else {
					lastBestMoveIndex = -1;
				}

				// Null move pruning
				// TODO: Avoid running in zugzwang
				if (depth > 1 && !nullMoveLast) {
					BoardState boardCopy = boardState;

					// Switch whos turn it is
					boardCopy.UpdateAttacksPinsValues(TEAM);
					boardCopy.turnTeam = !TEAM;

					Value eval = -MinMaxSearchRecursive<!TEAM>(boardCopy, -beta, -alpha, depth - 1, extendedDepth, UINT16_MAX, nullMoveLast);

					if (eval >= beta) {
						// Fail high
						return beta;
					}

					nullMoveLast = true;
				} else {
					nullMoveLast = false;
				}

				size_t bestMoveIndex = 0;
				// NOTE: size_t is unsigned so our loop condition should be (i < size)
				for (size_t i = moveCount - 1; i < moveCount; i--) {
					if (lastBestMoveIndex == i) {
						// We already added this move to the end of the vector
						// Ignore its original location
						continue;
					}

					Move& move = moves[i];

					BoardState boardCopy = boardState;
					boardCopy.ExecuteMove(move);

					uint16_t nextDepth, nextExtendedDepth;
					if (depth == 1 && (boardCopy.teamData[TEAM].checkers || (move.flags & Move::FL_CAPTURE)) && extendedDepth > 0) {
						// This is a check or capture, and we have extended depth left, so we need to keep searching deeper
						nextDepth = depth;
						nextExtendedDepth = extendedDepth - 1;
					} else {
						nextDepth = depth - 1;
						nextExtendedDepth = extendedDepth;
					}

					Value eval = -MinMaxSearchRecursive<!TEAM>(
						boardCopy, -beta, -alpha, nextDepth, nextExtendedDepth, 
						(pvIndex == UINT16_MAX) ? UINT16_MAX : pvIndex + 1, 
						nullMoveLast
						);

					if (eval >= beta) {
						// Fail high
						return beta;
					}

					if (eval > alpha) {
						// New best
						bestMoveIndex = i;
						alpha = eval;

						if (pvIndex != UINT16_MAX)
							g_CurPV[pvIndex] = move;
					}
				}

				// Update table entry
				entry->bestMoveIndex = bestMoveIndex;
				entry->fullHash = boardState.hash;
				entry->depth = depth;
				entry->eval = alpha;
			}
		}
	}

	return alpha;
}

uint8_t Engine::DoSearch(uint16_t depth, size_t maxTimeMS) {
	ASSERT(depth > 0);

#ifdef _DEBUG
	ASSERT(depth <= MAX_SEARCH_DEPTH);
#else
	depth = MIN(depth, MAX_SEARCH_DEPTH);
#endif

	size_t startTimeMS = CUR_MS();
	uint8_t previousState = STATE_READY;

	BoardState initialBoardState = GetPosition();
	infoMutex.lock();
	g_Stats = Stats();
	{
		if (g_CurState != STATE_READY) {
			// We aren't able to search right now
			infoMutex.unlock();
			return SEARCH_COULDNT_START;
		} else {
			previousState = g_CurState;
			g_CurState = STATE_SEARCHING;
		}

		g_CurPVLength = 0;
	}
	infoMutex.unlock();

	MoveList initialMoves;
	MoveGen::GetMoves(initialBoardState, initialMoves);
	bool isWhite = (initialBoardState.turnTeam == TEAM_WHITE);

	if (initialMoves.size > 0) {

		// Iterative deepening search
		for (uint16_t curDepth = 1; (curDepth <= depth) && (!g_StopSearch); curDepth++) {
			size_t msElapsed = CUR_MS() - startTimeMS;
			if (msElapsed > maxTimeMS && maxTimeMS > 0)
				break;

			DLOG("Searching at depth " << curDepth << "/" << depth << "...");

			const auto fnMinMaxSearch = [](uint8_t team, auto&&... args) -> Value {
				if (team == TEAM_WHITE) {
					return MinMaxSearchRecursive<TEAM_WHITE>(args...);
				} else {
					return MinMaxSearchRecursive<TEAM_BLACK>(args...);
				}
			};

			Value bestRelativeEval = fnMinMaxSearch(
				isWhite ? TEAM_WHITE : TEAM_BLACK,
				initialBoardState, -CHECKMATE_VALUE, CHECKMATE_VALUE, curDepth, g_Settings.maxExtendedDepth, 0
			);

			{ // Print UCI info
				// TODO: Move this all to UCI.cpp, use a callback function to trigger print
				std::stringstream uciInfo;
				uciInfo << "info";
				uciInfo << " depth " << curDepth;
				uciInfo << " multipv 1"; // TODO: Support multi-PV
				uciInfo << " score cp " << bestRelativeEval;
				uciInfo << " nodes " << g_Stats.leafNodesEvaluated;

				size_t nps = (size_t)(g_Stats.leafNodesEvaluated / (MAX(msElapsed, 500) / 1000.f));
				uciInfo << " nps " << nps;
				uciInfo << " time " << msElapsed;

				uciInfo << " pv ";
				for (int i = 0; i < g_CurPVLength; i++) {
					uciInfo << g_CurPV[i] << ' ';
				}
				LOG(uciInfo.str());
			}

			// Remove invalid PV
			infoMutex.lock();
			g_CurPVLength = depth;
			while ((g_CurPVLength > 1) && !g_CurPV[g_CurPVLength - 1].IsValid())
				g_CurPVLength--;
			infoMutex.unlock();
		}
	}

	infoMutex.lock();
	{
		g_CurState = previousState;
	}
	infoMutex.unlock();

	return SEARCH_COMPLETED;
}

void PerftSearchRecursive(BoardState& boardState, uint16_t depth, uint64_t& moveCount) {
	if (depth > 1) {
		MoveGen::GetMoves(boardState,
			[&](const Move& move) {
				BoardState boardCopy = boardState;
				boardCopy.ExecuteMove(move);
				PerftSearchRecursive(boardCopy, depth - 1, moveCount);
			}
		);
	} else {
		MoveGen::CountMoves(boardState, moveCount);
	}
}

uint8_t Engine::DoPerftSearch(uint16_t depth) {
	ASSERT(depth > 0);

	uint8_t previousState = STATE_READY;

	BoardState initialBoardState = GetPosition();
	infoMutex.lock();
	{
		if (g_CurState != STATE_READY) {
			// We aren't able to search right now
			infoMutex.unlock();
			return SEARCH_COULDNT_START;
		} else {
			previousState = g_CurState;
			g_CurState = STATE_SEARCHING;
		}
	}
	infoMutex.unlock();

	size_t startTimeMS = CUR_MS();

	uint64_t totalMoves = 0;

	bool stopped = false;

	MoveGen::GetMoves(initialBoardState,
		[&](const Move& move) {
			if (depth > 1) {

				if (g_StopSearch || stopped) {
					stopped = true;
					return;
				}

				BoardState boardCopy = initialBoardState;
				boardCopy.ExecuteMove(move);

				uint64_t totalSubMoves = 0;
				PerftSearchRecursive(boardCopy, depth - 1, totalSubMoves);
				LOG(move << ": " << totalSubMoves);
				totalMoves += totalSubMoves;
			} else {
				LOG(move << ": 1");
				totalMoves++;
			}
		}
	);

	if (!stopped) {
		uint64_t timeElapsed = CUR_MS() - startTimeMS;

		LOG((totalMoves > 0 ? "\n" : "") << "Nodes searched: " << totalMoves);
		string timeString;
		if (timeElapsed < 10000) {
			timeString = STR(timeElapsed << "ms");
		} else {
			timeString = STR(std::setprecision(4) << (timeElapsed / 1000.f) << "s");
		}
		LOG("Time: " << timeString);

		constexpr uint64_t MIN_TIME_ELAPSED = 500;

		if (timeElapsed >= MIN_TIME_ELAPSED) {
			uint64_t nps = totalMoves * 1000 / timeElapsed;
			float mnps = nps / (1000.f * 1000.f);
			LOG("Nodes per second: " << (totalMoves * 1000 / timeElapsed) << " (" << std::setprecision(3) << mnps << "mnps)");
		} else {
			LOG("Nodes per second: insufficient sample size (min = " << MIN_TIME_ELAPSED << "ms)");
		}
	} else {
		LOG("Search interrupted.");
	}

	infoMutex.lock();
	{
		g_CurState = previousState;
	}
	infoMutex.unlock();

	return SEARCH_COMPLETED;
}
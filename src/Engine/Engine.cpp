#include "Engine.h"

#include "Transpos/Transpos.h"
#include "MoveGen/MoveGen.h"
#include "MoveOrdering/MoveOrdering.h"
#include "MoveRating/MoveRating.h"
#include "ButterflyBoard/ButterflyBoard.h"
#include "Heuristics/Heuristics.h"

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

static ButterflyBoard g_ButterflyBoard = {};
constexpr uint16_t BUTTERFLY_BOARD_DEPTH = 7; // Depth at which we reset and start using the butterfly board

template<uint8_t TEAM>
FINLINE Value CalcRelativeEval(const BoardState& boardState, bool isEndgame) {
	Value
		whiteVal = boardState.teamData[TEAM_WHITE].totalValue,
		blackVal = boardState.teamData[TEAM_BLACK].totalValue;

	whiteVal += Heuristics::GetSpaceControlBonus<TEAM_WHITE>(boardState, isEndgame);
	blackVal += Heuristics::GetSpaceControlBonus<TEAM_BLACK>(boardState, isEndgame);

	return (TEAM == TEAM_WHITE) ? (whiteVal - blackVal) : (blackVal - whiteVal);
}

struct SearchInfo {
	uint16_t
		curDepth = 0, depthRemaining, curExtendedDepth = 0, extendedDepthRemaining;

	bool nullMoveUsed = false;
};

struct SearchFrame {
	MoveList moves;
};

// NOTE: Value is relative to who's turn it is
template <uint8_t TEAM>
Value MinMaxSearchRecursive(
	BoardState& boardState, Value alpha, Value beta, SearchInfo& info, SearchFrame* frame
) {

	if (g_StopSearch)
		return alpha;

	ASSERT(boardState.turnTeam == TEAM);

	TransposEntry* entry = Transpos::main.Find(boardState.hash);
	bool entryHashMatches = (entry->fullHash == boardState.hash);
	if (entryHashMatches)
		g_Stats.transposHits++;

	if (entryHashMatches && entry->depth >= info.depthRemaining) {
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

		if (info.depthRemaining == 0) {

			// Update values for this team
			boardState.UpdateAttacksPinsValues(TEAM);

			g_Stats.leafNodesEvaluated++;
			return CalcRelativeEval<TEAM>(boardState, boardState.IsEndgame());

			// NOTE: We won't bother setting a transposition entry for a zero-depth evaluation
		} else {

			if (info.depthRemaining == BUTTERFLY_BOARD_DEPTH + 1) {
				g_ButterflyBoard.Reset();
			}

			// NOTE: Moves will be iterated backwards
			MoveList& moves = frame->moves;
			moves.Clear();
			MoveGen::GetMoves(boardState, moves);
			
			size_t moveCount = moves.size;

			if (moveCount == 0) {
				if (boardState.teamData[!TEAM].checkers != 0) {
					// Checkmate!
					// Other team wins
					g_Stats.matesFound++;
					return -(CHECKMATE_VALUE + info.depthRemaining + info.extendedDepthRemaining); // Prioritize earlier checkmate
				} else {
					// Stalemate
					g_Stats.stalematesFound++;
					return 0;
				}
			} else {
				size_t lastBestMoveIndex;
				if (entryHashMatches) {
					lastBestMoveIndex = entry->bestMoveTrueIndex;

					if (lastBestMoveIndex < moveCount) {
						// Explore the best move first
						// We haven't ordered moves yet so this just requires index lookup
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
#ifdef ENABLE_NULL_MOVE_SEARCH
				// Null move search/pruning
				// TODO: Avoid running in zugzwang
				if (info.depthRemaining > 3 && !boardState.IsEndgame() && !info.nullMoveUsed && !boardState.teamData[!TEAM].checkers) {
					BoardState boardCopy = boardState;

					boardCopy.ExecuteNullMove();

					info.curDepth++;
					info.depthRemaining--;
					Value eval = -MinMaxSearchRecursive<!TEAM>(boardCopy, -beta, -alpha, info, frame + 1);
					info.curDepth--;
					info.depthRemaining++;

					if (eval >= beta) {
						// Fail high
						return beta;
					}

					info.nullMoveUsed = true;
				}
#endif

				MoveRating::RateMoves(boardState, moves, g_ButterflyBoard);
				MoveOrdering::SortMoves(moves);

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

					Value eval;
					if (
						info.depthRemaining == 1 && // Final depth (not counting eval)
						(boardCopy.teamData[TEAM].checkers || (move.flags & Move::FL_CAPTURE)) && // Is check or capture
						info.extendedDepthRemaining > 0 // We have extended depth left
						) {
						// Extended depth search
						info.curExtendedDepth++;
						info.extendedDepthRemaining--;
						eval = -MinMaxSearchRecursive<!TEAM>(
							boardCopy, -beta, -alpha, info, frame + 1
						);
						info.curExtendedDepth--;
						info.extendedDepthRemaining++;
					} else {
						// Normal search
						info.curDepth++;
						info.depthRemaining--;
						eval = -MinMaxSearchRecursive<!TEAM>(
							boardCopy, -beta, -alpha, info, frame + 1
							);
						info.curDepth--;
						info.depthRemaining++;
					}

					if (eval >= beta) {
						// Fail high

						if (info.depthRemaining <= BUTTERFLY_BOARD_DEPTH)
							g_ButterflyBoard.data[TEAM][move.from][move.to] |= BUTTERFLY_VAL_BETA_CUTOFF;

						return beta;
					}

					if (eval > alpha) {
						// New best

						if (info.depthRemaining <= BUTTERFLY_BOARD_DEPTH)
							g_ButterflyBoard.data[TEAM][move.from][move.to] |= BUTTERFLY_VAL_ALPHA_BEST;

						bestMoveIndex = move.trueIndex;
						alpha = eval;
					}
				}

				// Update table entry
				entry->bestMoveTrueIndex = bestMoveIndex;
				entry->fullHash = boardState.hash;
				entry->depth = info.depthRemaining;
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

		// Mark old transpos entries
		Transpos::main.MarkOld();

		// Iterative deepening search
		for (uint16_t curDepth = 1; (curDepth <= depth) && (!g_StopSearch); curDepth++) {
			size_t msElapsed = CUR_MS() - startTimeMS;
			if (msElapsed > maxTimeMS && maxTimeMS > 0)
				break;

			DLOG("Searching at depth " << curDepth << "/" << depth << "...");

			const auto fnMinMaxSearch = [](uint8_t team, auto&&... args) -> Value {
				g_ButterflyBoard.Reset();

				if (team == TEAM_WHITE) {
					return MinMaxSearchRecursive<TEAM_WHITE>(args...);
				} else {
					return MinMaxSearchRecursive<TEAM_BLACK>(args...);
				}
			};

			// Create search info
			SearchInfo searchInfo = {};
			searchInfo.depthRemaining = curDepth;
			ASSERT(g_Settings.maxExtendedDepth <= MAX_EXTENDED_DEPTH);
			searchInfo.extendedDepthRemaining = g_Settings.maxExtendedDepth;

			// Create frames
			SearchFrame frames[MAX_SEARCH_DEPTH + MAX_EXTENDED_DEPTH] = {};

			// Run recursive search
			Value bestRelativeEval = fnMinMaxSearch(
				isWhite ? TEAM_WHITE : TEAM_BLACK,
				initialBoardState, -CHECKMATE_VALUE * 2, CHECKMATE_VALUE * 2, searchInfo, frames
			);

			if (g_StopSearch) // We stopped early, don't update PV or print info as it is invalid
				break;

			// Update PV
			infoMutex.lock();
			{

				g_CurPVLength = 0;
				BoardState pvBoardState = initialBoardState;
				pvBoardState.ForceUpdateAll();

				for (size_t i = 0; i < curDepth; i++) {
					TransposEntry* entry = Transpos::main.Find(pvBoardState.hash);
					if (entry->fullHash == pvBoardState.hash) {
						MoveList moves;
						MoveGen::GetMoves(pvBoardState, moves);
						if (entry->bestMoveTrueIndex < moves.size) {
							Move bestMove = moves[entry->bestMoveTrueIndex];
							pvBoardState.ExecuteMove(bestMove);
							g_CurPV[i] = bestMove;
							g_CurPVLength++;
						} else {
							// Hash collision!
							break;
						}
					} else {
						// Failed to find
						break;
					}
				}
			}
			infoMutex.unlock();

			{ // Print UCI info
				// TODO: Move this all to UCI.cpp, use a callback function to trigger print
				std::stringstream uciInfo;
				uciInfo << "info";
				uciInfo << " depth " << curDepth;
				uciInfo << " multipv 1"; // TODO: Support multi-PV

				if (abs(bestRelativeEval) >= CHECKMATE_VALUE) {
					size_t remainingMovesTilCheckmate = g_CurPVLength;
					bool weWin = (bestRelativeEval > 0);

					size_t mateInMoveCount = (remainingMovesTilCheckmate + weWin) / 2;
					uciInfo << " mate " << (weWin ? "" : "-") << mateInMoveCount;
				} else {
					uciInfo << " score cp " << bestRelativeEval;
				}
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
		}
	}

	infoMutex.lock();
	{
		g_CurState = previousState;
	}
	infoMutex.unlock();

	g_StopSearch = false;

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
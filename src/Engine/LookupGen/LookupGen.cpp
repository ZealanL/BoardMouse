#include "LookupGen.h"

// Memory size: Negligible
static BitBoard
	kingMoveLookup[BD_SQUARE_AMOUNT],
	knightMoveLookup[BD_SQUARE_AMOUNT],
	rookMoveLookup[BD_SQUARE_AMOUNT],
	bishopMoveLookup[BD_SQUARE_AMOUNT];

// Lookup for the move or attack possibilities for any pawn in any position, for either team
// NOTE: Ignores occlusion and attack availability, that is done elsewhere
// Memory sizes: Negligible
static BitBoard pawnMoveLookup[TEAM_AMOUNT][BD_SQUARE_AMOUNT], pawnAttackLookup[TEAM_AMOUNT][BD_SQUARE_AMOUNT];

// The number of entries in a slider's occlusion lookup array
// Since the bit where the slider is present is ignored, there are 7 possible blocking spaces
// NOTE: To save memory, I could not check any bits along the edge (making this 64^2 instead of 128^2), 
//	however this would require another mask operation for every lookup
#define SLIDER_OCCLUSION_LOOKUP_ENTRY_COUNT (128*128)

// Memory size: ~16.8MB for both
// NOTE: If the edgemask is used (as discussed above), memory size would be only ~4.2MB for both
//	However, I'd prefer to use an extra 12 MB of memory to remove a lookup operation!
static BitBoard 
	rookOcclusionLookup[BD_SQUARE_AMOUNT][SLIDER_OCCLUSION_LOOKUP_ENTRY_COUNT],
	bishopOcclusionLookup[BD_SQUARE_AMOUNT][SLIDER_OCCLUSION_LOOKUP_ENTRY_COUNT];

// Masks of the path between two squares (not including those squares)
// Only works for paths that are diagonal or straight
// Memory size: Negligible
static BitBoard betweenMasks[BD_SQUARE_AMOUNT][BD_SQUARE_AMOUNT];

// Masks of the line connecting two squares, from one side of the board to the other
// Only works for paths that are diagonal or straight
// Memory size: Negligible
static BitBoard lineMasks[BD_SQUARE_AMOUNT][BD_SQUARE_AMOUNT];


template<bool IS_KING>
void GenerateNonSlidingPieceMoves() {
	pair<int, int>
		kingMoveOffsets[8] = {
			{-1, -1},
			{-1,  0},
			{-1,  1},

			{ 0, -1},
			{ 0,  1},

			{ 1, -1},
			{ 1,  0},
			{ 1,  1},
		},
		knightMoveOffsets[8] = {
			{-2, -1},
			{-2,  1},

			{-1, -2},
			{-1,  2},

			{ 1, -2},
			{ 1,  2},

			{ 2, -1},
			{ 2,  1},
		};

	auto moveOffsets = IS_KING ? kingMoveOffsets : knightMoveOffsets;

	for (Pos pos = 0; pos < BD_SQUARE_AMOUNT; pos++) {
		int x = pos.X();
		int y = pos.Y();

		BitBoard& mask = (IS_KING ? kingMoveLookup : knightMoveLookup)[pos];

		// Try to add any that we can
		for (int i = 0; i < 8; i++) {
			int rx = x + moveOffsets[i].first, ry = y + moveOffsets[i].second;
			if (rx >= 0 && ry >= 0 && rx < BD_SIZE && ry < BD_SIZE) {
				// In bounds, add
				mask.Set(Pos(rx, ry), true);
			}
		}
	}
}

template<bool IS_BISHOP>
void GenerateSlidingPieceMoves() {

	struct RayOffset {
		int x, y;
	};

	// Quick local lambda function for filling a 4 rays on a bitboard
	// Stops if it hits an occluded square (AFTER filling it)
	auto fnFillRays = [](BitBoard& board, Pos start, RayOffset rayOffsets[4], BitBoard occlusionMask = 0) {
		for (int rayIndex = 0; rayIndex < 4; rayIndex++) {
			RayOffset offset = rayOffsets[rayIndex];

			for (
				int x = start.X() + offset.x, y = start.Y() + offset.y;
				(x >= 0 && x < BD_SIZE) && (y >= 0 && y < BD_SIZE); 
				x += offset.x, y += offset.y
				) {

				Pos curPos = Pos(x, y);
				board.Set(curPos, true);
				if (occlusionMask[curPos])
					break; // We hit something
			}
		}
	};

	// X/Y offsets for each direction
	RayOffset offsets[4];
	if (IS_BISHOP) {
		offsets[0] = {-1, -1};
		offsets[1] = {1, -1};
		offsets[2] = {-1, 1};
		offsets[3] = {1, 1};
	} else {
		offsets[0] = {-1, 0};
		offsets[1] = {1, 0};
		offsets[2] = {0, -1};
		offsets[3] = {0, 1};
	}

	// Make the base masks (no occlusion)
	for (uint64_t i = 0; i < BD_SQUARE_AMOUNT; i++) {
		BitBoard& baseMoves = (IS_BISHOP ? bishopMoveLookup : rookMoveLookup)[i];

		fnFillRays(baseMoves, i, offsets);

		// Make the occlusion masks
		for (uint64_t j = 0; j < SLIDER_OCCLUSION_LOOKUP_ENTRY_COUNT; j++) {
			BitBoard& occlusionMoves = (IS_BISHOP ? bishopOcclusionLookup : rookOcclusionLookup)[i][j];

			// Using the current entry index, we can map the index bits to the bishop's attack squares to get the occlusion bitboard
			BitBoard occlusion = BitBoard(j).MapBits(baseMoves);
			occlusion.Set(i, false);

			// Then simply trace rays and stop when we hit something
			fnFillRays(occlusionMoves, i, offsets, occlusion);
		}
	}
}

void GeneratePawnMovesAndAttacks() {
	for (int i = 0; i < TEAM_AMOUNT; i++) {
		int yDir = (i == TEAM_WHITE) ? 1 : -1;
		for (Pos pos = 0; pos < BD_SQUARE_AMOUNT; pos++) {

			int x = pos.X();
			int nextY = pos.Y() + yDir;
			if (nextY < 0 || nextY > BD_MAXI)
				continue; // Pawn is at the other side of the board

			BitBoard& moveBoard = pawnMoveLookup[i][pos];
			BitBoard& attackBoard = pawnAttackLookup[i][pos];

			// Walk forward
			moveBoard.Set(Pos(x, nextY), true);

			// Attack left
			if (x > 0)
				attackBoard.Set(Pos(x - 1, nextY), true);

			// Attack right
			if (x < BD_MAXI)
				attackBoard.Set(Pos(x + 1, nextY), true);
		}
	}
}

void GenerateBetweenAndLineMasks() {
	// Prevent uninitialized memory usage
	memset(betweenMasks, 0, sizeof(betweenMasks));
	memset(lineMasks, 0, sizeof(lineMasks));

	for (Pos from = 0; from < BD_SQUARE_AMOUNT; from++) {
		for (Pos to = 0; to < BD_SQUARE_AMOUNT; to++) {
			if (from == to)
				continue;

			int dx = to.X() - from.X();
			int dy = to.Y() - from.Y();
			int maxDimLen = MAX(abs(dx), abs(dy));

			BitBoard& betweenMask = betweenMasks[from][to], lineMask = lineMasks[from][to];

			// Straight line path OR diagonal path
			if ((dx == 0) != (dy == 0) || abs(dx) == abs(dy)) {

				int step = POSI(dx / maxDimLen, dy / maxDimLen);
				
				// Between path: Fill all squares between start and end point
				for (Pos cur = from; cur != to; cur += step) {
					betweenMask.Set(cur, 1);
				}

				// Line path: Follow both directions (forwards and backwards)
				for (int dirSwitch = -1; dirSwitch <= 1; dirSwitch += 2) {

					// Fill all squares until we hit a wall
					for (
						int x = from.X(), y = from.Y();
						(x > -1 && y > -1) && (x < BD_SIZE && y < BD_SIZE);
						x += dx* dirSwitch, y += dy* dirSwitch) {
						lineMask.Set(POSI(x, y), 1);
					}
				}
				

			} else {
				// Invalid path, ignore it
			}
		}
	}
}

void LookupGen::InitOnce() {
	LOG("Initializing lookup data...");
	{ // Only run once
		static bool once = true;
		if (!once)
			return;
		once = false;
	}

	DLOG(" > Generating knight moves...");
	GenerateNonSlidingPieceMoves<false>();
	DLOG(" > Generating king moves...");
	GenerateNonSlidingPieceMoves<true>();
	
	DLOG(" > Generating rook moves...");
	GenerateSlidingPieceMoves<false>();
	DLOG(" > Generating bishop moves...");
	GenerateSlidingPieceMoves<true>();

	DLOG(" > Generating pawn moves/attacks...");
	GeneratePawnMovesAndAttacks();

	DLOG(" > Generating between-masks and line-masks...");
	GenerateBetweenAndLineMasks();

	DLOG(" > Done!");
}

void LookupGen::GetRookMoves(Pos rookPos, BitBoard occupiedMask, BitBoard& outBaseMoves, BitBoard& outOccludedMoves) {
	BitBoard baseMoves = rookMoveLookup[rookPos];
	outBaseMoves = baseMoves;
	BitBoard occlusionBits = occupiedMask.ExtractBits(baseMoves);
	outOccludedMoves = rookOcclusionLookup[rookPos][occlusionBits];
}

void LookupGen::GetBishopMoves(Pos bishopPos, BitBoard occupiedMask, BitBoard& outBaseMoves, BitBoard& outOccludedMoves) {
	BitBoard baseMoves = bishopMoveLookup[bishopPos];
	outBaseMoves = baseMoves;
	BitBoard occlusionBits = occupiedMask.ExtractBits(baseMoves);
	outOccludedMoves = bishopOcclusionLookup[bishopPos][occlusionBits];
}

void LookupGen::GetQueenMoves(Pos queenPos, BitBoard occupiedMask, BitBoard& outBaseMoves, BitBoard& outOccludedMoves) {
	// TODO: Maybe use seperate lookup(?)

	BitBoard 
		baseMoves_R = rookMoveLookup[queenPos],
		baseMoves_B = bishopMoveLookup[queenPos];
	outBaseMoves = baseMoves_R | baseMoves_B;

	BitBoard
		occlusionBits_R = occupiedMask.ExtractBits(baseMoves_R),
		occlusionBits_B = occupiedMask.ExtractBits(baseMoves_B);
	outOccludedMoves = 
		rookOcclusionLookup[queenPos][occlusionBits_R] | bishopOcclusionLookup[queenPos][occlusionBits_B];
}

BitBoard LookupGen::GetKingMoves(Pos kingPos) {
	return kingMoveLookup[kingPos];
}

BitBoard LookupGen::GetKnightMoves(Pos knightPos) {
	return knightMoveLookup[knightPos];
}

BitBoard LookupGen::GetPawnMoves(Pos pawnPos, uint8_t team) {
	return pawnMoveLookup[team][pawnPos];
}

BitBoard LookupGen::GetPawnAttacks(Pos pawnPos, uint8_t team) {
	return pawnAttackLookup[team][pawnPos];
}

BitBoard LookupGen::GetLineMask(Pos from, Pos to) {
	return lineMasks[from.index][to.index];
}

BitBoard LookupGen::GetBetweenMask(Pos from, Pos to) {
	return betweenMasks[from.index][to.index];
}
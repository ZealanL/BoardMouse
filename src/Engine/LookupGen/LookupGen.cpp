#include "LookupGen.h"

#include "../PieceValue/PieceValue.h"

 BitBoard
	 LookupGen::kingMoveLookup[BD_SQUARE_AMOUNT] = {},
	 LookupGen::knightMoveLookup[BD_SQUARE_AMOUNT] = {},
LookupGen::rookMoveLookup[BD_SQUARE_AMOUNT] = {},
LookupGen::bishopMoveLookup[BD_SQUARE_AMOUNT] = {};

// Lookup for the move or attack possibilities for any pawn in any position, for either team
// NOTE: Ignores occlusion and attack availability, that is done elsewhere
// Memory sizes: Negligible
BitBoard
LookupGen::pawnMoveLookup[TEAM_AMOUNT][BD_SQUARE_AMOUNT] = {},
LookupGen::pawnAttackLookup[TEAM_AMOUNT][BD_SQUARE_AMOUNT] = {};

// Memory size: ~16.8MB for both
// NOTE: If the edgemask is used (as discussed above), memory size would be only ~4.2MB for both
//	However, I'd prefer to use an extra 12 MB of memory to remove a lookup operation!
BitBoard
LookupGen::rookOcclusionLookup[BD_SQUARE_AMOUNT][SLIDER_OCCLUSION_LOOKUP_ENTRY_COUNT] = {},
LookupGen::bishopOcclusionLookup[BD_SQUARE_AMOUNT][SLIDER_OCCLUSION_LOOKUP_ENTRY_COUNT] = {};

// Masks of the path between two squares (not including those squares)
// Only works for paths that are diagonal or straight
// Memory size: Negligible
BitBoard LookupGen::betweenMasks[BD_SQUARE_AMOUNT][BD_SQUARE_AMOUNT] = {};

// Masks of the line connecting two squares, from one side of the board to the other
// Only works for paths that are diagonal or straight
// Memory size: Negligible
BitBoard LookupGen::lineMasks[BD_SQUARE_AMOUNT][BD_SQUARE_AMOUNT] = {};

// Masks of the line connecting two squares, but doesn't extend beyond the two points
// Only works for paths that are diagonal or straight
// Memory size: Negligible
BitBoard LookupGen::partialLineMasks[BD_SQUARE_AMOUNT][BD_SQUARE_AMOUNT] = {};

// Masks of all moves for any piece from the current location (aka queen|knight), including the current location
// Memory size: Negligible
BitBoard LookupGen::updateMasks[BD_SQUARE_AMOUNT] = {};

// Mask of the entire rank
// Memory size: Negligible
BitBoard LookupGen::rankMasks[BD_SQUARE_AMOUNT] = {};

// Values for each piece on each square
// Memory size: Negligible
float LookupGen::pieceValues[TEAM_AMOUNT][PT_AMOUNT][BD_SQUARE_AMOUNT] = {};

void GenerateNonSlidingPieceMoves(bool isKing) {
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

	auto moveOffsets = isKing ? kingMoveOffsets : knightMoveOffsets;

	for (Pos pos = 0; pos < BD_SQUARE_AMOUNT; pos++) {
		int x = pos.X();
		int y = pos.Y();

		BitBoard& mask = (isKing ? LookupGen::kingMoveLookup : LookupGen::knightMoveLookup)[pos];

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

struct RayOffset {
	int x, y;
};
void FillRay(BitBoard& board, Pos start, RayOffset offset, BitBoard occlusionMask) {
	for (
		int x = start.X() + offset.x, y = start.Y() + offset.y;
		(x >= 0 && x < BD_SIZE) && (y >= 0 && y < BD_SIZE);
		x += offset.x, y += offset.y
		) {

		Pos curPos = Pos(x, y);
		board.Set(curPos, true);
		if (occlusionMask[curPos])
			return; // We hit something
	}
}

void GenerateSlidingPieceMoves(bool isBishop) {

	// X/Y offsets for each direction
	RayOffset offsets[4];
	if (isBishop) {
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
		BitBoard& baseMoves = (isBishop ? LookupGen::bishopMoveLookup : LookupGen::rookMoveLookup)[i];

		for (int ri = 0; ri < 4; ri++)
			FillRay(baseMoves, i, offsets[ri], 0);

		// Make the occlusion masks
		for (uint64_t j = 0; j < SLIDER_OCCLUSION_LOOKUP_ENTRY_COUNT; j++) {
			BitBoard& occlusionMoves = (isBishop ? LookupGen::bishopOcclusionLookup : LookupGen::rookOcclusionLookup)[i][j];

			// Using the current entry index, we can map the index bits to the bishop's attack squares to get the occlusion bitboard
			BitBoard occlusion = BitBoard(j).MapBits(baseMoves);
			occlusion.Set(i, false);

			// Then simply trace rays and stop when we hit something
			for (int ri = 0; ri < 4; ri++)
				FillRay(occlusionMoves, i, offsets[ri], occlusion);
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

			BitBoard& moveBoard = LookupGen::pawnMoveLookup[i][pos];
			BitBoard& attackBoard = LookupGen::pawnAttackLookup[i][pos];

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
	for (Pos from = 0; from < BD_SQUARE_AMOUNT; from++) {
		for (Pos to = 0; to < BD_SQUARE_AMOUNT; to++) {
			if (from == to)
				continue;

			int dx = to.X() - from.X();
			int dy = to.Y() - from.Y();
			int maxDimLen = MAX(abs(dx), abs(dy));

			BitBoard betweenMask, lineMask, partialLineMask;

			// Straight line path OR diagonal path
			if ((dx == 0) != (dy == 0) || abs(dx) == abs(dy)) {

				int 
					dirX = dx / maxDimLen,
					dirY = dy / maxDimLen;
				int step = POSI(dirX, dirY);
				
				// Between path: Fill all squares between start and end point
				for (Pos cur = from + step; cur != to; cur += step)
					betweenMask.Set(cur, 1);

				// Line path: Follow both directions (forwards and backwards)
				for (int dirSwitch = -1; dirSwitch <= 1; dirSwitch += 2) {
					FillRay(lineMask, from, RayOffset{ dirX * dirSwitch, dirY * dirSwitch }, 0);

					// Fill starting square
					lineMask.Set(from, 1);
				}

				// Partial line path: just use between path with start and end filled
				partialLineMask = betweenMask;
				partialLineMask.Set(from, 1);
				partialLineMask.Set(to, 1);
			} else {
				// Invalid path, ignore it
			}

			LookupGen::betweenMasks[from][to] = betweenMask;
			LookupGen::lineMasks[from][to] = lineMask;
			LookupGen::partialLineMasks[from][to] = partialLineMask;
		}
	}
}

void GenerateUpdateMasks() {
	for (Pos pos = 0; pos < BD_SQUARE_AMOUNT; pos++) {
		BitBoard& mask = LookupGen::updateMasks[pos];
		mask |= LookupGen::rookMoveLookup[pos];
		mask |= LookupGen::bishopMoveLookup[pos];
		mask |= LookupGen::knightMoveLookup[pos];
		mask.Set(pos, true);
	}
}

void GenerateRankMasks() {
	for (Pos pos = 0; pos < BD_SQUARE_AMOUNT; pos++) {
		BitBoard& mask = LookupGen::rankMasks[pos];

		for (int x = 0; x < BD_SIZE; x++)
			mask.Set(Pos(x, pos.Y()), true);

		mask.Set(pos, true);
	}
}

void GeneratePieceValues() {
	for (uint8_t team = 0; team < TEAM_AMOUNT; team++) {
		for (uint8_t pieceType = 0; pieceType < PT_AMOUNT; pieceType++) {
			for (Pos pos = 0; pos < BD_SQUARE_AMOUNT; pos++) {
				// TODO: Make extra dimension for endgames
				float val = PieceValue::CalcPieceValue(pieceType, team, pos, false);
				LookupGen::pieceValues[team][pieceType][pos] = val;
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
	GenerateNonSlidingPieceMoves(false);
	DLOG(" > Generating king moves...");
	GenerateNonSlidingPieceMoves(true);
	
	DLOG(" > Generating rook moves...");
	GenerateSlidingPieceMoves(false);
	DLOG(" > Generating bishop moves...");
	GenerateSlidingPieceMoves(true);

	DLOG(" > Generating pawn moves/attacks...");
	GeneratePawnMovesAndAttacks();

	DLOG(" > Generating between-masks and line-masks...");
	GenerateBetweenAndLineMasks();

	DLOG(" > Generating update masks...");
	GenerateUpdateMasks();

	DLOG(" > Generating rank masks...");
	GenerateRankMasks();

	DLOG(" > Generating piece values...");
	GeneratePieceValues();

	DLOG(" > Done!");
}
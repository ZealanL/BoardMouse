#include "FEN.h"

void FEN::Parse(string fenStr, BoardState& boardStateOut) {

	// Nobody can castle by default
	for (int i = 0; i < 2; i++)
		boardStateOut.teamData[i].canCastle_K = boardStateOut.teamData[i].canCastle_Q = false;

	vector<string> tokens = SPLIT_STR(fenStr, " ");
	if (tokens.empty())
		throw "FEN string is empty";

	string boardStateStr = tokens[0];

	// Read board setup
	int
		x = 0,
		y = BD_SIZE - 1;
	for (char c : boardStateStr) {
		if (isalpha(c)) {
			if (POSI(x, y) >= BD_SQUARE_AMOUNT)
				throw "Too many pieces on rank";

			char lowerC = tolower(c);
			uint8_t team = (c == lowerC) ? TEAM_BLACK : TEAM_WHITE;
			auto& teamData = boardStateOut.teamData[team];

			uint8_t pieceType = PT_AMOUNT;
			for (int j = 0; j < PT_AMOUNT; j++) {
				if (lowerC == PT_CHARS[j]) {
					pieceType = j;
					break;
				}
			}

			if (pieceType == PT_AMOUNT)
				throw STR("Invalid piece type character: '" << toupper(c) << "'");

			int idx = POSI(x, y);

			if (pieceType == PT_KING) {
				if (teamData.pieceSets[PT_KING] != 0)
					throw STR("Color " << TEAM_NAMES[team] << " has multiple kings");

				teamData.kingPos = idx;
			}

			teamData.pieceSets[pieceType].Set(idx, true);
			teamData.occupy.Set(idx, true);

			x++;
		} else if (isdigit(c)) {
			int num = c - '0';
			x += num;
			if (x > BD_SIZE)
				throw "Too many pieces/pawns in rank";
		} else if (c == '/') {
			if (y <= 0)
				throw "Too many rank seperators";

			if (x != BD_SIZE)
				throw "Rank seperator doesn't happen at end of rank";

			y--;
			x = 0;
		}
	}

	for (int i = 0; i < TEAM_AMOUNT; i++)
		if (boardStateOut.teamData[i].pieceSets[PT_KING] == 0)
			throw STR("Color " << TEAM_NAMES[i] << " has no king");

	// Read turn
	if (tokens.size() >= 2) {
		string whosTurnToken = tokens[1];
		if (whosTurnToken.size() != 1)
			throw STR("Invalid turn character token (bad length of " << whosTurnToken.size() << ")");

		char teamChar = tolower(whosTurnToken[0]);

		if (teamChar == 'w') {
			boardStateOut.turnTeam = TEAM_WHITE;
		} else if (teamChar == 'b') {
			boardStateOut.turnTeam = TEAM_BLACK;
		} else {
			throw STR("Invalid turn character: '" << teamChar << "', should be 'w' or 'b'");
		}

	}

	// Read castle rights
	if (tokens.size() >= 3) {
		string castleRightsToken = tokens[2];
		if (castleRightsToken == "-") {
			// Nobody can castle
		} else {
			for (char c : castleRightsToken) {
				char upperC = toupper(c);
				uint8_t team = (c == upperC) ? TEAM_WHITE : TEAM_BLACK;

				auto& teamData = boardStateOut.teamData[team];
				if (upperC == 'K') {
					if (teamData.canCastle_K)
						throw STR("Repeated castling rights character: '" << c << '"');

					teamData.canCastle_K = true;
				} else if (upperC == 'Q') {
					if (teamData.canCastle_Q)
						throw STR("Repeated castling rights character: '" << c << '"');

					teamData.canCastle_Q = true;
				} else {
					throw STR("Invalid castling rights character: '" << c << "'");
				}
			}
		}
	}

	// Read en passant coordinate
	if (tokens.size() >= 4) {
		string enPassantToken = tokens[3];

		if (enPassantToken == "-") {
			// None
		} else {
			if (enPassantToken.size() != 2)
				throw STR("Invalid en passant coordinates: \"" << enPassantToken << "\"");

			char letter = enPassantToken[0];
			char number = enPassantToken[1];
			Pos enPassantToPos = ANI(enPassantToken[0], enPassantToken[1]);
			if (enPassantToPos >= BD_SQUARE_AMOUNT)
				throw STR("Out-of-bounds en passant coordinate: \"" << enPassantToken << "\"");

			boardStateOut.enPassantToMask.Set(enPassantToPos, true);

			Pos enPassantCapturePos = Pos(enPassantToPos.X(), enPassantToPos.Y() + (boardStateOut.turnTeam ? 1 : -1));
			boardStateOut.enPassantPawnPos = enPassantCapturePos;
		}

	}

	// Read half-move count
	if (tokens.size() >= 5) {
		string halfMoveCountStr = tokens[4];
		try {
			boardStateOut.halfMovesSincePawnOrCapture = std::stoi(halfMoveCountStr);
		} catch (std::exception& e) {
			throw "Invalid half-move count (failed to parse number)";
		}

		if (boardStateOut.halfMovesSincePawnOrCapture < 0)
			throw "Invalid half-move count (negative number)";
	}

	// Read move number
	if (tokens.size() >= 6) {
		string moveNumberStr = tokens[5];
		try {
			boardStateOut.moveNum = std::stoi(moveNumberStr);
		} catch (std::exception& e) {
			throw "Invalid move number (failed to parse number)";
		}
		if (boardStateOut.moveNum <= 0)
			throw "Invalid move number (number is non-positive)";
	}

	// TODO: Make sure rooks are present if castling is allowed

	boardStateOut.ForceUpdateAll();
}
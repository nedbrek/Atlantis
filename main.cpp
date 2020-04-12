// START A3HEADER
//
// This source file is part of the Atlantis PBM game program.
// Copyright (C) 1995-1999 Geoff Dunbar
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program, in the file license.txt. If not, write
// to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA 02111-1307, USA.
//
// See the Atlantis Project web page for details:
// http://www.prankster.com/project
//
// END A3HEADER
#include "gamedefs.h"
#include "game.h"
#include "items.h"
#include "skills.h"
#include "gamedata.h"
#include "gameio.h"
#include "astring.h"
#include <map>

namespace
{
void usage()
{
	Awrite("atlantis new");
	Awrite("atlantis run");
	Awrite("atlantis edit");
	Awrite("");
	Awrite("atlantis map <type> <mapfile>");
	Awrite("atlantis mapunits");
	Awrite("atlantis genrules <introfile> <cssfile> <rules-outputfile>");
	Awrite("");
	Awrite("atlantis check <orderfile> <checkfile>");
}

void doNew(Game &game, int argc, const char *argv[])
{
	int seed = 0;
	if (argc > 2)
		seed = AString(argv[2]).value();

	if (!game.NewGame(seed)) {
		Awrite("Couldn't make the new game!");
		return;
	}

	if (!game.SaveGame()) {
		Awrite("Couldn't save the game!");
		return;
	}

	if (!game.WritePlayers()) {
		Awrite("Couldn't write the players file!");
		return;
	}
}

void doMap(Game &game, int argc, const char *argv[])
{
	if (argc != 4) {
		usage();
		return;
	}

	if (!game.OpenGame()) {
		Awrite("Couldn't open the game file!");
		return;
	}

	if (!game.ViewMap(argv[2], argv[3])) {
		Awrite("Couldn't write the map file!");
		return;
	}
}

void doRun(Game &game, int argc, const char *argv[])
{
	if (!game.OpenGame()) {
		Awrite("Couldn't open the game file!");
		return;
	}

	if (!game.RunGame()) {
		Awrite("Couldn't run the game!");
		return;
	}

	if (!game.SaveGame()) {
		Awrite("Couldn't save the game!");
		return;
	}
}

void doEdit(Game &game, int argc, const char *argv[])
{
	if (!game.OpenGame()) {
		Awrite("Couldn't open the game file!");
		return;
	}

	int saveGame = 0;
	if (!game.EditGame(&saveGame)) {
		Awrite("Couldn't edit the game!");
		return;
	}

	if (saveGame) {
		if (!game.SaveGame()) {
			Awrite("Couldn't save the game!");
			return;
		}
	}
}

void doCheck(Game &game, int argc, const char *argv[])
{
	if (argc != 4) {
		usage();
		return;
	}

	game.DummyGame();

	if (!game.DoOrdersCheck(argv[2], argv[3])) {
		Awrite("Couldn't check the orders!");
		return;
	}
}

void doMapUnits(Game &game, int argc, const char *argv[])
{
	if (!game.OpenGame()) {
		Awrite("Couldn't open the game file!");
		return;
	}
	game.UnitFactionMap();
}

void doGenRules(Game &game, int argc, const char *argv[])
{
	if (argc != 5) {
		usage();
		return;
	}

	if (!game.GenRules(argv[4], argv[3], argv[2])) {
		Awrite("Unable to generate rules!");
		return;
	}
}
} // anonymous namespace

int main(int argc, const char *argv[])
{
	Game game;

	initIO();

	Awrite(AString("Atlantis Engine Version: ") +
			ATL_VER_STRING(CURRENT_ATL_VER));
	Awrite(AString(Globals->RULESET_NAME) + ", Version: " +
			ATL_VER_STRING(Globals->RULESET_VERSION));
	Awrite("");

	if (argc == 1) {
		usage();
		doneIO();
		return 0;
	}

	game.ModifyTablesPerRuleset();

	typedef void Handler(Game &, int, const char*[]);

	const std::map<std::string, Handler*> cmds = {
		{"check", doCheck},
		{"edit", doEdit},
		{"genrules", doGenRules},
		{"map", doMap},
		{"mapunits", doMapUnits},
		{"new", doNew},
		{"run", doRun}
	};

	auto i = cmds.find(argv[1]);
	if (i != cmds.end())
	{
		i->second(game, argc, argv);
	}
	else
	{
		Awrite("Unknown command");
		usage();
	}

	doneIO();
	return 0;
}


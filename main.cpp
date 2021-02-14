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
#include "object.h"
#include "gamedefs.h"
#include "game.h"
#include "items.h"
#include "skills.h"
#include "gamedata.h"
#include "gameio.h"
#include "astring.h"
#include "fileio.h"
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
	Awrite("atlantis dump <outfile>");
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

AString itemTypeToString(int type)
{
	if (type == -1)
		return "-1";

	switch (type)
	{
	case 0: return "0";
	case IT_MAN: return "IT_MAN";
	case IT_TRADE : return "IT_TRADE";
	case IT_MONSTER : return "IT_MONSTER";
	}

	AString ret;
	if (type & IT_NORMAL)
		ret += "IT_NORMAL";
	if (type & IT_ADVANCED)
		ret += "IT_ADVANCED";
	if (type & IT_MAGIC)
		ret += "IT_MAGIC";

	if (type & IT_WEAPON)
		ret += " | IT_WEAPON";
	if (type & IT_TOOL)
		ret += " | IT_TOOL";
	if (type & IT_FOOD)
		ret += " | IT_FOOD";
	if (type & IT_MOUNT)
		ret += " | IT_MOUNT";
	if (type & IT_ARMOR)
		ret += " | IT_ARMOR";
	if (type & IT_BATTLE)
		ret += " | IT_BATTLE";
	if (type & IT_SPECIAL)
		ret += " | IT_SPECIAL";

	if (ret.len())
		return ret;

	return "UNK_TYPE";
}

AString cleanStr(const char *str)
{
	AString ret;
	for (const char *s = str; *s != 0; ++s)
	{
		if (*s == ' ')
			ret += '_';
		else if (*s != '\'')
			ret += (char)toupper(*s);
	}
	return ret;
}

AString abbrToEnum(int idx)
{
	return cleanStr(ItemDefs[idx].abr);
}

AString nameToEnum(int idx)
{
	return cleanStr(ItemDefs[idx].name);
}

AString multToStr(int idx)
{
	if (idx == -1)
		return "\"\"";

	return AString("\"") + abbrToEnum(idx) + "\"";
}

AString multToStr2(int idx)
{
	if (idx == -1)
		return "\"\"";

	return AString("\"") + abbrToEnum(idx) + "\"";
}

AString itemIndexToString(int type, int idx, int baseidx)
{
	switch (type)
	{
	case 0:
	case IT_NORMAL:
	case IT_TRADE:
		return idx;

	case IT_MAN: return AString("MAN_") + ManDefs[idx].name;

	case IT_MONSTER:
		return AString("MONSTER_") + nameToEnum(baseidx);
	}

	if (type & IT_MOUNT)
		return AString("MOUNT_") + nameToEnum(baseidx);

	if (type & IT_WEAPON)
		return AString("WEAPON_") + nameToEnum(baseidx);

	if (type & IT_ARMOR)
		return AString("ARMOR_") + nameToEnum(baseidx);

	if (idx == 0)
		return "0";

	return "UNK_IDX";
}

AString itemFlagsToStr(int flags)
{
	switch (flags)
	{
	case 0: return "0";
	case ItemType::CANTGIVE: return "ItemType::CANTGIVE";
	case ItemType::DISABLED: return "ItemType::DISABLED";
	case ItemType::NOMARKET: return "ItemType::NOMARKET";
	case ItemType::NOSELL: return "ItemType::NOSELL";

	case ItemType::CANTGIVE | ItemType::DISABLED:
		return "ItemType::CANTGIVE|ItemType::DISABLED";

	case ItemType::DISABLED | ItemType::NOMARKET:
		return "ItemType::DISABLED | ItemType::NOMARKET";
	}
	return flags;
}

AString skillToStr(int skill)
{
	if (skill == -1)
		return "-1";

	AString ret = "S_";
	const char *skill_name = SkillDefs[skill].name;

	for (const char *str = skill_name; *str != 0; ++str)
	{
		if (*str == ' ')
			ret += '_';
		else
			ret += (char)toupper(*str);
	}

	return ret;
}

void dumpItem(Aoutfile &f, int baseindex, const ItemType &item)
{
	f.PutStr(AString("\t{\"") + item.name + "\",\"" +
	    item.names + "\",\"" + item.abr + "\",");
	f.PutStr(AString("\t ") + itemFlagsToStr(item.flags) + ",");

	AString p_inputs = "{";
	bool first = true;
	for (const auto &in : item.pInput)
	{
		if (!first)
			p_inputs += ",";
		first = false;

		p_inputs += "{";
		if (in.item == -1)
			p_inputs += "-1";
		else
			p_inputs += multToStr(in.item);

		p_inputs += ",";
		p_inputs += in.amt;
		p_inputs += "}";
	}
	p_inputs += "},";

	f.PutStr(AString("\t ") + skillToStr(item.pSkill) + "," + item.pLevel + "," +
	    item.pMonths + "," + item.pOut + ", " + p_inputs);

	AString m_inputs = "{";
	first = true;
	for (const auto &in : item.mInput)
	{
		if (!first)
			m_inputs += ",";
		first = false;

		m_inputs += "{";
		if (in.item == -1)
			m_inputs += "-1";
		else
			m_inputs += multToStr(in.item);

		m_inputs += ",";
		m_inputs += in.amt;
		m_inputs += "}";
	}
	m_inputs += "},";

	f.PutStr(AString("\t ") + skillToStr(item.mSkill) + "," + item.mLevel + "," +
	    item.mOut + ", " + m_inputs);

	f.PutStr(AString("\t ") + item.weight + ", " + itemTypeToString(item.type) + ", " +
	    item.baseprice + ", " + item.combat + ", " + itemIndexToString(item.type, item.index, baseindex) + ", " +
		 item.battleindex + ",");

	f.PutStr(AString("\t ") + item.walk + "," + item.ride + "," + item.fly + "," +
	    item.swim + ",");

	AString hitch_items = "{";
	first = true;
	for (const auto &h : item.hitchItems)
	{
		if (!first)
			hitch_items += ",";
		first = false;
		hitch_items += "{";
		hitch_items += h.item;
		hitch_items += ",";
		hitch_items += h.walk;
		hitch_items += "}";
	}
	hitch_items += "},";
	f.PutStr(AString("\t ") + hitch_items);

	AString mult_items = "{";
	first = true;
	for (const auto &m : item.mult_items)
	{
		if (!first)
			mult_items += ",";
		first = false;
		mult_items += "{";
		if (m.mult_item == -1)
			mult_items += "-1";
		else
			mult_items += multToStr(m.mult_item);
		mult_items += ",";
		mult_items += m.mult_val;
		mult_items += "}";
	}
	mult_items += "}";
	f.PutStr(AString("\t ") + mult_items + "},");
}

AString terrainFlags(int f)
{
	if (f == 0)
		return "0";
	//else TODO
	AString ret;
	if (f & TerrainType::RIDINGMOUNTS)
		ret += "TerrainType::RIDINGMOUNTS";

	if (f & TerrainType::FLYINGMOUNTS)
	{
		if (ret.len() != 0)
			ret += " | ";
		ret += "TerrainType::FLYINGMOUNTS";
	}

	return ret;
}

AString terrainProdToStr(const Product &p)
{
	AString ret = "{";
	ret += multToStr(p.product);
	ret += ",";
	ret += p.chance;
	ret += ",";
	ret += p.amount;
	ret += "}";
	return ret;
}

void dumpTerrain(Aoutfile &f, const TerrainType &tt)
{
	// line 1
	AString tmp = "\t{\"";
	tmp += tt.name;
	tmp += "\", ";

	tmp += "R_";
	tmp += cleanStr(TerrainDefs[tt.similar_type].name);
	tmp += ", ";

	tmp += terrainFlags(tt.flags);
	tmp += ",";
	f.PutStr(tmp);

	// line 2
	tmp = "\t ";
	tmp += tt.pop;
	tmp += ",";
	tmp += tt.wages;
	tmp += ",";
	tmp += tt.economy;
	tmp += ",";
	tmp += tt.movepoints;
	tmp += ",";
	f.PutStr(tmp);

	// lines 3-4 productarray
	tmp = "\t {";
	unsigned pi = 0;
	for (; pi < 4; ++pi)
	{
		tmp += terrainProdToStr(tt.prods[pi]);
		tmp += ",";
	}
	f.PutStr(tmp);
	tmp = "\t  ";
	tmp += terrainProdToStr(tt.prods[pi]);
	++pi;
	for (; pi < tt.prods.size(); ++pi)
	{
		tmp += ",";
		tmp += terrainProdToStr(tt.prods[pi]);
	}
	tmp += "},";
	f.PutStr(tmp);

	// line 5 races
	tmp = "\t {";
	for (unsigned i = 0; i < sizeof(tt.races)/sizeof(tt.races[0]); ++i)
	{
		if (i > 0)
			tmp += ",";

		tmp += multToStr(tt.races[i]);
	}
	tmp += "},";
	f.PutStr(tmp);

	// line 6 coastal races
	tmp = "\t {";
	for (unsigned i = 0; i < sizeof(tt.coastal_races)/sizeof(tt.coastal_races[0]); ++i)
	{
		if (i > 0)
			tmp += ",";

		tmp += multToStr(tt.coastal_races[i]);
	}
	tmp += "},";
	f.PutStr(tmp);

	// line 7
	tmp = "\t ";
	tmp += tt.wmonfreq;
	tmp += ",";
	tmp += multToStr2(tt.smallmon);
	tmp += ",";
	tmp += multToStr2(tt.bigmon);
	tmp += ",";
	tmp += multToStr2(tt.humanoid);
	tmp += ",";
	f.PutStr(tmp);

	// line 8
	tmp = "\t ";
	tmp += tt.lairChance;
	tmp += ",";
	tmp += "{";
	// lair array
	for (unsigned i = 0; i < sizeof(tt.lairs)/sizeof(tt.lairs[0]); ++i)
	{
		if (i > 0)
			tmp += ",";
		if (tt.lairs[i] == -1)
			tmp += "-1";
		else
		{
			tmp += "O_";
			tmp += cleanStr(ObjectDefs[tt.lairs[i]].name);
		}
	}
	tmp += "}},";
	f.PutStr(tmp);
}

void dumpMonster(Aoutfile &f, const MonType &mt)
{
	// line 1 - attack level and defense array
	AString tmp = "\t{";

	tmp += mt.attackLevel;
	tmp += ",{";
	for (unsigned i = 0; i < sizeof(mt.defense)/sizeof(mt.defense[0]); ++i)
	{
		if (i > 0)
			tmp += ",";
		tmp += mt.defense[i];
	}
	tmp += "},";
	f.PutStr(tmp);

	// line 2 - num attacks, hits, regen
	tmp = "\t ";
	tmp += mt.numAttacks;
	tmp += ",";
	tmp += mt.hits;
	tmp += ",";
	tmp += mt.regen;
	tmp += ",";
	f.PutStr(tmp);

	// line 3 - tactics, stealth, observation
	tmp = "\t ";
	tmp += mt.tactics;
	tmp += ",";
	tmp += mt.stealth;
	tmp += ",";
	tmp += mt.obs;
	tmp += ",";
	f.PutStr(tmp);

	// line 4 - special, special level
	tmp = "\t ";
	tmp += mt.special;
	tmp += ",";
	tmp += mt.specialLevel;
	tmp += ",";
	f.PutStr(tmp);

	// line 5 - silver, spoil type, hostile, number, name
	tmp = "\t ";
	tmp += mt.silver;
	tmp += ",";
	tmp += itemTypeToString(mt.spoiltype);
	tmp += ",";
	tmp += mt.hostile;
	tmp += ",";
	tmp += mt.number;
	tmp += ",";
	tmp += "\"";
	tmp += mt.name;
	tmp += "\"";

	// close entry
	tmp += "},";
	f.PutStr(tmp);
}

void doDumpData(Game &game, int argc, const char *argv[])
{
	if (argc != 3) {
		usage();
		return;
	}

	Aoutfile f;
	f.OpenByName(argv[2]);

	// header
	f.PutStr("// START A3HEADER");
	f.PutStr("//");
	f.PutStr("// This source file is part of the Atlantis PBM game program.");
	f.PutStr("// Copyright (C) 1995-1999 Geoff Dunbar");
	f.PutStr("//");
	f.PutStr("// This program is free software; you can redistribute it and/or");
	f.PutStr("// modify it under the terms of the GNU General Public License");
	f.PutStr("// as published by the Free Software Foundation; either version 2");
	f.PutStr("// of the License, or (at your option) any later version.");
	f.PutStr("//");
	f.PutStr("// This program is distributed in the hope that it will be useful,");
	f.PutStr("// but WITHOUT ANY WARRANTY; without even the implied warranty of");
	f.PutStr("// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the");
	f.PutStr("// GNU General Public License for more details.");
	f.PutStr("//");
	f.PutStr("// You should have received a copy of the GNU General Public License");
	f.PutStr("// along with this program, in the file license.txt. If not, write");
	f.PutStr("// to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,");
	f.PutStr("// Boston, MA 02111-1307, USA.");
	f.PutStr("//");
	f.PutStr("// See the Atlantis Project web page for details:");
	f.PutStr("// http://www.prankster.com/project");
	f.PutStr("//");
	f.PutStr("// END A3HEADER");
	f.PutStr("#include \"gamedata.h\"");
	f.PutStr("#include \"items.h\"");
	f.PutStr("#include \"skills.h\"");
	f.PutStr("#include \"object.h\"");
	f.PutStr("#include \"aregion.h\"");

	// field guides
	f.PutStr("");
	f.PutStr("//");
	f.PutStr("// Table of items");
	f.PutStr("//");
	f.PutStr("// name, plural, abbr,");
	f.PutStr("// flags,");
	f.PutStr("// pSkill, pLevel, pMonths, pOutput, pInput array");
	f.PutStr("// mSkill, mLevel, mOutput, mInput array");
	f.PutStr("// weight, type, baseprice, combat, index, battleindex");
	f.PutStr("// walk,ride,fly,swim,");
	f.PutStr("// hitchItems array");
	f.PutStr("// mult_item,mult_val");
	f.PutStr("");

	f.PutStr("std::vector<ItemType> ItemDefs = {");
	int baseindex = 0;
	for (const auto &item : ItemDefs)
	{
		dumpItem(f, baseindex, item);
		++baseindex;
	}
	f.PutStr("};");

	// TODO: man types

	// monster defs
	f.PutStr("");
	f.PutStr("//");
	f.PutStr("// Table of monsters.");
	f.PutStr("//");
	f.PutStr("static MonType md[NUMMONSTERS] = {");
	f.PutStr("\t// attackLevel, defense array");
	f.PutStr("\t// numAttacks, hits, regen,");
	f.PutStr("\t// tactics, stealth, obs");
	f.PutStr("\t// special, specialLevel,");
	f.PutStr("\t// silver spoiltype, hostile, number, name");
	f.PutStr("");
	for (unsigned i = 0; i < NUMMONSTERS; ++i)
	{
		dumpMonster(f, MonDefs[i]);
	}
	f.PutStr("};");
	f.PutStr("");
	f.PutStr("MonType * MonDefs = md;");

	// terrain defs
	f.PutStr("");
	f.PutStr("//");
	f.PutStr("// Table of terrain types.");
	f.PutStr("//");
	f.PutStr("static TerrainType td[R_NUM] = {");
	f.PutStr("\t//");
	f.PutStr("\t// name, similar_type, flags,");
	f.PutStr("\t// pop, wages, economy, movepoints,");
	f.PutStr("\t// product array (product, chance, amount)");
	f.PutStr("\t// normal races array");
	f.PutStr("\t// coastal races array");
	f.PutStr("\t// wmonfreq, smallmon, bigmon, humanoid,");
	f.PutStr("\t// lairChance, lair array");
	f.PutStr("\t//");
	for (unsigned i = 0; i < R_NUM; ++i)
	{
		dumpTerrain(f, TerrainDefs[i]);
	}
	f.PutStr("};");
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
	game.resolveBackRefs();

	typedef void Handler(Game &, int, const char*[]);

	const std::map<std::string, Handler*> cmds = {
		{"check", doCheck},
		{"dump", doDumpData},
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


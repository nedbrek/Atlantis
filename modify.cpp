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
#include "game.h"
#include "astring.h"
#include "items.h"
#include "gamedata.h"
#include "object.h"
#include "skills.h"

void Game::EnableSkill(int sk)
{
	if(sk < 0 || sk > (NSKILLS-1)) return;
	SkillDefs[sk].flags &= ~SkillType::DISABLED;
}

void Game::DisableSkill(int sk)
{
	if(sk < 0 || sk > (NSKILLS-1)) return;
	SkillDefs[sk].flags |= SkillType::DISABLED;
}

void Game::EnableItem(int item)
{
	if(item < 0 || item > int(ItemDefs.size()-1)) return;
	ItemDefs[item].flags &= ~ItemType::DISABLED;
}

void Game::DisableItem(int item)
{
	if(item < 0 || item > int(ItemDefs.size()-1)) return;
	ItemDefs[item].flags |= ItemType::DISABLED;
}

void Game::EnableObject(int obj)
{
	if(obj < 0 || obj > (NOBJECTS-1)) return;
	ObjectDefs[obj].flags &= ~ObjectType::DISABLED;
}

void Game::DisableObject(int obj)
{
	if(obj < 0 || obj > (NOBJECTS-1)) return;
	ObjectDefs[obj].flags |= ObjectType::DISABLED;
}

void Game::ModifyObjectFlags(int ob, int flags)
{
	if(ob < 0 || ob > (NOBJECTS-1)) return;
	ObjectDefs[ob].flags = flags;
}

void Game::ModifyObjectConstruction(int ob, int it, int num, int sk, int lev)
{
	if(ob < 0 || ob > (NOBJECTS-1)) return;
	if((it < -1 && it != I_WOOD_OR_STONE) || it > int(ItemDefs.size() -1))
		return;
	if(num < 0) return;
	if(sk < -1 || sk > (NSKILLS - 1)) return;
	if(lev < 0) return;
	ObjectDefs[ob].item = it;
	ObjectDefs[ob].cost = num;
	ObjectDefs[ob].skill = sk;
	ObjectDefs[ob].level = lev;
}

void Game::ModifyObjectManpower(int ob, int prot, int cap, int sail, int mages)
{
	if(ob < 0 || ob > (NOBJECTS-1)) return;
	if(prot < 0) return;
	if(cap < 0) return;
	if(sail < 0) return;
	if(mages < 0) return;
	ObjectDefs[ob].protect = prot;
	ObjectDefs[ob].capacity = cap;
	ObjectDefs[ob].sailors = sail;
	ObjectDefs[ob].maxMages = mages;
}

void Game::ClearTerrainRaces(int t)
{
	if(t < 0 || t > R_NUM-1) return;
	unsigned int c;
	for(c = 0; c < sizeof(TerrainDefs[t].races)/sizeof(int); c++) {
		TerrainDefs[t].races[c] = -1;
	}
	for(c = 0; c < sizeof(TerrainDefs[t].coastal_races)/sizeof(int); c++) {
		TerrainDefs[t].coastal_races[c] = -1;
	}
}

void Game::ModifyTerrainRace(int t, int i, const char *rname)
{
	int r = ParseEnabledItem(rname);
	if(t < 0 || t > (R_NUM -1)) return;
	if(i < 0 || i >= (int)(sizeof(TerrainDefs[t].races)/sizeof(int))) return;
	if(r < -1 || r > (int)ItemDefs.size()-1) r = -1;
	if(r != -1 && !(ItemDefs[r].type & IT_MAN)) r = -1;
	TerrainDefs[t].races[i] = r;
}

void Game::ModifyTerrainCoastRace(int t, int i, int r)
{
	if(t < 0 || t > (R_NUM -1)) return;
	if(i < 0 || i >= (int)(sizeof(TerrainDefs[t].coastal_races)/sizeof(int)))
		return;
	if(r < -1 || r > (int)ItemDefs.size()-1) r = -1;
	if(r != -1 && !(ItemDefs[r].type & IT_MAN)) r = -1;
	TerrainDefs[t].coastal_races[i] = r;
}

void Game::ClearTerrainItems(int terrain)
{
	if(terrain < 0 || terrain > R_NUM-1) return;

	for(unsigned int c = 0; c < TerrainDefs[terrain].prods.size(); c++)
	{
		TerrainDefs[terrain].prods[c].product = -1;
		TerrainDefs[terrain].prods[c].chance = 0;
		TerrainDefs[terrain].prods[c].amount = 0;
	}
}

void Game::ModifyTerrainItems(int terrain, int i, int p, int c, int a)
{
	if(terrain < 0 || terrain > (R_NUM -1)) return;
	if(i < 0 || i >= (int)TerrainDefs[terrain].prods.size())
		return;
	if(p < -1 || p > (int)ItemDefs.size()-1) p = -1;
	if(c < 0 || c > 100) c = 0;
	if(a < 0) a = 0;
	TerrainDefs[terrain].prods[i].product = p;
	TerrainDefs[terrain].prods[i].chance = c;
	TerrainDefs[terrain].prods[i].amount = a;
}

void Game::ModifyTerrainEconomy(int t, int pop, int wages, int econ, int move)
{
	if(t < 0 || t > (R_NUM -1)) return;
	if(pop < 0) pop = 0;
	if(wages < 0) wages = 0;
	if(econ < 0) econ = 0;
	if(move < 1) move = 1;
	TerrainDefs[t].pop = pop;
	TerrainDefs[t].wages = wages;
	TerrainDefs[t].economy = econ;
	TerrainDefs[t].movepoints = move;
}

void Game::ModifyRangeFlags(int range, int flags)
{
	if(range < 0 || range > (NUMRANGES-1)) return;
	RangeDefs[range].flags = flags;
}


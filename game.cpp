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
#include "faction.h"
#include "unit.h"
#include "gameio.h"
#include "fileio.h"
#include "astring.h"
#include "gamedata.h"
#include "object.h"
#include "orders.h"

#ifdef WIN32
#include <memory.h>  // Needed for memcpy on windows
#endif
#include <string.h>
#include <memory>

Game::Game()
{
}

Game::~Game()
{
	delete[] ppUnits;
	ppUnits = 0;
	maxppunits = 0;
}

void Game::resolveBackRefs()
{
	for (auto &i : ItemDefs)
	{
		//if (i.flags & ItemType::DISABLED)
			//continue;

		for (auto &m : i.pInput)
		{
			if (m.back_ref)
			{
				const int item_idx = ParseEnabledItem(m.back_ref);
				m.item = item_idx;
			}
		}
		for (auto &m : i.mInput)
		{
			if (m.back_ref)
			{
				const int item_idx = ParseEnabledItem(m.back_ref);
				m.item = item_idx;
			}
		}
		for (auto &m : i.hitchItems)
		{
			if (m.back_ref)
			{
				const int item_idx = ParseEnabledItem(m.back_ref);
				m.item = item_idx;
			}
		}
		for (auto &m : i.mult_items)
		{
			if (m.back_ref)
			{
				const int item_idx = ParseEnabledItem(m.back_ref);
				m.mult_item = item_idx;

				i.mult_item = item_idx; // TODO: remove me
			}
		}
	}
}

int Game::TurnNumber()
{
	return (year-1)*12 + month + 1;
}

// Insert default work order (and apply autotax)
void Game::DefaultWorkOrder()
{
	forlist(&regions)
	{
		ARegion *r = (ARegion*)elem;
		if (r->type == R_NEXUS)
			continue;

		forlist(&r->objects)
		{
			Object *o = (Object*)elem;
			for(auto &u : o->getUnits())
			{
				if (u->monthorders || u->faction->IsNPC() ||
				      (Globals->TAX_PILLAGE_MONTH_LONG &&
				       u->taxing != TAX_NONE))
				{
					continue;
				}

				if (u->GetFlag(FLAG_AUTOTAX) &&
				    (Globals->TAX_PILLAGE_MONTH_LONG && u->Taxers()))
				{
					u->taxing = TAX_AUTO;
				}
				else
				{
					if (Globals->DEFAULT_WORK_ORDER)
						ProcessWorkOrder(u, 0, true);
				}
			}
		}
	}
}

AString Game::GetXtraMap(ARegion *reg, int type)
{
	// extra information in ASCII map view
	switch (type)
	{
	// normal (show starting cities and shafts)
	case 0: return reg->IsStartingCity() ? "!" : (reg->HasShaft() ? "*" : " ");

	// wandering monster view
	case 1:
	{
		const int i = reg->CountWMons();
		return i ? AString(i) : AString(" ");
	}

	// lair view
	case 2:
		forlist(&reg->objects)
		{
			Object *o = (Object*)elem;
			if ((ObjectDefs[o->type].flags & ObjectType::CANENTER) != 0)
				continue; // player structure

			if (o->getUnits().front())
				return "*"; // occupied lair

			return "."; // unoccupied lair
		}

		return " "; // no lair

	// gate view
	case 3:
		return reg->gate ? "*" : " ";
	}

	return " ";
}

void Game::WriteSurfaceMap(Aoutfile *f, ARegionArray *pArr, const int type)
{
	// sweep y coordinate
	for (int y = 0; y < pArr->y; y += 2)
	{
		// sweep even x
		AString temp;
		for (int x = 0; x < pArr->x; x += 2)
		{
			ARegion *reg = pArr->GetRegion(x, y);
			temp += AString(GetRChar(reg));
			temp += GetXtraMap(reg, type);
			temp += "  ";
		}
		f->PutStr(temp);

		// sweep odd x
		temp = "  ";
		for (int x = 1; x < pArr->x; x += 2)
		{
			ARegion *reg = pArr->GetRegion(x, y+1);
			temp += AString(GetRChar(reg));
			temp += GetXtraMap(reg, type);
			temp += "  ";
		}
		f->PutStr(temp);
	}
	f->PutStr("");
}

void Game::WriteUnderworldMap(Aoutfile *f, ARegionArray *pArr, const int type)
{
	// sweep y coordinate
	for (int y = 0; y < pArr->y; y += 2)
	{
		AString temp = " ";
		AString temp2;

		// sweep even x
		for (int x = 0; x < pArr->x; x += 2)
		{
			ARegion *reg, *reg2;
			reg = pArr->GetRegion(x, y);
			reg2 = pArr->GetRegion(x+1, y+1);

			temp += AString(GetRChar(reg));
			temp += GetXtraMap(reg, type);

			if (reg2->neighbors[D_NORTH])
				temp += "|";
			else
				temp += " ";

			temp += " ";

			if (reg->neighbors[D_SOUTHWEST])
				temp2 += "/";
			else
				temp2 += " ";

			temp2 += " ";
			if (reg->neighbors[D_SOUTHEAST])
				temp2 += "\\";
			else
				temp2 += " ";

			temp2 += " ";
		}

		f->PutStr(temp);
		f->PutStr(temp2);
		temp = " ";
		temp2 = "  ";

		// sweep odd x
		for (int x = 1; x < pArr->x; x += 2)
		{
			ARegion *reg, *reg2;
			reg = pArr->GetRegion(x,y+1);
			reg2 = pArr->GetRegion(x-1, y);

			if (reg2->neighbors[D_SOUTH])
				temp += "|";
			else
				temp += " ";

			temp += AString(" ");
			temp += AString(GetRChar(reg));
			temp += GetXtraMap(reg,type);

			if (reg->neighbors[D_SOUTHWEST])
				temp2 += "/";
			else
				temp2 += " ";

			temp2 += " ";
			if (reg->neighbors[D_SOUTHEAST])
				temp2 += "\\";
			else
				temp2 += " ";

			temp2 += " ";
		}
		f->PutStr(temp);
		f->PutStr(temp2);
	}

	f->PutStr("");
}

int Game::ViewMap(const AString &typestr, const AString &mapfile)
{
	int type = 0;
	if (typestr == "wmon") type = 1;
	if (typestr == "lair") type = 2;
	if (typestr == "gate") type = 3;

	Aoutfile f;
	if (f.OpenByName(mapfile) == -1)
	{
		return 0;
	}

	switch (type)
	{
	case 0: f.PutStr("Geographical Map"); break;
	case 1: f.PutStr("Wandering Monster Map"); break;
	case 2: f.PutStr("Lair Map"); break;
	case 3: f.PutStr("Gate Map"); break;
	}

	for (int i = 0; i < regions.numLevels; ++i)
	{
		f.PutStr("");
		ARegionArray *pArr = regions.pRegionArrays[i];
		switch (pArr->levelType)
		{
		case ARegionArray::LEVEL_NEXUS:
			f.PutStr(AString("Level ") + i + ": Nexus");
			break;

		case ARegionArray::LEVEL_SURFACE:
			f.PutStr(AString("Level ") + i + ": Surface");
			WriteSurfaceMap(&f, pArr, type);
			break;

		case ARegionArray::LEVEL_UNDERWORLD:
			f.PutStr(AString("Level ") + i + ": Underworld");
			WriteUnderworldMap(&f, pArr, type);
			break;

		case ARegionArray::LEVEL_UNDERDEEP:
			f.PutStr(AString("Level ") + i + ": Underdeep");
			WriteUnderworldMap(&f, pArr, type);
			break;
		}
	}

	f.Close();

	return 1;
}

int Game::NewGame(int seed)
{
	factionseq = 1;
	guardfaction = 0;
	monfaction = 0;
	unitseq = 1;
	SetupUnitNums();
	shipseq = 100;
	year = 1;
	month = -1;
	gameStatus = GAME_STATUS_NEW;

	// seed the random number generator
	if (seed == 0) // unspecified seed
		seedrandomrandom();
	else // use a specific seed
		seedrandom(seed);

	CreateWorld();
	CreateNPCFactions();

	if (Globals->CITY_MONSTERS_EXIST)
		CreateCityMons();

	if (Globals->WANDERING_MONSTERS_EXIST)
		CreateWMons();

	if (Globals->LAIR_MONSTERS_EXIST)
	{
		CreateLMons();
		CreateVMons();
	}

	return 1;
}

// account for shift in array data
void adjustItemId(int &start, int amt, int min_val)
{
	if (start > min_val)
		start += amt;
}

int Game::OpenGame()
{
	// The order here must match the order in SaveGame()
	Ainfile f;
	if (f.OpenByName("game.in") == -1)
		return 0;

	// Read in Globals
	std::unique_ptr<AString> s1(f.GetStr());
	if (!s1)
		return 0;

	std::unique_ptr<AString> s2(s1->gettoken());
	if (!s2)
		return 0;

	if (! (*s2 == "atlantis_game"))
	{
		f.Close();
		return 0;
	}

	const ATL_VER eVersion = f.GetInt();
	Awrite(AString("Saved Game Engine Version: ") + ATL_VER_STRING(eVersion));
	if (ATL_VER_MAJOR(eVersion) != ATL_VER_MAJOR(CURRENT_ATL_VER) ||
	    ATL_VER_MINOR(eVersion) != ATL_VER_MINOR(CURRENT_ATL_VER))
	{
		Awrite("Incompatible Engine versions!");
		return 0;
	}

	if (ATL_VER_PATCH(eVersion) > ATL_VER_PATCH(CURRENT_ATL_VER))
	{
		Awrite("This game was created with a more recent Atlantis Engine!");
		return 0;
	}

	AString *gameName = f.GetStr();
	if (!gameName)
		return 0;

	if (!(*gameName == Globals->RULESET_NAME))
	{
		Awrite("Incompatible rule-set!");
		return 0;
	}

	ATL_VER gVersion = f.GetInt();
	Awrite(AString("Saved Rule-Set Version: ") + ATL_VER_STRING(gVersion));

	if (ATL_VER_MAJOR(gVersion) < ATL_VER_MAJOR(Globals->RULESET_VERSION))
	{
		Awrite(AString("Upgrading to ") +
		      ATL_VER_STRING(MAKE_ATL_VER(
		          ATL_VER_MAJOR(Globals->RULESET_VERSION), 0, 0)));
		if (!UpgradeMajorVersion(gVersion))
		{
			Awrite( "Unable to upgrade!  Aborting!" );
			return 0;
		}
		gVersion = MAKE_ATL_VER(ATL_VER_MAJOR(Globals->RULESET_VERSION), 0, 0);
	}

	if (ATL_VER_MINOR(gVersion) < ATL_VER_MINOR(Globals->RULESET_VERSION))
	{
		Awrite(AString("Upgrading to ") +
		      ATL_VER_STRING(MAKE_ATL_VER(
		            ATL_VER_MAJOR(Globals->RULESET_VERSION),
		            ATL_VER_MINOR(Globals->RULESET_VERSION), 0)));
		if (!UpgradeMinorVersion(gVersion))
		{
			Awrite("Unable to upgrade!  Aborting!");
			return 0;
		}

		gVersion = MAKE_ATL_VER( ATL_VER_MAJOR( gVersion ),
		      ATL_VER_MINOR( Globals->RULESET_VERSION ), 0);
	}

	if (ATL_VER_PATCH(gVersion) < ATL_VER_PATCH(Globals->RULESET_VERSION))
	{
		Awrite(AString("Upgrading to ") +
		      ATL_VER_STRING(Globals->RULESET_VERSION));
		if (!UpgradePatchLevel(gVersion))
		{
			Awrite("Unable to upgrade!  Aborting!");
			return 0;
		}

		gVersion = MAKE_ATL_VER( ATL_VER_MAJOR( gVersion ),
		      ATL_VER_MINOR( gVersion ),
		      ATL_VER_PATCH( Globals->RULESET_VERSION ) );
	}

	// when we get the stuff above fixed, we'll remove this junk
	// add a small hack to check to see whether we need to populate
	// ocean lairs
	doExtraInit = f.GetInt();
	if (doExtraInit > 0) {
		year = doExtraInit;
	} else {
		year = f.GetInt();
	}

	month = f.GetInt();
	seedrandom(f.GetInt());
	factionseq = f.GetInt();
	unitseq = f.GetInt();
	shipseq = f.GetInt();
	guardfaction = f.GetInt();
	monfaction = f.GetInt();

	// Read in the Factions
	const int num_factions = f.GetInt();
	for (int j = 0; j < num_factions; ++j)
	{
		Faction *temp = new Faction(*this);
		temp->Readin(&f, eVersion);
		factions.Add(temp);
	}

	// Read in the ARegions
	const int i = regions.ReadRegions(&f, &factions, eVersion);
	if (!i)
		return 0;

	// here we add ocean lairs
	if (doExtraInit > 0)
		CreateOceanLairs();

	FixBoatNums();
	FixGateNums();
	SetupUnitNums();

	f.Close();

	// fixup for adding FLEAD and 2 items after I_FOOD
	if (eVersion < MAKE_ATL_VER(4, 2, 49))
	{
		// adjust show list in factions
		{
		forlist (&factions)
		{
			Faction *f = (Faction*)elem;
			forlist (&f->items)
			{
				Item *item = (Item*)elem;
				adjustItemId(item->type, 1, -1);
				adjustItemId(item->type, 2, I_FOOD);
			}
		}
		}

		// in regions: adjust item ids in production, markets, and units
		forlist(&regions)
		{
			ARegion *r = (ARegion*)elem;

			adjustItemId(r->race, 1, -1);
			adjustItemId(r->race, 2, I_FOOD);

			{
			forlist((&r->products))
			{
				Production *p = (Production*)elem;
				adjustItemId(p->itemtype, 1, -1);
				adjustItemId(p->itemtype, 2, I_FOOD);
			}
			}

			{
			forlist((&r->markets))
			{
				Market *m = (Market*)elem;
				adjustItemId(m->item, 1, -1);
				adjustItemId(m->item, 2, I_FOOD);
			}
			}

			forlist(&r->objects)
			{
				Object *o = (Object*)elem;
				for(auto &u : o->getUnits())
				{
					forlist(&u->items)
					{
						Item *item = (Item*)elem;
						adjustItemId(item->type, 1, -1);
						adjustItemId(item->type, 2, I_FOOD);
					}

					if (u->readyItem != -1)
					{
						adjustItemId(u->readyItem, 1, -1);
						adjustItemId(u->readyItem, 2, I_FOOD);
					}

					for (unsigned i = 0; i < MAX_READY; ++i)
					{
						if (u->readyWeapon[i] != -1)
						{
							adjustItemId(u->readyWeapon[i], 1, -1);
							adjustItemId(u->readyWeapon[i], 2, I_FOOD);
						}

						if (u->readyArmor[i] != -1)
						{
							adjustItemId(u->readyArmor[i], 1, -1);
							adjustItemId(u->readyArmor[i], 2, I_FOOD);
						}
					}
				}
			}
		}
	}

	return 1; // success
}

int Game::SaveGame()
{
	Aoutfile f;
	if (f.OpenByName("game.out") == -1)
		return 0;

	// Write out Globals
	f.PutStr("atlantis_game");
	f.PutInt(CURRENT_ATL_VER);
	f.PutStr(Globals->RULESET_NAME);
	f.PutInt(Globals->RULESET_VERSION);

	// mark the fact that we created ocean lairs
	f.PutInt(-1);

	f.PutInt(year);
	f.PutInt(month);
	f.PutInt(getrandom(10000));
	f.PutInt(factionseq);
	f.PutInt(unitseq);
	f.PutInt(shipseq);
	f.PutInt(guardfaction);
	f.PutInt(monfaction);

	// Write out the Factions
	f.PutInt(factions.Num());

	forlist(&factions) {
		((Faction*)elem)->Writeout(&f);
	}

	// Write out the ARegions
	regions.WriteRegions(&f);

	f.Close();
	return 1; // success
}

void Game::DummyGame()
{
	// No need to set anything up; we're just syntax checking some orders
}

#define PLAYERS_FIRST_LINE "AtlantisPlayerStatus"

int Game::WritePlayers()
{
	Aoutfile f;
	if (f.OpenByName("players.out") == -1)
		return 0;

	f.PutStr(PLAYERS_FIRST_LINE);
	f.PutStr(AString("Version: ") + CURRENT_ATL_VER);
	f.PutStr(AString("TurnNumber: ") + TurnNumber());

	if (gameStatus == GAME_STATUS_UNINIT)
		return 0; // can't save

	if (gameStatus == GAME_STATUS_NEW)
	{
		f.PutStr(AString("GameStatus: New"));
	}
	else if (gameStatus == GAME_STATUS_RUNNING)
	{
		f.PutStr(AString("GameStatus: Running"));
	}
	else if (gameStatus == GAME_STATUS_FINISHED)
	{
		f.PutStr(AString("GameStatus: Finished"));
	}

	f.PutStr("");

	forlist (&factions)
	{
		Faction *fac = (Faction*)elem;
		fac->WriteFacInfo(&f);
	}

	f.Close();
	return 1; // success
}

int Game::ReadPlayers()
{
	Aorders f;
	if (f.OpenByName("players.in") == -1)
		return 0;

	// The first line of the file should match
	std::unique_ptr<AString> pLine(f.GetLine());
	if(!(*pLine == PLAYERS_FIRST_LINE))
		return 0;

	// Get the file version number
	pLine.reset(f.GetLine());

	std::unique_ptr<AString> pToken(pLine->gettoken());
	if (!pToken || !(*pToken == "Version:"))
		return 0;

	pToken.reset(pLine->gettoken());
	if (!pToken)
		return 0;

	const int nVer = pToken->value();
	if (ATL_VER_MAJOR(nVer) != ATL_VER_MAJOR(CURRENT_ATL_VER) ||
	    ATL_VER_MINOR(nVer) != ATL_VER_MINOR(CURRENT_ATL_VER) ||
	    ATL_VER_PATCH(nVer) > ATL_VER_PATCH(CURRENT_ATL_VER))
	{
		Awrite("The players.in file is not compatible with this version of Atlantis.");
		return 0;
	}

	// Ignore the turn number line
	pLine.reset(f.GetLine());

	// Next, the game status
	pLine.reset(f.GetLine());

	pToken.reset(pLine->gettoken());
	if (!pToken || !(*pToken == "GameStatus:"))
		return 0;

	// value for GameStatus
	pToken.reset(pLine->gettoken());
	if (!pToken)
		return 0;

	if (*pToken == "New")
	{
		gameStatus = GAME_STATUS_NEW;
	}
	else if (*pToken == "Running")
	{
		gameStatus = GAME_STATUS_RUNNING;
	}
	else if (*pToken == "Finished")
	{
		gameStatus = GAME_STATUS_FINISHED;
	}
	else
	{
		// The status doesn't seem to be valid
		return 0;
	}

	// Now, we should have a list of factions
	pLine.reset(f.GetLine());

	Faction *pFac = nullptr;
	int lastWasNew = 0;

	// OK, set our return code to success; we'll set it to fail below if necessary
	int rc = 1;

	// foreach remaining line
	while (pLine)
	{
		// pull first token
		pToken.reset(pLine->gettoken());
		if (!pToken)
		{
			// blank line, continue
			pLine.reset(f.GetLine());
			continue;
		}

		if (*pToken == "Faction:")
		{
			// Get the new faction
			pToken.reset(pLine->gettoken());
			if (!pToken)
			{
				rc = 0;
				break;
			}

			if (*pToken == "new")
			{
				pFac = AddFaction(1);
				if (!pFac)
				{
					Awrite("Failed to add a new faction!");
					rc = 0;
					break;
				}

				lastWasNew = 1;
			}
			else if (*pToken == "newNoLeader")
			{
				pFac = AddFaction(0);
				if (!pFac)
				{
					Awrite("Failed to add a new faction!");
					rc = 0;
					break;
				}
				lastWasNew = 1;
			}
			else
			{
				const int nFacNum = pToken->value();
				pFac = GetFaction(&factions, nFacNum);
				lastWasNew = 0;
			}
		}
		else if (pFac)
		{
			if (!ReadPlayersLine(pToken.get(), pLine.get(), pFac, lastWasNew))
			{
				rc = 0;
				break;
			}
		}

		pLine.reset(f.GetLine());
	}

	f.Close();

	return rc;
}

Unit* Game::ParseGMUnit(AString *tag, Faction *pFac)
{
	const char *str = tag->Str();

	// if not "gm"
	if (*str != 'g' || *(str+1) != 'm')
	{
		const int v = tag->value();
		if (unsigned(v) >= maxppunits)
			return nullptr; // beyond end of unit vec

		return GetUnit(v);
	}
	// else find gm added unit

	AString p(str+2);
	const int gma = p.value();
	forlist(&regions)
	{
		ARegion *reg = (ARegion *)elem;
		forlist(&reg->objects)
		{
			Object *obj = (Object *)elem;
			for(auto &u : obj->getUnits())
			{
				if (u->faction->num == pFac->num && u->gm_alias == gma)
				{
					return u; // found it
				}
			}
		}
	}
	return nullptr; // not found
}

int Game::ReadPlayersLine(AString *pToken, AString *pLine, Faction *pFac, int newPlayer)
{
	AString *pTemp = nullptr;

	if (*pToken == "Name:")
	{
		pTemp = pLine->StripWhite();
		if (pTemp)
		{
			if (newPlayer)
			{
				*pTemp += AString(" (") + pFac->num + ")";
			}
			pFac->SetNameNoChange(pTemp);
		}
	}
	else if (*pToken == "RewardTimes")
	{
		pFac->TimesReward();
	}
	else if (*pToken == "Email:")
	{
		pTemp = pLine->gettoken();
		if (pTemp)
		{
			delete pFac->address;
			pFac->address = pTemp;
			pTemp = nullptr;
		}
	}
	else if (*pToken == "Password:")
	{
		pTemp = pLine->StripWhite();
		delete pFac->password;
		if (pTemp)
		{
			pFac->password = pTemp;
			pTemp = nullptr;
		}
		else
		{
			pFac->password = nullptr;
		}
	}
	else if (*pToken == "Reward:")
	{
		pTemp = pLine->gettoken();
		const int nAmt = pTemp->value();
		pFac->Event(AString("Reward of ") + nAmt + " silver.");
		pFac->unclaimed += nAmt;
	}
	else if (*pToken == "SendTimes:")
	{
		// check for valid value?
		pTemp = pLine->gettoken();
		pFac->times = pTemp->value();
	}
	else if (*pToken == "LastOrders:")
	{
		// Read this line and correctly set the lastorders for this
		// faction if the game itself isn't maintaining them.
		pTemp = pLine->gettoken();
		if (Globals->LASTORDERS_MAINTAINED_BY_SCRIPTS)
			pFac->lastorders = pTemp->value();
	}
	else if (*pToken == "Loc:")
	{
		int x, y, z;
		pTemp = pLine->gettoken();
		if (pTemp)
		{
			x = pTemp->value();
			delete pTemp;
			pTemp = pLine->gettoken();
			if (pTemp)
			{
				y = pTemp->value();
				delete pTemp;
				pTemp = pLine->gettoken();
				if (pTemp)
				{
					z = pTemp->value();
					ARegion *pReg = regions.GetRegion(x, y, z);
					if (pReg)
					{
						pFac->pReg = pReg;
					}
					else
					{
						Awrite(AString("Invalid Loc:")+x+","+y+","+z+ " in faction " + pFac->num);
						pFac->pReg = nullptr;
					}
				}
			}
		}
	}
	else if (*pToken == "StartLoc:")
	{
		int x, y, z;
		pTemp = pLine->gettoken();
		if (pTemp)
		{
			x = pTemp->value();
			delete pTemp;
			pTemp = pLine->gettoken();
			if (pTemp)
			{
				y = pTemp->value();
				delete pTemp;
				pTemp = pLine->gettoken();
				if (pTemp)
				{
					z = pTemp->value();
					ARegion *pReg = regions.GetRegion( x, y, z );
					if (pReg)
					{
						if (!newPlayer)
						{
							Awrite(AString("StartLoc must be used on new faction (set on faction ") + pFac->num + ")");
							delete pTemp;
							return 1;
						}

						if (!pFac->pReg)
							pFac->pReg = pReg;

						// move starting units to start location
						for (unsigned uidx = 0; uidx < unitseq; ++uidx)
						{
							if (ppUnits[uidx] && ppUnits[uidx]->faction == pFac)
							{
								ppUnits[uidx]->MoveUnit( pReg->GetDummy() );
							}
						}
					}
					else
					{
						Awrite(AString("Invalid StartLoc:")+x+","+y+","+z+
						      " in faction " + pFac->num);
					}
				}
			}
		}
	}
	else if (*pToken == "NewUnit:")
	{
		// Creates a new unit in the location specified by a previous "Loc:" line
		// with a gm_alias of whatever is after the NewUnit: tag.
		if (!pFac->pReg)
		{
			Awrite(AString("NewUnit is not valid without a Loc: for faction ") + pFac->num);
		}
		else
		{
			pTemp = pLine->gettoken();
			if (!pTemp)
			{
				Awrite(AString("NewUnit: must be followed by an alias ") +
				      "in faction "+pFac->num);
			}
			else
			{
				const int val = pTemp->value();
				if (!val)
				{
					Awrite(AString("NewUnit: must be followed by an alias ") +
					      "in faction "+pFac->num);
				}
				else
				{
					Unit *u = GetNewUnit(pFac);
					u->gm_alias = val;
					u->MoveUnit(pFac->pReg->GetDummy());
					u->Event("Is given to your faction.");
				}
			}
		}
	}
	else if (*pToken == "Item:")
	{
		pTemp = pLine->gettoken();
		if (!pTemp)
		{
			Awrite(AString("Item: needs to specify a unit in faction ") + pFac->num);
		}
		else
		{
			Unit *u = ParseGMUnit(pTemp, pFac);
			if (!u)
			{
				Awrite(AString("Item: needs to specify a unit in faction ") + pFac->num);
			}
			else
			{
				if (u->faction->num != pFac->num)
				{
					Awrite(AString("Item: unit ")+ u->num +
					      " doesn't belong to " + "faction " + pFac->num);
				}
				else
				{
					delete pTemp;
					pTemp = pLine->gettoken();
					if (!pTemp)
					{
						Awrite(AString("Must specify a number of items to ") +
						      "give for Item: in faction " + pFac->num);
					}
					else
					{
						const int v = pTemp->value();
						if (!v)
						{
							Awrite(AString("Must specify a number of ") +
							         "items to give for Item: in " +
							         "faction " + pFac->num);
						}
						else
						{
							delete pTemp;
							pTemp = pLine->gettoken();
							if (!pTemp)
							{
								Awrite(AString("Must specify a valid item ") +
								      "to give for Item: in faction " + pFac->num);
							}
							else
							{
								const int it = ParseAllItems(*pTemp);
								if (it == -1)
								{
									Awrite(AString("Must specify a valid ") +
									      "item to give for Item: in " +
									      "faction " + pFac->num);
								}
								else
								{
									// pull current number for the item
									const int has = u->items.GetNum(it);
									u->items.SetNum(it, has + v);
									if (!u->gm_alias)
									{
										u->Event(AString("Is given ") +
										      ItemString(it, v) +
										      " by the gods.");
									}
									// make sure player has a description
									u->faction->DiscoverItem(it, 0, 1);
								}
							}
						}
					}
				}
			}
		}
	}
	else if (*pToken == "Skill:")
	{
		pTemp = pLine->gettoken();
		if (!pTemp)
		{
			Awrite(AString("Skill: needs to specify a unit in faction ") + pFac->num);
		}
		else
		{
			Unit *u = ParseGMUnit(pTemp, pFac);
			if (!u)
			{
				Awrite(AString("Skill: needs to specify a unit in faction ") + pFac->num);
			}
			else
			{
				if (u->faction->num != pFac->num)
				{
					Awrite(AString("Skill: unit ")+ u->num + " doesn't belong to " + "faction " + pFac->num);
				}
				else
				{
					delete pTemp;
					pTemp = pLine->gettoken();
					if (!pTemp)
					{
						Awrite(AString("Must specify a valid skill for ") + "Skill: in faction " + pFac->num);
					}
					else
					{
						const int sk = ParseSkill(pTemp);
						if (sk == -1)
						{
							Awrite(AString("Must specify a valid skill for ")+ "Skill: in faction " + pFac->num);
						}
						else
						{
							delete pTemp;
							pTemp = pLine->gettoken();
							if (!pTemp)
							{
								Awrite(AString("Must specify a days for ") + "Skill: in faction " + pFac->num);
							}
							else
							{
								const int days = pTemp->value() * u->GetMen();
								if (!days)
								{
									Awrite(AString("Must specify a days for ")+ "Skill: in faction " + pFac->num);
								}
								else
								{
									const int odays = u->skills.GetDays(sk); // original days
									u->skills.SetDays(sk, odays + days);
									u->AdjustSkills();

									// update max level in faction
									const int lvl = u->GetRealSkill(sk);
									if (lvl > pFac->skills.GetDays(sk))
									{
										pFac->skills.SetDays(sk, lvl);
										pFac->shows.Add(new ShowSkill(sk, lvl));
									}

									if (!u->gm_alias)
									{
										u->Event(AString("Is taught ") + days + " days of " +
										      SkillStrs(sk) + " by the gods.");
									}

									// This is NOT quite the same, but the gods
									// are more powerful than mere mortals
									const bool mage = (SkillDefs[sk].flags & SkillType::MAGIC) != 0;
									const bool app = (SkillDefs[sk].flags & SkillType::APPRENTICE) != 0;
									if (mage)
									{
										u->type = U_MAGE;
									}
									if (app && u->type == U_NORMAL)
									{
										u->type = U_APPRENTICE;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	else if (*pToken == "NoStartLeader")
	{
		pFac->noStartLeader = 1;
	}
	else if (*pToken == "Order:")
	{
		pTemp = pLine->StripWhite();
		if (*pTemp == "quit")
		{
			pFac->quit = QUIT_BY_GM;
		}
		else
		{
			// handle this as a unit order
			delete pTemp;
			pTemp = pLine->gettoken();
			if (!pTemp)
			{
				Awrite(AString("Order: needs to specify a unit in faction ") + pFac->num);
			}
			else
			{
				Unit *u = ParseGMUnit(pTemp, pFac);
				if (!u)
				{
					Awrite(AString("Order: needs to specify a unit in faction ") + pFac->num);
				}
				else
				{
					if (u->faction->num != pFac->num)
					{
						Awrite(AString("Order: unit ")+ u->num + " doesn't belong to " + "faction " + pFac->num);
					}
					else
					{
						delete pTemp;
						AString saveorder = *pLine;
						const int getatsign = pLine->getat();
						pTemp = pLine->gettoken();
						if (!pTemp)
						{
							Awrite(AString("Order: must provide unit order ") + "for faction " + pFac->num);
						}
						else
						{
							const int o = Parse1Order(pTemp);
							if (o == -1 || o == O_ATLANTIS || o == O_END ||
							    o == O_UNIT || o == O_FORM || o == O_ENDFORM)
							{
								Awrite(AString("Order: invalid order given ")+
										"for faction "+pFac->num);
							} else {
								if(getatsign) {
									u->oldorders.Add(new RawTextOrder(saveorder));
								}
								ProcessOrder(o, u, pLine, nullptr, 0);
							}
						}
					}
				}
			}
		}
	}
	else
	{
		pFac->extraPlayers_.emplace_back(new AString(*pToken + *pLine));
	}

	delete pTemp;
	return 1;
}

int Game::RunGame()
{
	Awrite("Setting Up Turn...");
	PreProcessTurn();

	Awrite("Reading the Gamemaster File...");
	if (!ReadPlayers())
		return 0;

	if (gameStatus == GAME_STATUS_FINISHED)
	{
		Awrite("This game is finished!");
		return 0;
	}
	gameStatus = GAME_STATUS_RUNNING;

	Awrite("Reading the Orders File...");
	ReadOrders();

	if (Globals->MAX_INACTIVE_TURNS != -1)
	{
		Awrite("QUITting Inactive Factions...");
		RemoveInactiveFactions();
	}

	Awrite("Running the Turn...");
	RunOrders();

	Awrite("Writing the Report File...");
	WriteReport();
	Awrite("");
	battles.DeleteAll();

	Awrite("Writing Playerinfo File...");
	WritePlayers();
	EmptyHell();

	Awrite("Removing Dead Factions...");
	DeleteDeadFactions();
	Awrite("done");

	return 1;
}

int Game::EditGame(int *pSaveGame)
{
	*pSaveGame = 0;

	Awrite("Editing an Atlantis Game: ");
	bool exit = false;

	while (!exit)
	{
		Awrite("Main Menu");
		Awrite("  1) Find a region...");
		Awrite("  2) Find a unit...");
		Awrite("  q) Quit without saving.");
		Awrite("  x) Exit and save.");
		Awrite("> ");

		// get user input
		AString *pStr = AGetString();
		Awrite("");

		if (*pStr == "q")
		{
			exit = true;
			Awrite("Quiting without saving.");
		}
		else if (*pStr == "x")
		{
			exit = true;
			*pSaveGame = 1;
			Awrite("Exit and save.");
		}
		else if (*pStr == "1")
		{
			ARegion *pReg = EditGameFindRegion();
			if (pReg)
			{
				EditGameRegion(pReg);
			}
		}
		else if (*pStr == "2")
		{
			EditGameFindUnit();
		}
		else
		{
			Awrite("Select from the menu.");
		}

		delete pStr;
	}

	return 1;
}

ARegion* Game::EditGameFindRegion()
{
	Awrite("Region coords (x y [z]):");

	// get user input
	std::unique_ptr<AString> pStr(AGetString());

	// pull first token
	std::unique_ptr<AString> pToken(pStr->gettoken());
	if (!pToken)
	{
		Awrite(AString("Bad input: ") + *pStr);
		return nullptr;
	}

	// put it in x
	const int x = pToken->value();

	// second token
	pToken.reset(pStr->gettoken());
	if (!pToken)
	{
		Awrite(AString("Bad input: ") + *pStr);
		return nullptr;
	}

	// put it in y
	const int y = pToken->value();

	// check for (optional) third token
	pToken.reset(pStr->gettoken());

	const int z = pToken ? pToken->value() : 1;

	ARegion *pReg = regions.GetRegion(x, y, z);
	if (!pReg)
	{
		Awrite("No such region.");
		return nullptr;
	}

	return pReg;
}

void Game::EditGameFindUnit()
{
	Awrite("Which unit number?");

	// get user input
	std::unique_ptr<AString> pStr(AGetString());

	const int num = pStr->value();

	Unit *pUnit = GetUnit(num);
	if (!pUnit)
	{
		Awrite(AString("No such unit: ") + *pStr);
		return;
	}

	EditGameUnit(pUnit);
}

void Game::EditGameRegion(ARegion *pReg)
{
	// TODO: Implement region editor
	Awrite("Not implemented yet.");
}

void Game::EditGameUnit(Unit *pUnit)
{
	// run unit quit
	bool exit = false;
	while (!exit)
	{
		Awrite(AString("Unit: ") + *pUnit->name);
		Awrite(AString("  in ") + pUnit->object->region->ShortPrint(&regions));
		Awrite("  1) Edit items...");
		Awrite("  2) Edit skills...");
		Awrite("  3) Move unit...");
		Awrite("  q) Stop editing this unit.");

		std::unique_ptr<AString> pStr(AGetString());

		if (*pStr == "1")
		{
			EditGameUnitItems(pUnit);
		}
		else if (*pStr == "2")
		{
			EditGameUnitSkills(pUnit);
		}
		else if (*pStr == "3")
		{
			EditGameUnitMove(pUnit);
		}
		else if (*pStr == "q")
		{
			exit = true;
		}
		else
		{
			Awrite("Select from the menu.");
		}
	}
}

void Game::EditGameUnitItems(Unit *pUnit)
{
	while (1)
	{
		Awrite(AString("Unit items: ") + pUnit->items.Report(2, 1, 1));
		Awrite("  [item] [number] to update an item.");
		Awrite("  q) Stop editing the items.");

		// get user input
		std::unique_ptr<AString> pStr(AGetString());

		if (*pStr == "q")
			return;

		// pull first token (item name)
		std::unique_ptr<AString> pToken(pStr->gettoken());
		if (!pToken)
		{
			Awrite("Try again.");
			continue;
		}

		// handles full name, plural, and abbr
		const int itemNum = ParseAllItems(*pToken);
		if (itemNum == -1)
		{
			Awrite(AString("No such item: ") + *pToken);
			continue;
		}

		if (ItemDefs[itemNum].flags & ItemType::DISABLED)
		{
			Awrite("Item disabled.");
			continue;
		}

		// pull (optional) second token
		pToken.reset(pStr->gettoken());

		const int num = pToken ? pToken->value() : 0;

		pUnit->items.SetNum(itemNum, num);

		// Mark it as known about for 'shows'
		pUnit->faction->items.SetNum(itemNum, 1);
	}
}

void Game::EditGameUnitSkills(Unit *pUnit)
{
	while (1)
	{
		Awrite(AString("Unit skills: ") + pUnit->skills.Report(pUnit->GetMen()));
		Awrite("  [skill] [days] to update a skill.");
		Awrite("  q) Stop editing the skills.");

		// get user input
		std::unique_ptr<AString> pStr(AGetString());

		if (*pStr == "q")
			return;

		// pull first token
		std::unique_ptr<AString> pToken(pStr->gettoken());
		if (!pToken)
		{
			Awrite(AString("Skill not found: ") + *pToken);
			continue;
		}

		const int skillNum = ParseSkill(pToken.get());
		if (skillNum == -1)
		{
			Awrite("No such skill.");
			continue;
		}

		if (SkillDefs[skillNum].flags & SkillType::DISABLED)
		{
			Awrite("Skill disabled.");
			continue;
		}

		// pull (optional) second token
		pToken.reset(pStr->gettoken());
		const int days = pToken ? pToken->value() : 0;

		if (days > 0)
		{
			// update unit type
			if ((SkillDefs[skillNum].flags & SkillType::MAGIC) && pUnit->type != U_MAGE)
			{
				pUnit->type = U_MAGE;
			}
			if ((SkillDefs[skillNum].flags & SkillType::APPRENTICE) && pUnit->type == U_NORMAL)
			{
				pUnit->type = U_APPRENTICE;
			}
		}
		// NOTE: no check for mage+appr -> normal

		pUnit->skills.SetDays(skillNum, days * pUnit->GetMen());

		// update faction max level
		const int lvl = pUnit->GetRealSkill(skillNum);
		if (lvl > pUnit->faction->skills.GetDays(skillNum))
		{
			pUnit->faction->skills.SetDays(skillNum, lvl);
		}
	}
}

void Game::EditGameUnitMove(Unit *pUnit)
{
	ARegion *pReg = EditGameFindRegion();
	if (pReg)
		pUnit->MoveUnit(pReg->GetDummy());
}

void Game::PreProcessTurn()
{
	++month;
	if (month > 11)
	{
		month = 0; // January
		++year;
	}

	SetupUnitNums();

	{
		forlist(&factions)
		{
			((Faction*)elem)->DefaultOrders();
//			((Faction*)elem)->TimesReward();
		}
	}

	{
		forlist(&regions)
		{
			ARegion *pReg = (ARegion*)elem;
			if (Globals->WEATHER_EXISTS)
			{
				pReg->SetWeather(regions.GetWeather(pReg, month));
			}
			pReg->DefaultOrders();
		}
	}
}

void Game::ClearOrders(Faction *f)
{
	forlist(&regions)
	{
		ARegion *r = (ARegion*)elem;
		r->applyToUnits([f](Unit *u)
		{
			if (u->faction == f)
			{
				u->ClearOrders();
			}
		}
		);
	}
}

void Game::ReadOrders()
{
	forlist(&factions)
	{
		Faction *fac = (Faction*)elem;
		if (fac->IsNPC())
			continue;

		AString str = "orders.";
		str += fac->num;

		Aorders file;
		if (file.OpenByName(str) != -1)
		{
			ParseOrders(fac->num, &file, 0);
			file.Close();
		}
		DefaultWorkOrder();
	}
}

void Game::MakeFactionReportLists()
{
	// factions aware of the region
	FactionVector vector(factionseq);

	//foreach region
	forlist(&regions)
	{
		ARegion *reg = (ARegion*)elem;

		vector.ClearVector(); // reset the vector

		// farsight
		forlist(&reg->farsees)
		{
			Faction *fac = ((Farsight*)elem)->faction;
			vector.SetFaction(fac->num, fac);
		}

		{
			// units passing through
			forlist(&reg->passers)
			{
				Faction *fac = ((Farsight*)elem)->faction;
				vector.SetFaction(fac->num, fac);
			}
		}

		// units present
		reg->applyToUnits([&vector](Unit *unit)
		{
			vector.SetFaction(unit->faction->num, unit->faction);
		}
		);

		for (int i = 0; i < vector.vectorsize; ++i)
		{
			Faction *fp = vector.GetFaction(i);
			if (!fp)
				continue;

			// why do regions have a race with zero pop?
			if (reg->race != -1 && reg->population > 0)
				fp->DiscoverItem(reg->race, 0, 1);

			fp->present_regions.Add(new ARegionPtr(reg));
		}
	}
}

void Game::WriteReport()
{
	MakeFactionReportLists();
	CountAllMages();

	if (Globals->APPRENTICES_EXIST)
		CountAllApprentices();

	Areport f; // avoid loop overhead

	forlist(&factions)
	{
		Faction *fac = (Faction*)elem;
		const AString str = AString("report.") + fac->num;

		if (!fac->IsNPC() ||
		    (((month == 0 && year == 1) || Globals->GM_REPORT) &&
		     fac->num == 1)
		   )
		{
			const int i = f.OpenByName(str);
			if (i == -1)
				continue; // failed to open!

			fac->WriteReport(&f, this);
			f.Close();
		}
		Adot();
	}
}

void Game::DeleteDeadFactions()
{
	forlist(&factions)
	{
		Faction *fac = (Faction*)elem;
		if (!fac->IsNPC() && !fac->exists)
		{
			factions.Remove(fac);
			forlist((&factions))
				((Faction*)elem)->RemoveAttitude(fac->num);
			delete fac;
		}
	}
}

Faction* Game::getFaction(int n)
{
	forlist(&factions)
	{
		Faction *fac = (Faction*)elem;
		if (fac->num == n)
			return fac;
	}
	return nullptr;
}

Faction* Game::AddFaction(int setup)
{
	// set up faction
	Faction *temp = new Faction(*this, factionseq);
	temp->SetAddress("NoAddress");
	temp->lastorders = TurnNumber();

	if (setup && !SetupFaction(temp))
	{
		// setup failed!
		delete temp;
		return nullptr;
	}

	factions.Add(temp);

	++factionseq;
	return temp;
}

void Game::ViewFactions()
{
	forlist(&factions)
		((Faction*)elem)->View();
}

void Game::SetupUnitSeq()
{
	int max = 0;
	forlist(&regions)
	{
		ARegion *r = (ARegion*)elem;
		r->applyToUnits([&max](Unit *u)
		{
			if (u && u->num > max)
				max = u->num;
		}
		);
	}
	unitseq = max+1;
}

void Game::FixGateNums()
{
	for (int i = 1; i <= regions.numberofgates; ++i)
	{
		// Look for a missing gate, and add it
		ARegion *tar = regions.FindGate(i);
		if (tar)
			continue; // This gate exists, continue

		while (1)
		{
			// Get the z coord, exclude the nexus (the abyss has type NEXUS, go figure)
			const int z = getrandom(regions.numLevels);
			ARegionArray *arr = regions.GetRegionArray(z);
			if (arr->levelType == ARegionArray::LEVEL_NEXUS)
				continue;

			// Get a random hex within that level
			const int x = getrandom(arr->x);
			const int y = getrandom(arr->y);

			tar = arr->GetRegion(x, y);
			if (!tar)
				continue;

			// Make sure the hex can have a gate and doesn't already
			if (TerrainDefs[tar->type].similar_type == R_OCEAN || tar->gate)
				continue;

			tar->gate = i;
			break;
		}
	}
}

void Game::FixBoatNums()
{
	forlist(&regions)
	{
		ARegion *r = (ARegion*)elem;
		forlist(&r->objects)
		{
			Object *o = (Object*)elem;
			if (ObjectIsShip(o->type) && o->num < 100)
			{
				o->num = shipseq++;
				o->SetName(new AString("Ship"));
			}
		}
	}
}

void Game::SetupUnitNums()
{
	SetupUnitSeq(); // capture max unit num into this->unitseq

	maxppunits = unitseq+10000;

	// rebuild pointer to pointers of units
	delete[] ppUnits;
	ppUnits = new Unit *[ maxppunits ];

	for (unsigned i = 0; i < maxppunits ; ++i)
		ppUnits[i] = nullptr;

	forlist(&regions)
	{
		ARegion *r = (ARegion*)elem;
		forlist(&r->objects)
		{
			Object *o = (Object*)elem;
			for(auto &u : o->getUnits())
			{
				const int i = u->num;

				if (i > 0 && unsigned(i) < maxppunits)
				{
					if (!ppUnits[i])
						ppUnits[i] = u;
					else
					{
						Awrite(AString("Error: Unit number ") + i + " multiply defined.");
						if (unitseq > 0 && unitseq < maxppunits)
						{
							u->num = unitseq;
							ppUnits[unitseq++] = u;
						}
					}
				}
				else
				{
					Awrite(AString("Error: Unit number ") + i + " out of range.");

					if (unitseq > 0 && unitseq < maxppunits)
					{
						u->num = unitseq;
						ppUnits[unitseq++] = u;
					}
				}
			}
		}
	}
}

Unit* Game::GetNewUnit(Faction *fac, int an)
{
	// look for a hole
	for (unsigned i = 1; i < unitseq; ++i)
	{
		if (ppUnits[i])
			continue;

		Unit *pUnit = new Unit(i, fac, an);
		ppUnits[i] = pUnit;
		return pUnit;
	}
	//else add to end

	Unit *pUnit = new Unit(unitseq, fac, an);
	ppUnits[unitseq] = pUnit;
	++unitseq;

	// grow pointer to pointers to units array
	if (unitseq >= maxppunits)
	{
		Unit **temp = new Unit*[maxppunits+10000];
		memcpy(temp, ppUnits, maxppunits*sizeof(Unit *));
		maxppunits += 10000;
		delete ppUnits;
		ppUnits = temp;
	}

	return pUnit;
}

Unit* Game::GetUnit(int num)
{
	if (num < 0 || unsigned(num) >= maxppunits)
		return nullptr;

	return ppUnits[num];
}

void Game::CountAllMages()
{
	// clear counts
	forlist(&factions)
	{
		((Faction*)elem)->nummages = 0;
	}

	{
		// recount
		forlist(&regions)
		{
			ARegion *r = (ARegion*)elem;
			r->applyToUnits([](Unit *u)
			{
				if (u->type == U_MAGE)
					++u->faction->nummages;
			}
			);
		}
	}
}

int Game::countItemFaction(int item_id, int faction_id)
{
	int count = 0;
	forlist(&regions)
	{
		ARegion *r = (ARegion*)elem;
		r->applyToUnits([&count, item_id, faction_id](Unit *u)
		{
			if (u->faction->num == faction_id)
				count += u->items.GetNum(item_id);
		}
		);
	}
	return count;
}

void Game::UnitFactionMap()
{
	Awrite("Opening units.txt");

	Aoutfile f;
	if (f.OpenByName("units.txt") == -1)
	{
		Awrite("Couldn't open file!");
		return;
	}
	Awrite(AString("Writing ") + unitseq + " units");


	for (unsigned i = 1; i < unitseq; i++)
	{
		Unit *u = GetUnit(i);
		if (!u)
		{
			Awrite("doesn't exist");
			continue;
		}

		AString tmp(AString(i) + ":" + u->faction->num);
		Awrite(tmp); // to screen
		f.PutStr(tmp); // to file
	}
	f.Close();
}


void Game::RemoveInactiveFactions()
{
	if (Globals->MAX_INACTIVE_TURNS == -1)
		return;

	const int cturn = TurnNumber();
	forlist(&factions)
	{
		Faction *fac = (Faction*)elem;
		if ((cturn - fac->lastorders) >= Globals->MAX_INACTIVE_TURNS && !fac->IsNPC())
		{
			fac->quit = QUIT_BY_GM;
		}
	}
}

void Game::CountAllApprentices()
{
	if (!Globals->APPRENTICES_EXIST)
		return;

	forlist(&factions)
	{
		((Faction*)elem)->numapprentices = 0;
	}

	{
		forlist(&regions)
		{
			ARegion *r = (ARegion*)elem;
			r->applyToUnits([](Unit *u)
			{
				if (u->type == U_APPRENTICE)
					u->faction->numapprentices++;
			}
			);
		}
	}
}

int Game::CountApprentices(Faction *pFac)
{
	int i = 0;
	forlist(&regions)
	{
		ARegion *r = (ARegion*)elem;
		forlist(&r->objects)
		{
			Object *o = (Object*)elem;
			for(auto &u : o->getUnits())
			{
				if (u->faction == pFac && u->type == U_APPRENTICE)
					++i;
			}
		}
	}
	return i;
}

int Game::AllowedMages(Faction *pFac)
{
	// return number of mages allowed by faction points
	int points = pFac->type[F_MAGIC];

	// bounds check
	if (points < 0) points = 0;
	if (points > allowedMagesSize - 1)
		points = allowedMagesSize - 1;

	return allowedMages[points];
}

int Game::AllowedApprentices(Faction *pFac)
{
	// return number of apprentices allowed by faction points
	int points = pFac->type[F_MAGIC];

	// bounds check
	if (points < 0) points = 0;
	if (points > allowedApprenticesSize - 1)
		points = allowedApprenticesSize - 1;

	return allowedApprentices[points];
}

int Game::AllowedTaxes(Faction *pFac)
{
	// return number of tax regions allowed by faction points
	int points = pFac->type[F_WAR];

	// bounds check
	if (points < 0) points = 0;
	if (points > allowedTaxesSize - 1)
		points = allowedTaxesSize - 1;

	return allowedTaxes[points];
}

int Game::AllowedTrades(Faction *pFac)
{
	// return number of trade regions allowed by faction points
	int points = pFac->type[F_TRADE];

	// bounds check
	if (points < 0) points = 0;
	if (points > allowedTradesSize - 1)
		points = allowedTradesSize - 1;

	return allowedTrades[points];
}

int Game::UpgradeMajorVersion(int savedVersion)
{
	return 1;
}

int Game::UpgradeMinorVersion(int savedVersion)
{
	return 1;
}

int Game::UpgradePatchLevel(int savedVersion)
{
	return 1;
}

// This will get called if we load an older version of the database which
// didn't have ocean lairs
void Game::CreateOceanLairs()
{
	// here's where we add the creation
	forlist (&regions)
	{
		ARegion *r = (ARegion*)elem;
		if (TerrainDefs[r->type].similar_type == R_OCEAN)
		{
			r->LairCheck();
		}
	}
}

void Game::MidProcessUnitExtra(ARegion *r, Unit *u)
{
	if (Globals->CHECK_MONSTER_CONTROL_MID_TURN)
		MonsterCheck(r, u);
}

void Game::PostProcessUnitExtra(ARegion *r, Unit *u)
{
	// don't check both mid and post
	if (!Globals->CHECK_MONSTER_CONTROL_MID_TURN)
		MonsterCheck(r, u);
}

int skillForSummon(int item_type)
{
	switch (item_type)
	{
	case I_IMP     : return S_SUMMON_IMPS;
	case I_DEMON   : return S_SUMMON_DEMON;
	case I_BALROG  : return S_SUMMON_BALROG;
	case I_SKELETON: return S_SUMMON_SKELETONS;
	case I_UNDEAD  : return S_RAISE_UNDEAD; // why not summon?
	case I_LICH    : return S_SUMMON_LICH;
	case I_WOLF    : return S_WOLF_LORE;
	case I_EAGLE   : return S_BIRD_LORE;
	case I_DRAGON  : return S_DRAGON_LORE;
	}
	return -1;
}

void Game::MonsterCheck(ARegion *r, Unit *u)
{
	if (u->type == U_WMON)
		return; // wandering monsters don't change

	int escape = 0; // demons escape
	int totlosses = 0;

	forlist (&u->items)
	{
		Item *i = (Item*)elem;
		if (!i->num)
			continue; // why is there an item with 0 count?

		const int skill = skillForSummon(i->type);
		if (skill == -1)
			continue;

		// TODO -- This should be genericized -- heavily!

		// loss of control
		if (i->type == I_IMP || i->type == I_DEMON || i->type == I_BALROG)
		{
			// top of random range (can't lose more than we have)
			const int top = i->num * i->num;

			// pull skill level
			const int level = u->GetSkill(skill);
			if (!level)
			{
				// no skill - auto escape
				escape = 10000;
			}
			else // max chance
			{
				int bottom = level * level;
				if (i->type == I_IMP) bottom *= 4;

				bottom = bottom * bottom;

				if (i->type != I_BALROG) bottom *= 20;
				else bottom *= 4;

				const int chance = (top * 10000) / bottom;

				// set escape to largest chance
				if (chance > escape) escape = chance;
			}
		}

		// decay (not actually based on skill level)
		if (i->type == I_SKELETON || i->type == I_UNDEAD || i->type == I_LICH)
		{
			const int losses = (i->num + getrandom(10)) / 10;
			u->items.SetNum(i->type, i->num - losses);
			totlosses += losses;
		}

		// check control limits
		if (i->type == I_WOLF || i->type == I_EAGLE || i->type == I_DRAGON)
		{
			// Enforce 1 dragon limit (for in-progress games)
			if (i->type == I_DRAGON && i->num > 1)
			{
				u->Event(AString("Loses control of ") + ItemString(i->type, i->num-1) + ".");
				u->items.SetNum(I_DRAGON, 1);
			}

			const int level = u->GetSkill(skill);
			if (!level)
			{
				if (Globals->WANDERING_MONSTERS_EXIST && Globals->RELEASE_MONSTERS)
				{
					Faction *mfac = GetFaction(&factions, monfaction);
					Unit *mon = GetNewUnit(mfac, 0);
					const int mondef = ItemDefs[i->type].index;
					mon->MakeWMon(MonDefs[mondef].name, i->type, i->num);
					mon->MoveUnit(r->GetDummy());

					// This will be zero unless these are set. (0 means full spoils)
					mon->free = Globals->MONSTER_NO_SPOILS + Globals->MONSTER_SPOILS_RECOVERY;
				}

				u->Event(AString("Loses control of ") + ItemString(i->type, i->num) + ".");
				u->items.SetNum(i->type, 0);
			}
		}
	}

	if (totlosses)
	{
		u->Event(AString(totlosses) + " undead decay into nothingness.");
	}

	// check escape
	if (escape <= getrandom(10000))
		return;

	if (!Globals->WANDERING_MONSTERS_EXIST)
	{
		u->items.SetNum(I_IMP, 0);
		u->items.SetNum(I_DEMON, 0);
		u->items.SetNum(I_BALROG, 0);
		u->Event("Summoned demons vanish.");
		return;
	}

	Faction *mfac = GetFaction(&factions, monfaction);
	if (u->items.GetNum(I_IMP))
	{
		Unit *mon = GetNewUnit(mfac, 0);
		mon->MakeWMon(MonDefs[MONSTER_IMP].name, I_IMP, u->items.GetNum(I_IMP));
		mon->MoveUnit(r->GetDummy());
		u->items.SetNum(I_IMP, 0);

		// This will be zero unless these are set. (0 means full spoils)
		mon->free = Globals->MONSTER_NO_SPOILS + Globals->MONSTER_SPOILS_RECOVERY;
	}

	if (u->items.GetNum(I_DEMON))
	{
		Unit *mon = GetNewUnit(mfac, 0);
		mon->MakeWMon(MonDefs[MONSTER_DEMON].name, I_DEMON, u->items.GetNum(I_DEMON));
		mon->MoveUnit(r->GetDummy());
		u->items.SetNum(I_DEMON, 0);

		// This will be zero unless these are set. (0 means full spoils)
		mon->free = Globals->MONSTER_NO_SPOILS + Globals->MONSTER_SPOILS_RECOVERY;
	}

	if (u->items.GetNum(I_BALROG))
	{
		Unit *mon = GetNewUnit(mfac, 0);
		mon->MakeWMon(MonDefs[MONSTER_BALROG].name, I_BALROG, u->items.GetNum(I_BALROG));
		mon->MoveUnit(r->GetDummy());
		u->items.SetNum(I_BALROG, 0);
		// This will be zero unless these are set. (0 means full spoils)
		mon->free = Globals->MONSTER_NO_SPOILS + Globals->MONSTER_SPOILS_RECOVERY;
	}

	u->Event("Controlled demons break free!");
}

void Game::CheckUnitMaintenance(int consume)
{
	CheckUnitMaintenanceItem(I_FOOD, Globals->UPKEEP_FOOD_VALUE, consume);
	CheckUnitMaintenanceItem(I_GRAIN, Globals->UPKEEP_FOOD_VALUE, consume);
	CheckUnitMaintenanceItem(I_LIVESTOCK, Globals->UPKEEP_FOOD_VALUE, consume);
	CheckUnitMaintenanceItem(I_FISH, Globals->UPKEEP_FOOD_VALUE, consume);
}

void Game::CheckFactionMaintenance(int con)
{
	CheckFactionMaintenanceItem(I_FOOD, Globals->UPKEEP_FOOD_VALUE, con);
	CheckFactionMaintenanceItem(I_GRAIN, Globals->UPKEEP_FOOD_VALUE, con);
	CheckFactionMaintenanceItem(I_LIVESTOCK, Globals->UPKEEP_FOOD_VALUE, con);
	CheckFactionMaintenanceItem(I_FISH, Globals->UPKEEP_FOOD_VALUE, con);
}

void Game::CheckAllyMaintenance()
{
	CheckAllyMaintenanceItem(I_FOOD, Globals->UPKEEP_FOOD_VALUE);
	CheckAllyMaintenanceItem(I_GRAIN, Globals->UPKEEP_FOOD_VALUE);
	CheckAllyMaintenanceItem(I_LIVESTOCK, Globals->UPKEEP_FOOD_VALUE);
	CheckAllyMaintenanceItem(I_FISH, Globals->UPKEEP_FOOD_VALUE);
}

void Game::CheckUnitHunger()
{
	CheckUnitHungerItem(I_FOOD, Globals->UPKEEP_FOOD_VALUE);
	CheckUnitHungerItem(I_GRAIN, Globals->UPKEEP_FOOD_VALUE);
	CheckUnitHungerItem(I_LIVESTOCK, Globals->UPKEEP_FOOD_VALUE);
	CheckUnitHungerItem(I_FISH, Globals->UPKEEP_FOOD_VALUE);
}

void Game::CheckFactionHunger()
{
	CheckFactionHungerItem(I_FOOD, Globals->UPKEEP_FOOD_VALUE);
	CheckFactionHungerItem(I_GRAIN, Globals->UPKEEP_FOOD_VALUE);
	CheckFactionHungerItem(I_LIVESTOCK, Globals->UPKEEP_FOOD_VALUE);
	CheckFactionHungerItem(I_FISH, Globals->UPKEEP_FOOD_VALUE);
}

void Game::CheckAllyHunger()
{
	CheckAllyHungerItem(I_FOOD, Globals->UPKEEP_FOOD_VALUE);
	CheckAllyHungerItem(I_GRAIN, Globals->UPKEEP_FOOD_VALUE);
	CheckAllyHungerItem(I_LIVESTOCK, Globals->UPKEEP_FOOD_VALUE);
	CheckAllyHungerItem(I_FISH, Globals->UPKEEP_FOOD_VALUE);
}

char Game::GetRChar(ARegion *r)
{
	char c;
	switch (r->type)
	{
	case R_OCEAN: return '-';
	case R_LAKE : return '-';
	case R_PLAIN       : c = 'p'; break;
	case R_CERAN_PLAIN1: c = 'p'; break;
	case R_CERAN_PLAIN2: c = 'p'; break;
	case R_CERAN_PLAIN3: c = 'p'; break;
	case R_FOREST       : c = 'f'; break;
	case R_CERAN_FOREST1: c = 'f'; break;
	case R_CERAN_FOREST2: c = 'f'; break;
	case R_CERAN_FOREST3: c = 'f'; break;
	case R_CERAN_MYSTFOREST : c = 'y'; break;
	case R_CERAN_MYSTFOREST1: c = 'y'; break;
	case R_CERAN_MYSTFOREST2: c = 'y'; break;
	case R_MOUNTAIN       : c = 'm'; break;
	case R_CERAN_MOUNTAIN1: c = 'm'; break;
	case R_CERAN_MOUNTAIN2: c = 'm'; break;
	case R_CERAN_MOUNTAIN3: c = 'm'; break;
	case R_CERAN_HILL : c = 'h'; break;
	case R_CERAN_HILL1: c = 'h'; break;
	case R_CERAN_HILL2: c = 'h'; break;
	case R_SWAMP       : c = 's'; break;
	case R_CERAN_SWAMP1: c = 's'; break;
	case R_CERAN_SWAMP2: c = 's'; break;
	case R_CERAN_SWAMP3: c = 's'; break;
	case R_JUNGLE       : c = 'j'; break;
	case R_CERAN_JUNGLE1: c = 'j'; break;
	case R_CERAN_JUNGLE2: c = 'j'; break;
	case R_CERAN_JUNGLE3: c = 'j'; break;
	case R_DESERT       : c = 'd'; break;
	case R_CERAN_DESERT1: c = 'd'; break;
	case R_CERAN_DESERT2: c = 'd'; break;
	case R_CERAN_DESERT3: c = 'd'; break;
	case R_CERAN_WASTELAND : c = 'z'; break;
	case R_CERAN_WASTELAND1: c = 'z'; break;
	case R_CERAN_LAKE: c = 'l'; break;
	case R_TUNDRA       : c = 't'; break;
	case R_CERAN_TUNDRA1: c = 't'; break;
	case R_CERAN_TUNDRA2: c = 't'; break;
	case R_CERAN_TUNDRA3: c = 't'; break;
	case R_CAVERN       : c = 'c'; break;
	case R_CERAN_CAVERN1: c = 'c'; break;
	case R_CERAN_CAVERN2: c = 'c'; break;
	case R_CERAN_CAVERN3: c = 'c'; break;
	case R_UFOREST       : c = 'u'; break;
	case R_CERAN_UFOREST1: c = 'u'; break;
	case R_CERAN_UFOREST2: c = 'u'; break;
	case R_CERAN_UFOREST3: c = 'u'; break;
	case R_TUNNELS       : c = 't'; break;
	case R_CERAN_TUNNELS1: c = 't'; break;
	case R_CERAN_TUNNELS2: c = 't'; break;
	case R_ISLAND_PLAIN: c = 'a'; break;
	case R_ISLAND_MOUNTAIN: c = 'n'; break;
	case R_ISLAND_SWAMP: c = 'w'; break;
	case R_VOLCANO: c = 'v'; break;
	case R_CHASM       : c = 'l'; break;
	case R_CERAN_CHASM1: c = 'l'; break;
	case R_GROTTO       : c = 'g'; break;
	case R_CERAN_GROTTO1: c = 'g'; break;
	case R_DFOREST       : c = 'e'; break;
	case R_CERAN_DFOREST1: c = 'e'; break;
	default: return '?';
	}

	// upper case for town
	if (r->town)
	{
		c = (c - 'a') + 'A';
	}

	return c;
}

void Game::CreateNPCFactions()
{
	if (Globals->CITY_MONSTERS_EXIST)
	{
		Faction *f = new Faction(*this, factionseq++);
		guardfaction = f->num; // save faction number for guards
		f->SetName(new AString("The Guardsmen"));
		f->SetNPC();
		f->lastorders = 0;
		factions.Add(f);
	}

	// Only create the monster faction if wandering monsters or lair
	// monsters exist.
	if (Globals->LAIR_MONSTERS_EXIST || Globals->WANDERING_MONSTERS_EXIST)
	{
		Faction *f = new Faction(*this, factionseq++);
		monfaction = f->num; // save faction number for monsters
		f->SetName(new AString("Creatures"));
		f->SetNPC();
		f->lastorders = 0;
		factions.Add(f);
	}
}

void Game::CreateCityMon(ARegion *pReg, int percent, int needmage)
{
	// invincible
	const bool IV = pReg->type == R_NEXUS || (Globals->SAFE_START_CITIES && pReg->IsStartingCity());
	const bool AC = pReg->type == R_NEXUS || pReg->IsStartingCity();

	// starting cities are always "CITY", no city is -1 (0 is a valid city type)
	const int skilllevel = AC ? (TOWN_CITY + 1) : (pReg->town->TownType() + 1);

	// starting cities use the global num, others scale by size
	int num = AC ? Globals->AMT_START_CITY_GUARDS : Globals->CITY_GUARD * skilllevel;

	// scale num by arg
	num = num * percent / 100;

	Faction *pFac = GetFaction(&factions, guardfaction);

	Unit *u = GetNewUnit(pFac);
	u->SetName(new AString("City Guard"));
	u->type = U_GUARD;
	u->guard = GUARD_GUARD;

	// Use local race for guards, where possible
	int guard_race = I_LEADERS;
	if (pReg->race > 0) { guard_race = pReg->race; }
	u->SetMen(guard_race, num);

	// give them swords
	const int sword_idx = ParseEnabledItem("sword");
	if (sword_idx != -1)
		u->items.SetNum(sword_idx, num);

	// if invincible, give amulets
	if (IV) u->items.SetNum(I_AMULETOFI, num);

	// give money
	u->SetMoney(num * Globals->GUARD_MONEY);

	// give COMB
	u->SetSkill(S_COMBAT, skilllevel);

	if (AC)
	{
		// plate, if enabled
		if (Globals->START_CITY_GUARDS_PLATE)
		{
			const int plate_idx = ParseEnabledItem("plate armor");
			if (plate_idx != -1)
				u->items.SetNum(plate_idx, num);
		}

		// super observant
		u->SetSkill(S_OBSERVATION, 10);

		// tactics, if enabled
		if (Globals->START_CITY_TACTICS)
			u->SetSkill(S_TACTICS, Globals->START_CITY_TACTICS);
	}
	else
	{
		// give OBSE
		u->SetSkill(S_OBSERVATION, skilllevel);
	}

	// set to hold
	u->SetFlag(FLAG_HOLDING, 1);

	// place in open area
	u->MoveUnit(pReg->GetDummy());

	if (AC && Globals->START_CITY_MAGES && needmage)
	{
		// create city mage
		u = GetNewUnit(pFac);
		u->SetName(new AString("City Mage"));
		u->type = U_GUARDMAGE;
		u->SetMen(I_LEADERS, 1);

		// if invincible, give amulet
		if (IV) u->items.SetNum(I_AMULETOFI, 1);

		// give money
		u->SetMoney(Globals->GUARD_MONEY);

		// give FIRE (and FORC)
		u->SetSkill(S_FORCE, Globals->START_CITY_MAGES);
		u->SetSkill(S_FIRE, Globals->START_CITY_MAGES);

		u->combat = S_FIRE; // set for combat

		// give TACT
		if(Globals->START_CITY_TACTICS)
			u->SetSkill(S_TACTICS, Globals->START_CITY_TACTICS);

		// set behind and hold
		u->SetFlag(FLAG_BEHIND, 1);
		u->SetFlag(FLAG_HOLDING, 1);

		// place in open area
		u->MoveUnit(pReg->GetDummy());
	}
}

void Game::AdjustCityMons(ARegion *r)
{
	int needguard = 1; // can guards regenerate
	int needmage = 1; // need new mage
	forlist(&r->objects)
	{
		Object *o = (Object*)elem;
		for(auto &u : o->getUnits())
		{
			if (u->type == U_GUARD || u->type == U_GUARDMAGE)
			{
				// existing guards are adjusted
				AdjustCityMon(r, u);

				// don't create new city guards
				needguard = 0;
				if (u->type == U_GUARDMAGE)
					needmage = 0;
			}

			// players on guard prevent regen
			if (u->guard == GUARD_GUARD)
				needguard = 0;
		}
	}

	if (needguard && getrandom(100) < Globals->GUARD_REGEN)
	{
		// 10% regen
		CreateCityMon(r, 10, needmage);
	}
}

void Game::AdjustCityMon(ARegion *r, Unit *u)
{
	const bool AC = r->type == R_NEXUS || r->IsStartingCity();

	// force starting cities to type CITY
	const int towntype = AC ? TOWN_CITY : r->town->TownType();

	// invincible
	const bool IV = r->type == R_NEXUS || (Globals->SAFE_START_CITIES && r->IsStartingCity());

	int men;
	if (r->type == R_NEXUS || r->IsStartingCity())
	{
		if (u->type == U_GUARDMAGE)
		{
			men = 1;
		}
		else
		{
			men = u->GetMen() + (Globals->AMT_START_CITY_GUARDS/10);
			if (men > Globals->AMT_START_CITY_GUARDS)
				men = Globals->AMT_START_CITY_GUARDS;
		}
	}
	else
	{
		men = u->GetMen() + (Globals->CITY_GUARD/10)*(towntype+1);
		if (men > Globals->CITY_GUARD * (towntype+1))
			men = Globals->CITY_GUARD * (towntype+1);
	}

	// Use local race for guards, where possible
	int guard_race = I_LEADERS;
	if (r->race > 0) { guard_race = r->race; }
	u->SetMen(guard_race, men);

	if (IV) u->items.SetNum(I_AMULETOFI, men);

	if (u->type == U_GUARDMAGE)
	{
		if (Globals->START_CITY_TACTICS)
			u->SetSkill(S_TACTICS, Globals->START_CITY_TACTICS);

		u->SetSkill(S_FORCE, Globals->START_CITY_MAGES);
		u->SetSkill(S_FIRE, Globals->START_CITY_MAGES);
		u->combat = S_FIRE;

		u->SetFlag(FLAG_BEHIND, 1);
		u->SetMoney(Globals->GUARD_MONEY);
	}
	else
	{
		u->SetMoney(men * Globals->GUARD_MONEY);
		u->SetSkill(S_COMBAT, towntype + 1);

		if (AC)
		{
			u->SetSkill(S_OBSERVATION, 10);
			if (Globals->START_CITY_TACTICS)
				u->SetSkill(S_TACTICS, Globals->START_CITY_TACTICS);

			if (Globals->START_CITY_GUARDS_PLATE)
			{
				const int plate_idx = ParseEnabledItem("plate armor");
				if (plate_idx != -1)
					u->items.SetNum(plate_idx, men);
			}
		}
		else
		{
			u->SetSkill(S_OBSERVATION, towntype + 1);
		}
		const int sword_idx = ParseEnabledItem("sword");
		if (sword_idx != -1)
			u->items.SetNum(sword_idx, men);
	}
}

int Game::GetAvgMaintPerMan()
{
	int man_count = 0;
	int hits_sum = 0;

	for (unsigned i = 0; i < ItemDefs.size(); ++i)
	{
		if (!(ItemDefs[i].type & IT_MAN)) { continue; }
		if (ItemDefs[i].flags & ItemType::DISABLED) { continue; }
		if (i == I_LEADERS || i == I_FACTIONLEADER) { continue; }

		++man_count;
		hits_sum += ManDefs[ItemDefs[i].index].hits;
	}

	return hits_sum / man_count * Globals->MAINTENANCE_COST;
}

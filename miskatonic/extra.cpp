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
//
// This file contains extra game-specific functions
//
#include "game.h"
#include "gamedata.h"

int Game::SetupFaction( Faction *pFac )
{
    pFac->unclaimed = Globals->START_MONEY + TurnNumber() * 100;

	if(pFac->noStartLeader)
		return 1;

    ARegion *reg = NULL;
	if(pFac->pStartLoc) {
		reg = pFac->pStartLoc;
	} else if(!Globals->MULTI_HEX_NEXUS) {
		reg = (ARegion *)(regions.First());
	} else {
		ARegionArray *pArr = regions.GetRegionArray(ARegionArray::LEVEL_NEXUS);
		while(!reg) {
			reg = pArr->GetRegion(getrandom(pArr->x), getrandom(pArr->y));
		}
	}

	// Setup Faction Leader
    Unit *leader = GetNewUnit( pFac );

    leader->SetMen( I_LEADERS, 1 );
	pFac->DiscoverItem(I_LEADERS, 0, 1);

	// Flags
    leader->reveal = REVEAL_FACTION;
	leader->SetFlag(FLAG_BEHIND,1);

	// Skills
    leader->type = U_MAGE;
    leader->Study(S_PATTERN, 30);
    leader->Study(S_SPIRIT, 30);
	leader->Study(S_FORCE, 30);
	leader->Study(S_FIRE, 30);
	leader->Study(S_COMBAT, 30);
	leader->Study(S_TACTICS, 30);
	leader->Study(S_GATE_LORE, 30);

	// Set combat spell
	leader->combat = S_FIRE;

	// Special wizard items
	leader->items.SetNum(I_WIZARDSTAFF, 1);
	pFac->DiscoverItem(I_WIZARDSTAFF, 0, 1);
	leader->items.SetNum(I_WIZROBE, 1);
	pFac->DiscoverItem(I_WIZROBE, 0, 1);

	// Horses too heavy for base GATE LORE skill
	if (!Globals->NEXUS_NO_EXITS) {
		leader->items.SetNum(I_HORSE, 1);
		pFac->DiscoverItem(I_HORSE, 0, 1);
	}

	if(TurnNumber() >= 24) {
		leader->Study(S_PATTERN, 60);
		leader->Study(S_SPIRIT, 60);
		leader->Study(S_FORCE, 90);
		leader->Study(S_EARTH_LORE, 30);
		leader->Study(S_STEALTH, 30);
		leader->Study(S_OBSERVATION, 30);
	}
	if(TurnNumber() >= 36) {
		leader->Study(S_TACTICS, 90);
		leader->Study(S_COMBAT, 60);
	}

	if (Globals->UPKEEP_MINIMUM_FOOD > 0)
	{
		if (!(ItemDefs[I_FOOD].flags & ItemType::DISABLED))
		{
			leader->items.SetNum(I_FOOD, 6);
			pFac->DiscoverItem(I_FOOD, 0, 1);
		}
		else if (!(ItemDefs[I_FISH].flags & ItemType::DISABLED))
		{
			leader->items.SetNum(I_FISH, 6);
			pFac->DiscoverItem(I_FISH, 0, 1);
		}
		else if (!(ItemDefs[I_LIVESTOCK].flags & ItemType::DISABLED))
		{
			leader->items.SetNum(I_LIVESTOCK, 6);
			pFac->DiscoverItem(I_LIVESTOCK, 0, 1);
		}
		else if (!(ItemDefs[I_GRAIN].flags & ItemType::DISABLED))
		{
			leader->items.SetNum(I_GRAIN, 2);
			pFac->DiscoverItem(I_GRAIN, 0, 1);
		}
		leader->items.SetNum(I_SILVER, 10);
		pFac->DiscoverItem(I_SILVER, 0, 1);
	}

	leader->MoveUnit( reg->GetDummy() );

    return( 1 );
}

Faction *Game::CheckVictory()
{
	ARegion *reg;
	forlist(&regions) {
		ARegion *r = (ARegion *)elem;
		forlist(&r->objects) {
			Object *obj = (Object *)elem;
			if(obj->type != O_BKEEP) continue;
			if(obj->units.Num()) return NULL;
			reg = r;
			break;
		}
	}
	{
		// Now see find the first faction guarding the region
		forlist(&reg->objects) {
			Object *o = reg->GetDummy();
			forlist(&o->units) {
				Unit *u = (Unit *)elem;
				if(u->guard == GUARD_GUARD) return u->faction;
			}
		}
	}
	return NULL;
}

void Game::ModifyTablesPerRuleset(void)
{
	if(Globals->APPRENTICES_EXIST)
	   	EnableSkill(S_MANIPULATE);

	if((Globals->UNDERDEEP_LEVELS > 0) || (Globals->UNDERWORLD_LEVELS > 1)) {
		EnableItem(I_MUSHROOM);
		EnableItem(I_HEALPOTION);
		EnableItem(I_ROUGHGEM);
		EnableItem(I_GEMS);
		EnableItem(I_MWOLF);
		EnableItem(I_MSPIDER);
		EnableItem(I_MOLE);
		EnableSkill(S_GEMCUTTING);
		EnableSkill(S_MONSTERTRAINING);
	}

	if(!Globals->GATES_EXIST)
		DisableSkill(S_GATE_LORE);

	if(Globals->NEXUS_IS_CITY) {
		ClearTerrainRaces(R_NEXUS);
		ModifyTerrainRace(R_NEXUS, 0, I_HIGHELF);

		ClearTerrainItems(R_NEXUS);
		ModifyTerrainItems(R_NEXUS, 0, I_HERBS, 100, 20);
		ModifyTerrainItems(R_NEXUS, 1, I_ROOTSTONE, 100, 10);
		ModifyTerrainItems(R_NEXUS, 2, I_MITHRIL, 100, 10);
		ModifyTerrainItems(R_NEXUS, 3, I_YEW, 100, 10);
		ModifyTerrainItems(R_NEXUS, 4, I_IRONWOOD, 100, 10);
		ModifyTerrainEconomy(R_NEXUS, 1000, 15, 50, 2);
	}

	// Races
	EnableItem(I_HIGHELF);
	EnableItem(I_WOODELF);
	EnableItem(I_HILLDWARF);
	EnableItem(I_HALFLING);
	EnableItem(I_CENTAURMAN);
	EnableItem(I_MOUNTAINDWARF);
	EnableItem(I_GNOME);
	EnableItem(I_GREYELF);
	EnableItem(I_HUMAN);
	EnableItem(I_LIZARDMAN);
	//EnableItem(I_HALFORC);
	EnableItem(I_MINOTAUR);
	EnableItem(I_ORC);
	EnableItem(I_HOBGOBLIN);
	EnableItem(I_GNOLL);
	EnableItem(I_OGREMAN);
	EnableItem(I_GOBLINMAN);
	EnableItem(I_UNDERDWARF);
	EnableItem(I_DARKELF);
	EnableItem(I_KOBOLDMAN);

	DisableItem(I_VIKING);
	DisableItem(I_BARBARIAN);
	DisableItem(I_PLAINSMAN);
	DisableItem(I_NORTHLING);
	DisableItem(I_NOMAD);
	DisableItem(I_TRIBESMAN);
	DisableItem(I_DARKLING);
	DisableItem(I_SEAELF);
	DisableItem(I_TRIBALELF);
	DisableItem(I_ICEDWARF);
	DisableItem(I_DESERTDWARF);
	DisableItem(I_MAN);
	DisableItem(I_FAIRY);
	DisableItem(I_URUK);
	DisableItem(I_DROWMAN);
	DisableItem(I_MERC);
	DisableItem(I_TITAN);
	DisableItem(I_AMAZON);
	DisableItem(I_MOUNTAINMAN);

	// Weapons
	// Iron
	EnableItem(I_PICK);
	EnableItem(I_DAGGER);
	EnableItem(I_SPEAR);
	EnableItem(I_AXE);
	EnableItem(I_HAMMER);
	EnableItem(I_MACE);
	EnableItem(I_SWORD);
	EnableItem(I_2HSWORD);
	EnableItem(I_HALBERD);
	EnableItem(I_MAUL);
	// Mithril
	EnableItem(I_MDAGGER);
	EnableItem(I_MSPEAR);
	EnableItem(I_MAXE);
	EnableItem(I_MHAMMER);
	EnableItem(I_MMACE);
	EnableItem(I_MSWORD);
	EnableItem(I_M2HSWORD);
	EnableItem(I_MHALBERD);
	EnableItem(I_MMAUL);
	// Admantium
	EnableItem(I_ADAGGER);
	EnableItem(I_ASPEAR);
	EnableItem(I_AAXE);
	EnableItem(I_AHAMMER);
	EnableItem(I_AMACE);
	EnableItem(I_ASWORD);
	EnableItem(I_A2HSWORD);
	EnableItem(I_AHALBERD);
	EnableItem(I_AMAUL);
	// Ranged
	EnableItem(I_SHORTBOW);
	EnableItem(I_BOW);
	EnableItem(I_LONGBOW);
	EnableItem(I_CROSSBOW);
	EnableItem(I_HEAVYCROSSBOW);

	// Enchanted
	EnableItem(I_EPICK);
	EnableItem(I_EDAGGER);
	EnableItem(I_ESPEAR);
	EnableItem(I_EAXE);
	EnableItem(I_EHAMMER);
	EnableItem(I_EMACE);
	EnableItem(I_ESWORD);
	EnableItem(I_E2HSWORD);
	EnableItem(I_EHALBERD);
	EnableItem(I_EMAUL);
	EnableItem(I_EMDAGGER);
	EnableItem(I_EMSPEAR);
	EnableItem(I_EMAXE);
	EnableItem(I_EMHAMMER);
	EnableItem(I_EMMACE);
	EnableItem(I_EMSWORD);
	EnableItem(I_EM2HSWORD);
	EnableItem(I_EMHALBERD);
	EnableItem(I_EMMAUL);
	EnableItem(I_EADAGGER);
	EnableItem(I_EASPEAR);
	EnableItem(I_EAAXE);
	EnableItem(I_EAHAMMER);
	EnableItem(I_EAMACE);
	EnableItem(I_EASWORD);
	EnableItem(I_EA2HSWORD);
	EnableItem(I_EAHALBERD);
	EnableItem(I_EAMAUL);
	EnableItem(I_ESHORTBOW);
	EnableItem(I_EBOW);
	EnableItem(I_ELONGBOW);
	EnableItem(I_ECROSSBOW);
	EnableItem(I_EHEAVYCROSSBOW);

	// Flaming Sword
	EnableItem(I_FSWORD);
	EnableSkill(S_CREATE_FLAMING_SWORD);

	// Special Leader Items
	EnableItem(I_WIZARDSTAFF);
	EnableItem(I_WIZROBE);

	// Armor
	EnableItem(I_PADDEDARMOR);
	EnableItem(I_LEATHERARMOR);
	// Iron
	EnableItem(I_SCALEARMOR);
	EnableItem(I_MAILARMOR);
	EnableItem(I_PLATEMAILARMOR);
	EnableItem(I_PLATEARMOR);
	// Mithril
	EnableItem(I_MSCALEARMOR);
	EnableItem(I_MMAILARMOR);
	EnableItem(I_MPLATEMAILARMOR);
	EnableItem(I_MPLATEARMOR);
	// Admantium
	EnableItem(I_ASCALEARMOR);
	EnableItem(I_AMAILARMOR);
	EnableItem(I_APLATEMAILARMOR);
	EnableItem(I_APLATEARMOR);

	// Enchanted
	// Armor
	EnableItem(I_EPADDEDARMOR);
	EnableItem(I_ELEATHERARMOR);
	// Iron
	EnableItem(I_ESCALEARMOR);
	EnableItem(I_EMAILARMOR);
	EnableItem(I_EPLATEMAILARMOR);
	EnableItem(I_EPLATEARMOR);
	// Mithril
	EnableItem(I_EMSCALEARMOR);
	EnableItem(I_EMMAILARMOR);
	EnableItem(I_EMPLATEMAILARMOR);
	EnableItem(I_EMPLATEARMOR);
	// Admantium
	EnableItem(I_EASCALEARMOR);
	EnableItem(I_EAMAILARMOR);
	EnableItem(I_EAPLATEMAILARMOR);
	EnableItem(I_EAPLATEARMOR);

	DisableItem(I_CHAINARMOR);
	DisableItem(I_CLOTHARMOR);
	DisableItem(I_MPLATE);
	DisableItem(I_DOUBLEBOW);

	// Resources
	EnableItem(I_ADMANTIUM);

	// Trade Items
	DisableItem(I_VODKA);
	DisableItem(I_CAVIAR);
	DisableItem(I_ROSES);
	DisableItem(I_TAROTCARDS);
	DisableItem(I_CHOCOLATE);

	// Mounts
	EnableItem(I_MWOLF);
	EnableItem(I_MSPIDER);
	EnableItem(I_MOLE);
	EnableItem(I_CAMEL);

	// Misc Items
	EnableItem(I_HARP);
	EnableItem(I_MUSHROOM);
	EnableItem(I_HEALPOTION);
	EnableItem(I_GLIDER);
	EnableItem(I_NET);
	EnableItem(I_LASSO);
	EnableItem(I_BAG);
	EnableItem(I_SPINNING);
	//EnableItem(I_ROUGHGEM);
	//EnableItem(I_GEMS);

	// Skills
	EnableSkill(S_CAMELTRAINING);
	EnableSkill(S_MONSTERTRAINING);
	//EnableSkill(S_WEAPONCRAFT);
	//EnableSkill(S_ARMORCRAFT);
	//EnableSkill(S_GEMCUTTING);

	// Monsters
	EnableItem(I_DROW);
	EnableItem(I_HYDRA);
	EnableItem(I_STORMGIANT);
	EnableItem(I_CLOUDGIANT);
	EnableItem(I_ILLYRTHID);
	EnableItem(I_SORCERERS);
	EnableItem(I_MAGICIANS);
	EnableItem(I_DARKMAGE);
	EnableItem(I_WARRIORS);
	EnableItem(I_ICEDRAGON);

	EnableItem(I_PIRATES);
	EnableItem(I_KRAKEN);
	EnableItem(I_MERFOLK);
	EnableItem(I_ELEMENTAL);

	// Lairs
	EnableObject(O_ISLE);
	EnableObject(O_DERELICT);
	EnableObject(O_OCAVE);
	EnableObject(O_WHIRL);

	EnableObject(O_ROADN);
	EnableObject(O_ROADNW);
	EnableObject(O_ROADNE);
	EnableObject(O_ROADSW);
	EnableObject(O_ROADSE);
	EnableObject(O_ROADS);
	EnableObject(O_TEMPLE);
	EnableObject(O_MQUARRY);
	EnableObject(O_AMINE);
	EnableObject(O_PRESERVE);
	EnableObject(O_SACGROVE);

	EnableObject(O_DCLIFFS);
	EnableObject(O_MTOWER);
	EnableObject(O_WGALLEON);
	EnableObject(O_HUT);
	EnableObject(O_STOCKADE);
	EnableObject(O_CPALACE);
	EnableObject(O_NGUILD);
	EnableObject(O_AGUILD);
	EnableObject(O_ATEMPLE);
	EnableObject(O_HTOWER);
	EnableObject(O_HPTOWER);

	EnableObject(O_MAGETOWER);
	EnableObject(O_DARKTOWER);
	EnableObject(O_GIANTCASTLE);
	EnableObject(O_ILAIR);
	EnableObject(O_ICECAVE);
	EnableObject(O_BOG);

	EnableObject(O_TRAPPINGHUT);
	EnableObject(O_STABLE);
	EnableObject(O_MSTABLE);
	EnableObject(O_TRAPPINGLODGE);
	EnableObject(O_FAERIERING);
	EnableObject(O_ALCHEMISTLAB);
	EnableObject(O_OASIS);
	EnableObject(O_GEMAPPRAISER);

	// OCEAN
	// no changes

	// PLAIN
	ClearTerrainRaces(R_PLAIN);
	ModifyTerrainRace(R_PLAIN, 0, I_HUMAN);
	ModifyTerrainRace(R_PLAIN, 1, I_HIGHELF);
	ModifyTerrainRace(R_PLAIN, 2, I_HALFLING);
	ModifyTerrainRace(R_PLAIN, 3, I_CENTAURMAN);

	ModifyTerrainItems(R_PLAIN, 1, I_WHORSE, 5, 5);
	// ModifyTerrainLairChance(R_PLAIN, 5);

	// FOREST
	ClearTerrainRaces(R_FOREST);
	ModifyTerrainRace(R_FOREST, 0, I_WOODELF);
	ModifyTerrainCoastRace(R_FOREST, 0, I_WOODELF);
	ModifyTerrainCoastRace(R_FOREST, 1, I_HIGHELF);

	ModifyTerrainItems(R_FOREST, 5, I_MWOLF, 5, 5);

	// MYSTFOREST
	// Not in Use

	// MOUNTAIN
	ClearTerrainRaces(R_MOUNTAIN);
	ModifyTerrainRace(R_MOUNTAIN, 0, I_MOUNTAINDWARF);
	ModifyTerrainRace(R_MOUNTAIN, 1, I_ORC);

	ModifyTerrainItems(R_MOUNTAIN, 5, I_MWOLF, 5, 5);

	// HILL
	// No changes needed

	// SWAMP
	ClearTerrainRaces(R_SWAMP);
	ModifyTerrainRace(R_SWAMP, 0, I_LIZARDMAN);
	ModifyTerrainRace(R_SWAMP, 1, I_ORC);
	ModifyTerrainCoastRace(R_SWAMP, 0, I_LIZARDMAN);
	ModifyTerrainCoastRace(R_SWAMP, 1, I_GOBLINMAN);
	ModifyTerrainCoastRace(R_SWAMP, 1, I_HUMAN);

	// JUNGLE
	ClearTerrainRaces(R_SWAMP);
	ModifyTerrainRace(R_SWAMP, 0, I_LIZARDMAN);
	ModifyTerrainRace(R_SWAMP, 1, I_WOODELF);
	ModifyTerrainCoastRace(R_SWAMP, 0, I_HUMAN);
	ModifyTerrainCoastRace(R_SWAMP, 1, I_GOBLINMAN);

	// DESERT
	ClearTerrainRaces(R_DESERT);
	ModifyTerrainRace(R_DESERT, 0, I_HUMAN);
	ModifyTerrainRace(R_DESERT, 1, I_GNOLL);

	// TUNDRA
	ClearTerrainRaces(R_TUNDRA);
	ModifyTerrainRace(R_TUNDRA, 0, I_HUMAN);
	ModifyTerrainRace(R_TUNDRA, 1, I_GNOLL);

	ModifyTerrainItems(R_TUNDRA, 1, I_MWOLF, 10, 15);

	// Using only R_TUNDRA and R_CERAN_TUNDRA1

	// CAVERNS
	ClearTerrainRaces(R_CAVERN);
	ModifyTerrainRace(R_CAVERN, 0, I_UNDERDWARF);
	ModifyTerrainRace(R_CAVERN, 1, I_KOBOLDMAN);
	ModifyTerrainRace(R_CAVERN, 2, I_DARKELF);

	// UNDERFOREST
	ClearTerrainRaces(R_UFOREST);
	ModifyTerrainRace(R_UFOREST, 0, I_GREYELF);
	ModifyTerrainRace(R_UFOREST, 1, I_DARKELF);

	// TUNNELS
	// No changes

	// GROTTO
	// No changes

	// DEEPFOREST
	// No changes

	// CHASM
	// No changes

	ModifyObjectFlags(O_BKEEP, ObjectType::NEVERDECAY);
	ModifyObjectFlags(O_DCLIFFS, ObjectType::CANENTER|ObjectType::NEVERDECAY);
	ModifyObjectConstruction(O_DCLIFFS, I_ROOTSTONE, 50, S_DRAGON_LORE, 3);

	// Make GateLore, ConstructGate and PortalLore take twice as long to study.
	// ModifySkillFlags(S_GATE_LORE,
	// 		SkillType::MAGIC | SkillType::CAST | SkillType::SLOWSTUDY);
	// ModifySkillFlags(S_CONSTRUCT_GATE,
	// 		SkillType::MAGIC | SkillType::CAST | SkillType::SLOWSTUDY);
	// ModifySkillFlags(S_PORTAL_LORE,
	// 		SkillType::MAGIC | SkillType::CAST | SkillType::SLOWSTUDY);

	// Modify the various spells which are allowed to cross levels
	if(Globals->EASIER_UNDERWORLD) {
		ModifyRangeFlags(RANGE_TELEPORT, RangeType::RNG_CROSS_LEVELS);
		ModifyRangeFlags(RANGE_FARSIGHT, RangeType::RNG_CROSS_LEVELS);
        ModifyRangeFlags(RANGE_CLEAR_SKIES, RangeType::RNG_CROSS_LEVELS);
        ModifyRangeFlags(RANGE_WEATHER_LORE, RangeType::RNG_CROSS_LEVELS);
	}

	return;
}

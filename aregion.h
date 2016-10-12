#ifndef REGION_CLASS
#define REGION_CLASS
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
#include "production.h"
#include "market.h"
#include "gamedefs.h"
#include "alist.h"
class Areport;
class Faction;
class Object;
class Unit;
class UnitId;

// defined locally
class ARegion;
class ARegionList;
class ARegionArray;

//----------------------------------------------------------------------------
/// Weather Types
enum
{
	W_NORMAL,
	W_WINTER,
	W_MONSOON,
	W_BLIZZARD,
	W_MAX
};

///@return text representation of weather
const char* weatherString(int w);

//----------------------------------------------------------------------------
/// Raw materials produced in a region
struct Product
{
	int product;
	int chance;
	int amount;
};

//----------------------------------------------------------------------------
/// Terrain in a region
class TerrainType
{
public:
	const char *name;
	int similar_type;

	enum
	{
		RIDINGMOUNTS = 0x1,
		FLYINGMOUNTS = 0x2,
	};
	int flags;

	int pop;
	int wages;
	int economy;
	int movepoints;
	Product prods[7];

	// Race information
	// A hex near water will have either one of the normal races or one
	// of the coastal races in it.   Non-coastal hexes will only have one
	// of the normal races.
	int races[4];
	int coastal_races[3];

	int wmonfreq;
	int smallmon;
	int bigmon;
	int humanoid;
	int lairChance;
	int lairs[6];
};

extern TerrainType *TerrainDefs;

//----------------------------------------------------------------------------
class Location : public AListElem
{
public:
	Unit *unit;
	Object *obj;
	ARegion *region;
};

///@return the location in 'location_list' containing 'unit_num' or NULL
Location* GetUnit(AList *location_list, int unit_num);

//----------------------------------------------------------------------------
class ARegionPtr : public AListElem
{
public:
	ARegionPtr(ARegion *p = NULL)
	: ptr(p)
	{
	}

	ARegion *ptr;
};

///@return region_num in region_list, or NULL
ARegionPtr* GetRegion(AList *region_list, int region_num);

//----------------------------------------------------------------------------
class Farsight : public AListElem
{
public:
	Farsight();

	Faction *faction;
	Unit *unit;
	int level;
	int exits_used[NDIRS];
};

///@return the farsight report for 'fac', or NULL
Farsight* GetFarsight(AList *farsight_list, Faction *fac);

//----------------------------------------------------------------------------
enum
{
	TOWN_VILLAGE,
	TOWN_TOWN,
	TOWN_CITY,
	NTOWNS
};

class TownInfo
{
public:
	TownInfo();
	~TownInfo();

	void Readin(Ainfile *f, ATL_VER &v);
	void Writeout(Aoutfile *f) const;

	int TownType() const;

	AString *name;
	int pop;
	int basepop;
	int activity;
};

//----------------------------------------------------------------------------
class ARegion : public AListElem
{
friend class Game;
friend class ARegionArray;

public:
	/// constructor
	ARegion();

	/// destructor
	~ARegion();

	void Setup();

	void Writeout(Aoutfile *f);
	void Readin(Ainfile *f, AList *facs, ATL_VER v);

	void WriteReport(Areport *f, Faction *fac, int month, ARegionList *pRegions);
	void WriteTemplate(Areport *f, Faction *fac, ARegionList *pRegs, int month);

	// defined in template.cpp
	void WriteTemplateHeader(Areport *f, Faction *fac, ARegionList *pRegs, int month);
	void GetMapLine(char *buffer, int line, ARegionList *pRegs);

	AString ShortPrint(ARegionList *pRegs);
	AString Print(ARegionList *pRegs);

	/// change the name of the region
	void SetName(const char *new_name);

	///@return 1 if 'fac' can make 'item'
	int CanMakeAdv(Faction *fac, int item);

	///@return 1 if a unit from 'fac' has 'item' in this
	int HasItem(Faction *fac, int item);

	/// take the items from 'u' and give them to a friend, then move 'u' to "Hell"
	void Kill(Unit *u);

	/// delete all the units that have died
	void ClearHell();

	///@return unit matching 'num', or NULL
	Unit* GetUnit(int num);

	///@return unit matching 'alias' for 'faction', or NULL
	Unit* GetUnitAlias(int alias, int faction);

	///@return unit matching 'id' for 'faction'
	Unit* GetUnitId(UnitId *id, int faction);

	///@return new Location wrapping id, faction, this
	Location* GetLocation(UnitId *id, int faction);

	///@return 1 if a unit from 'f' is in this, else 0
	int Present(Faction *f);

	///@return new list of FactionPtr containing all the factions represented here
	AList* PresentFactions();

	int GetObservation(Faction *f, int use_passers);
	int GetTrueSight(Faction *f, int use_passers);

	///@return object 'num', or NULL
	Object* GetObject(int num);

	///@return the object representing open space
	Object* GetDummy();

	///@param[out] road message which is updated for road usage
	///@return the cost to move into this region 'from_region' along 'dir' via 'movetype'
	int MoveCost(int movetype, ARegion *from_region, int dir, AString *road);

	///@return unit that is forbidding 'u' from entering this
	Unit* Forbidden(Unit *u);

	///@return allied unit that is forbidding 'u' from entering this
	Unit* ForbiddenByAlly(Unit *u);

	///@return 1 if 'u' can tax here
	int CanTax(Unit *u);

	///@return 1 if 'u' can pillage here
	int CanPillage(Unit *u);

	///@return 1 if any unit in 'ship' is forbidden from entering here
	int ForbiddenShip(Object *ship);

	///@return 1 if city guard present here
	int HasCityGuard();

	/// post a report to all the units here
	void NotifySpell(Unit *caster, int spell, ARegionList *pRegs);

	/// notify all units in city about city name change
	void NotifyCity(Unit *namer, const AString &oldname, const AString &newname);

	/// reset orders for all units here
	void DefaultOrders();

	void PostTurn(ARegionList *pRegs);
	void UpdateProducts();
	void SetWeather(int newWeather);

	///@return 1 for ocean-like (not lake) or number of adjacent ocean-like
	int IsCoastal();

	int IsCoastalOrLakeside();
	void MakeStartingCity();
	int IsStartingCity();
	int IsSafeRegion();
	int CanBeStartingCity(ARegionArray *pRA);
	int HasShaft();

	///@return 1 if this region has a road in any direction, else 0
	///@NOTE: the destination region must have a corresponding road to be useful
	int HasRoad();

	///@return 1 if this region has a road in the specified direction
	int HasExitRoad(int realDirection);

	int CountConnectingRoads();
	int HasConnectingRoad(int realDirection);

	///@return the road direction corresponding to 'realDirection'
	int GetRoadDirection(int realDirection);

	int GetRealDirComp(int realDirection) const;
	void DoDecayCheck(ARegionList *pRegs);
	void DoDecayClicks(Object *o, ARegionList *pRegs);
	void RunDecayEvent(Object *o, ARegionList *pRegs);
	int PillageCheck() const;

	int GetPoleDistance(int dir);

	int CountWMons();
	int IsGuarded();

	int Wages();
	AString WagesForReport();
	int Population() const;

	// used by ARegionArray
	void ZeroNeighbors();
	void SetLoc(int x, int y, int z);

private: // methods
	void SetupPop();
	void SetupProds();
	int GetNearestProd(int item);
	void SetupCityMarket();
	void AddTown();
	void MakeLair(int lair_type);
	void LairCheck();

	void WriteProducts(Areport *f, Faction *fac, int present);
	void WriteMarkets(Areport *f, Faction *fac, int present);
	void WriteEconomy(Areport *f, Faction *fac, int present);
	void WriteExits(Areport *f, ARegionList *pRegs, bool *exits_seen) const;

	int GetMaxClicks() const;
	AString GetDecayFlavor() const;

	/// apply variable economy effects
	void UpdateTown();

public: // data
	AString *name;
	int num;
	int type;
	int buildingseq;
	int weather;
	int gate;

	TownInfo *town;
	int race;
	int population;
	int basepopulation;
	int wages;
	int maxwages;
	int money;

	// Potential bonuses to economy
	int clearskies;
	int earthlore;

	ARegion *neighbors[NDIRS];
	AList objects;
	AList hell; // Where dead units go
	AList farsees;

	// List of units which passed through the region
	AList passers;

	ProductionList products;
	MarketList markets;
	int xloc, yloc, zloc;
};

//----------------------------------------------------------------------------
class ARegionArray
{
public:
	ARegionArray(int x, int y);
	~ARegionArray();

	void SetRegion(int x, int y, ARegion *r);
	ARegion* GetRegion(int x, int y);
	void SetName(const char *name);

public: // data
	int x;
	int y;
	ARegion **regions;
	AString *strName;

	enum
	{
		LEVEL_NEXUS,
		LEVEL_SURFACE,
		LEVEL_UNDERWORLD,
		LEVEL_UNDERDEEP,
	};
	int levelType;
};

//----------------------------------------------------------------------------
class ARegionList : public AList
{
public:
	ARegionList();
	~ARegionList();

	void WriteRegions(Aoutfile *f);
	int ReadRegions(Ainfile *f, AList *factions, ATL_VER v);

	ARegion* GetRegion(int region_num);
	ARegion* GetRegion(int x, int y, int z);
	Location* FindUnit(int unit_num);

	void ChangeStartingCity(ARegion *, int);
	ARegion* GetStartingCity(ARegion *AC, int num, int level, int maxX, int maxY);

	ARegion* FindGate(int gate_id);
	int GetDistance(ARegion *one, ARegion *two);
	int GetPlanarDistance(ARegion *one, ARegion *two, int penalty);
	int GetWeather(ARegion *pReg, int month);

	ARegionArray* GetRegionArray(int level);

public: // data
	int numberofgates;
	int numLevels;
	ARegionArray **pRegionArrays;

public: // Public world creation stuff
	void CreateLevels(int numLevels);
	void CreateAbyssLevel(int level, const char *name);
	void CreateNexusLevel(int level, int xSize, int ySize, const char *name);
	void CreateSurfaceLevel(int level, int xSize, int ySize,
	    int percentOcean, int continentSize, const char *name);
	void CreateIslandLevel(int level, int nPlayers, const char *name);
	void CreateUnderworldLevel(int level, int xSize, int ySize, const char *name);
	void CreateUnderdeepLevel(int level, int xSize, int ySize, const char *name);

	void MakeShaftLinks(int levelFrom, int levelTo, int odds);
	void SetACNeighbors(int levelSrc, int levelTo, int maxX, int maxY);
	void InitSetupGates(int level);
	void FinalSetupGates();

	void CleanUpWater(ARegionArray *pRegs);
	void RemoveCoastalLakes(ARegionArray *pRegs);
	void SeverLandBridges(ARegionArray *pRegs);

	void CalcDensities();
	int GetLevelXScale(int level);
	int GetLevelYScale(int level);

private: // Private world creation stuff
	void MakeRegions(int level, int xSize, int ySize);
	void SetupNeighbors(ARegionArray *pRegs);
	void NeighSetup(ARegion *r, ARegionArray *ar);

	void SetRegTypes(ARegionArray *pRegs, int newType);
	void MakeLand(ARegionArray *pRegs, int percentOcean, int continentSize);
	void MakeCentralLand(ARegionArray *pRegs);

	void SetupAnchors(ARegionArray *pArr);
	void GrowTerrain(ARegionArray *pArr, int growOcean);
	void RandomTerrain(ARegionArray *pArr);
	void MakeUWMaze(ARegionArray *pArr);
	void MakeIslands(ARegionArray *pArr, int nPlayers);
	void MakeOneIsland(ARegionArray *pRegs, int xx, int yy);

	void AssignTypes(ARegionArray *pArr);
	void FinalSetup(ARegionArray *pArr);

	void MakeShaft(ARegion *reg, ARegionArray *pFrom, ARegionArray *pTo);

	// Game-specific world stuff (see world.cpp)
	int GetRegType(ARegion *pReg);
	int CheckRegionExit(ARegion *pFrom, ARegion *pTo);
};

//----------------------------------------------------------------------------
///@return a random name (defined in specific world.cpp)
int AGetName(int town);
const char* AGetNameString(int name);

#endif


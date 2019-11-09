#ifndef FACTION_CLASS
#define FACTION_CLASS
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
#include "items.h"
#include "skills.h"
#include "helper.h"
#include "alist.h"
#include <list>
#include <vector>

class Ainfile;
class Aoutfile;
class ARegion;
class ARegionList;
class Areport;
class AString;
class Faction;
class Game;
class Unit;

/// Relations between factions
enum Attitudes
{
	A_HOSTILE,
	A_UNFRIENDLY,
	A_NEUTRAL,
	A_FRIENDLY,
	A_ALLY,
	NATTITUDES
};
extern const char* *AttitudeStrs; ///< string for enum
int ParseAttitude(AString *); ///< enum from string

/// Places to spend faction points
enum FactionPointTypes
{
	F_WAR,
	F_TRADE,
	F_MAGIC,
	NFACTYPES
};
extern const char* *FactionStrs; ///< string for enum

/// Player report format
enum ReportFormat
{
	TEMPLATE_OFF,
	TEMPLATE_SHORT,
	TEMPLATE_LONG,
	TEMPLATE_MAP
};

/// Ways to quit
enum QuitCommand
{
	QUIT_NONE,
	QUIT_BY_ORDER,
	QUIT_BY_GM,
	QUIT_AND_RESTART,
	QUIT_WON_GAME,
	QUIT_GAME_OVER,
};

/// a list of factions
class FactionVector
{
public:
	explicit FactionVector(int);
	~FactionVector();

	void ClearVector();
	void SetFaction(int, Faction *);
	Faction* GetFaction(int);

public: // data
	Faction **vector;
	int vectorsize;
};

/// wrapper of faction
class FactionPtr : public AListElem
{
public:
	Faction *ptr;
};

/// toplevel container for one player to control
class Faction : public AListElem
{
public:
	 /// Track good and evil
	 enum Alignments
	 {
		 ALL_NEUTRAL,
		 SOME_EVIL,
		 SOME_GOOD,
		 ALIGN_ERROR
	 };

public:
	/// simplest constructor
	explicit Faction(Game &g);

	/// construct with faction number
	Faction(Game &g, int);

	/// destructor
	~Faction();

	/// read from game file
	void Readin(Ainfile *, ATL_VER version);

	/// write to game file
	void Writeout(Aoutfile *);

	/// write short faction string
	void View();

	void SetName(AString *);
	void SetNameNoChange(AString *str);
	void SetAddress(const AString &strNewAddress);

	void CheckExist(ARegionList *);
	void Error(const AString &);
	void Event(const AString &);

	AString FactionTypeStr();
	void WriteReport(Areport *f, Game *pGame);
	void WriteFacInfo(Aoutfile *);

	void SetAttitude(int num, int attitude); // if attitude == -1, clear it
	int GetAttitude(int);
	void RemoveAttitude(int);

	int CanCatch(ARegion *, Unit *);

	// Return 1 if can see, 2 if can see faction
	int CanSee(ARegion *, Unit *, int practise = 0);

	void DefaultOrders();
	void TimesReward();

	void SetNPC();
	int IsNPC();

	void DiscoverItem(int item, int force, int full);

public: // data
	Game &game_; ///< back pointer to game
	int num;

	int type[NFACTYPES]; ///< current placement of faction points

	int lastchange;
	int lastorders;
	int unclaimed;
	AString *name;
	AString *address;
	AString *password;
	int times;
	int temformat;
	char exists;
	int quit;
	int numshows;

	int nummages;
	int numapprentices;
	AList war_regions;
	AList trade_regions;

	// Used when writing reports
	AList present_regions;

	int defaultattitude;
	AList attitudes;
	SkillList skills;
	ItemList items;
	
	std::list<AString*> extraPlayers_;
	std::vector<AString*> errors_;
	AList events;
	AList battles;
	AList shows;
	AList itemshows;
	AList objectshows;

	// used for 'granting' units to a faction via the players.in file
	ARegion *pReg;
	int noStartLeader;
	Alignments alignments_;
};

Faction* GetFaction(AList *fac_list, int fac_num);
Faction* GetFaction2(AList *fac_ptr_list, int fac_num); // This AList is a list of FactionPtr

#endif

#ifndef OBJECT_CLASS
#define OBJECT_CLASS
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
#include "alist.h"
#include "helper.h"
class Ainfile;
class Aoutfile;
class ARegion;
class Areport;
class AString;
class Faction;
class Unit;
class UnitId;

//----------------------------------------------------------------------------
#define I_WOOD_OR_STONE -2

/// Global definition of an in-game object.
/// Objects reside in regions
class ObjectType
{
public:
	const char *name; ///< name of the object

	enum
	{
		DISABLED        = 0x01, ///< not available in this game
		NOMONSTERGROWTH = 0x02,
		NEVERDECAY      = 0x04, ///< not subject to decay
		CANENTER        = 0x08,
		CANMODIFY       = 0x20
	};
	int flags; ///< combination of above

	int protect;  ///< adds to the defense of units inside it
	int capacity; ///< denotes a ship, which has some carrying capacity
	int sailors;  ///< amount of sailing skill required to run a ship
	int maxMages; ///< maximum number of mages who can study in the structure

	int item;  ///< index of input item required for construction of this
	int cost;  ///< number of the items required
	int skill; ///< index of the skill required for construction of this
	int level; ///< level of the skill required for construction of this

	int maxMaintenance; ///< max damage before decay sets in
	int maxMonthlyDecay; ///< decay rate
	int maintFactor; ///< number of input items required to repair

	int monster; ///< can monsters lair here (-1 for no)

	int productionAided; ///< item index that receives a production boost from this
};
extern ObjectType *const ObjectDefs; ///< global table of object definitions

///@return a new string with a description of the item type 'obj'
AString* ObjectDescription(int obj);

///@return object index for 'token' (-1 if not found or disabled)
int ParseObject(AString *token);

///@return 1 if object has capacity, else 0
int ObjectIsShip(int);

/// Instance of an object in a region
class Object : public AListElem
{
public:
	/// constructor
	Object(ARegion *region);
	/// destructor
	~Object();

	/// read from 'f'
	void Readin(Ainfile *f, AList *factions, ATL_VER ver);

	/// write to 'f'
	void Writeout(Aoutfile *f);

	/// write report to 'f'
	void Report(Areport *f, Faction *fac, int obs, int truesight, int detfac, int passobs, int passtrue, int passdetfac, int present);

	/// change name to 's', takes ownership
	void SetName(AString *s);

	/// change user description to 's', takes ownership
	void SetDescribe(AString *s);

	///@return the unit matching 'num'
	Unit* GetUnit(int num);

	///@return unit matching 'alias' with formfaction 'faction' (or faction)
	Unit* GetUnitAlias(int alias, int faction);

	///@return unit according to 'id'
	Unit* GetUnitId(const UnitId *id, int faction);

	///@return 1 if this is a road, else 0
	int IsRoad();

	///@return 1 if this has some capacity
	int IsBoat();

	///@return 1 if this can protect units
	int IsBuilding();

	///@return 1 if the object can be modified (have its name and description changed)
	int CanModify();

	///@return 1 if 'u' can enter this
	int CanEnter(ARegion *reg, Unit *u);

	///@return the unit forbidding entry to 'u'
	Unit* ForbiddenBy(ARegion *reg, Unit *u);

	///@return the first populated unit in this
	Unit* GetOwner();

	/// set the previous direction
	void SetPrevDir(int newdir);

	/// move object to 'toreg'
	void MoveObject(ARegion *toreg);

public: // data
	AString *name; ///< type name
	AString *describe; ///< user description
	ARegion *region; ///< region location
	int inner; ///< contains inner location
	int num; ///< quantity
	int type; ///< index into global table
	int incomplete; ///< still under construction
	int capacity; ///< carrying capacity (on water)
	int runes; ///< (bool?) has been protected with runes
	int prevdir; ///< previous direction (trivial portage?)
	int mages; ///< maximum number of mages that can be inside
	AList units; ///< units inside
};

#endif


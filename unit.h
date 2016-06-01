#ifndef UNIT_CLASS
#define UNIT_CLASS
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
#include "skills.h"
#include "items.h"
#include "helper.h"
class Ainfile;
class Aoutfile;
class ARegion;
class Areport;
class AttackOrder;
class CastOrder;
class EvictOrder;
class Object;
class Order;
class TeleportOrder;

//----------------------------------------------------------------------------
/// Ways a unit can be on guard
enum
{
	GUARD_NONE,
	GUARD_GUARD,
	GUARD_AVOID,
	GUARD_SET,
	GUARD_ADVANCE
};

/// Ways a unit can tax/pillage
enum
{
	TAX_NONE,
	TAX_TAX,
	TAX_PILLAGE,
	TAX_AUTO
};

/// Ways a unit can reveal itself
enum
{
	REVEAL_NONE,
	REVEAL_UNIT,
	REVEAL_FACTION
};

/// Types of units
enum
{
	U_NORMAL,
	U_MAGE,
	U_GUARD,
	U_WMON,
	U_GUARDMAGE,
	U_APPRENTICE,
	NUNITTYPES
};

/// maximum number of ready weapons or armors
enum { MAX_READY = 4 };

/// Unit flags (set of standing orders)
enum
{
	FLAG_BEHIND            = 0x0001, ///< try to hide behind friendly units
	FLAG_NOCROSS_WATER     = 0x0002, ///< don't obey orders that lead to crossing water
	FLAG_AUTOTAX           = 0x0004, ///< try to exhort taxes from the population
	FLAG_HOLDING           = 0x0008, ///< do not go to aid neighboring hexes in battle
	FLAG_NOAID             = 0x0010, ///< do not ask for aid in battle
	FLAG_INVIS             = 0x0020, ///< maintain invisibility?
	FLAG_CONSUMING_UNIT    = 0x0040, ///< consume food items in inventory
	FLAG_CONSUMING_FACTION = 0x0080, ///< consume any food from friendly units
	FLAG_NOSPOILS          = 0x0100, ///< do not take spoils
	FLAG_FLYSPOILS         = 0x0200, ///< do not take spoils that would prevent flying
	FLAG_WALKSPOILS        = 0x0400, ///< do not take spoils that would prevent walking
	FLAG_RIDESPOILS        = 0x0800  ///< do not take spoils that would prevent riding
};

//----------------------------------------------------------------------------
/// Handle on a unit, including new units ('alias')
class UnitId : public AListElem
{
public:
	UnitId();
	~UnitId();

	AString Print() const;

	int unitnum; ///< 0 -> new unit
	int alias;   ///< new unit id
	int faction; ///< owner id
};

//----------------------------------------------------------------------------
/// One unit in the game
class Unit : public AListElem
{
public:
	/// constructor
	Unit(int seq = 0, Faction *f = NULL, int alias = 0);

	/// destructor
	~Unit();

	/// set flags to make unit into monsters
	void SetMonFlags();
	/// Add 'num' of 'mon_id' and set flags for monsters
	void MakeWMon(const char *mon_name, int mon_id, int num);

	/// write to 'f'
	void Writeout(Aoutfile *f);

	/// read from 'f'
	void Readin(Ainfile *f, AList *facs, ATL_VER v);

	/// write appropriate details to 'f'
	void WriteReport(Areport *f, int obs, int truesight, int detfac, int autosee);

	///@return string with name, and faction if visible
	AString GetName(int obs);

	///@return extra string for mages
	AString MageReport() const;

	///@return string describing ready items
	AString ReadyItem() const;

	///@return string of all skills that can be studied
	AString StudyableSkills();

	///@return 1 if 'i' can be added to spoils
	int CanGetSpoil(const Item *i);

	///@return new string with combat details
	AString* BattleReport(int obs);

	AString TemplateReport();

	/// clear all the values related to monthly orders
	void ClearOrders();

	/// clear cast order and teleport order
	void ClearCastOrders();

	/// clear orders and populate with defaults
	void DefaultOrders(Object *obj);

	/// change name to 's', delete s
	void SetName(AString *s);

	/// change describe to 's', delete s
	void SetDescribe(AString *s);

	/// post turn processing
	void PostTurn(ARegion *reg);

	///@return 1 if unit has leaders, else 0
	int IsLeader();

	///@return 1 if unit has men who are not leaders
	int IsNormal();

	///@return number of monsters in unit
	int GetMons();

	///@return number of men in unit
	int GetMen();

	///@return number of items of type 't'
	int GetMen(int t);

	///@return number of soldiers in unit
	int GetSoldiers();

	/// set the number of men of 'type' to 'num' (assumes reduction)
	void SetMen(int type, int num);

	///@return 1 if this has some fight in it
	int IsAlive();

	///@return amount of silver
	int GetMoney();

	/// set silver to 'n'
	void SetMoney(int n);

	///@return maintenance cost for this
	int MaintCost();

	/// apply effects of hunger on this
	void Short(int needed, int hunger);

	///@return total number of skill levels known
	int SkillLevels();

	///@return the skill with type 'sk'
	Skill* GetSkillObject(int sk);

	///@return effective stealth for this
	int GetStealth();

	///@return effective tactics for this
	int GetTactics();

	///@return effective observation for this
	int GetObservation();

	///@return effective entertainment for this
	int GetEntertainment();

	///@return effective riding attack bonus for this
	int GetAttackRiding();

	///@return effective riding defense bonus for this
	int GetDefenseRiding();

	///@return bonus to 'sk'
	int GetSkillBonus(int sk);

	///@return bonus when producing 'item'
	int GetProductionBonus(int item);

	/// wrapper around various get skill level functions
	int GetSkill(int sk);

	/// set skill 'sk' to 'level'
	void SetSkill(int sk, int level);

	///@return skill level of 'sk'
	int GetRealSkill(int sk);

	/// remove 'sk' from known skills (also reverts type as appopriate)
	void ForgetSkill(int sk);

	///@return 1 if dependencies for 's' are satisfied, else 0
	int CheckDepend(int sk, const SkillDepend &s);

	///@return 1 if 'sk' can be studied
	int CanStudy(int sk);

	/// increase 'sk' by 'days', @return 1 on success, else 0
	int Study(int sk, int days);

	/// increase 'sk' by the practice amount, @return 1 if done, else 0
	int Practise(int sk);

	/// adjust skills for maximums
	void AdjustSkills();

	///@return 1 if can see, 2 if can see faction (forwards to Faction::CanSee())
	int CanSee(ARegion *r, Unit *u, int practise = 0);

	/// forwards to Faction::CanCatch()
	int CanCatch(ARegion *r, Unit *u);

	///@return 1 if there are enough amulets of true sight to protect this from the invisibility of 'u'
	int AmtsPreventCrime(Unit *u);

	/// Get this unit's attitude toward the Unit parameter
	int GetAttitude(ARegion *,Unit *);

	///@return this highest hostility of all monsters in this
	int Hostile();

	///@return 1 if this forbids entry of 'u' to 'r'
	int Forbids(ARegion *r, Unit *u);

	///@return weight of all items
	int Weight();

	///@return carrying capacity for different movement modes
	int FlyingCapacity();
	int RidingCapacity();
	int SwimmingCapacity();
	int WalkingCapacity();

	///@return true if can move in mode for given weight (defaults to weight of items)
	int CanFly(int weight);
	int CanRide(int weight);
	int CanWalk(int weight);
	int CanFly();
	int CanSwim();
	int CanReallySwim();

	///@return highest move possible
	int MoveType();

	///@return movement points corresponding to MoveType()
	int CalcMovePoints();

	///@return 1 if there is really some movement (0 for exits)
	int CanMoveTo(ARegion *r1, ARegion *r2);

	///@return flags & mask
	int GetFlag(int mask) const;

	/// update flags according to 'mask' (clear or set using 'val')
	void SetFlag(int mask, int val);

	/// copy 'flags', 'guard' and 'reveal' flags from 'u'
	void CopyFlags(const Unit *u);

	///@return type of item to be used in battle (not a weapon), -1 for none
	int GetBattleItem(int index);

	///@return type of armor in use (modulo assassination), -1 for none
	int GetArmor(int index, int ass);

	///@param[out] bonus attack bonus provided
	///@return -1 if no mount, else item type
	int GetMount(int index, int canFly, int canRide, int &bonus);

	///@return skill level and bonuses for weapon at 'index'
	int GetWeapon(int index, int riding, int ridingBonus,
	              int &attackBonus, int &defenseBonus, int &attacks);

	/// wrap the base form, and check for riding limitations
	int CanUseWeapon(const WeaponType *pWep, int riding);

	///@return 0 for weapons that need no skill, -1 if skill is lacking, or the skill level appropriate
	///@NOTE: also practises the appropriate skill
	int CanUseWeapon(const WeaponType *pWep);

	///@return number of "men" who can tax (includes monsters)
	int Taxers();

	/// move this from current object to 'newobj'
	void MoveUnit(Object *newobj);

	/// prepend unit name and forward to Faction::Event
	void Event(const AString &);

	/// prepend unit name and forward to Faction::Error
	void Error(const AString &);

	///@return number of items of type 'itemId' this unit can consume
	/// stop after you reach 'hint' items, -1 -> no limit
	int canConsume(int itemId, int hint = -1);
	void consume(int itemId, int num);

public: // data
	Faction *faction;
	Faction *formfaction;
	Object *object;
	AString *name;
	AString *describe;
	int num;
	int type;
	int alias;
	int gm_alias; // used for gm manual creation of new units
	int guard; // Also, avoid- see enum above
	int reveal;
	int flags;
	int taxing;
	int movepoints;
	int canattack;
	int nomove;
	SkillList skills;
	ItemList items;
	int combat;
	int readyItem;
	int readyWeapon[MAX_READY];
	int readyArmor[MAX_READY];
	AList oldorders;
	int needed; // For assessing maintenance
	int hunger;
	int stomach_space;
	int losses;
	int free;
	int practised; ///< Has this unit practised a skill this turn

	// Orders
	int destroy;
	int enter;
	Object *build;
	int leftShip;
	UnitId *promote;
	AList findorders;
	AList giveorders;
	AList withdraworders;
	AList buyorders;
	AList sellorders;
	AList forgetorders;
	CastOrder *castorders;
	TeleportOrder *teleportorders;
	Order *stealorders;
	Order *monthorders;
	AttackOrder *attackorders;
	EvictOrder *evictorders;
	ARegion *advancefrom;

	AList exchangeorders;
	AList turnorders;
	int inTurnBlock;
	Order *presentMonthOrders;
	int presentTaxing;
	Unit *former;

protected:
	/// limit the unit to one skill (for non MU)
	void limitSkillNoMagic();
	void limitSkillMagic();

	AString SpoilsReport();
	void SkillStarvation();
};

//----------------------------------------------------------------------------
class UnitPtr : public AListElem
{
public:
	Unit *ptr;
};
UnitPtr* GetUnitList(AList *, Unit *);

#endif


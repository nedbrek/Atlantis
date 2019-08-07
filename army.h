#ifndef ARMY_CLASS
#define ARMY_CLASS
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
#include "items.h" // NUM_ATTACK_TYPES
#include "shields.h"
#include "helper.h" // BITFIELD
#include "astring.h"
class Battle;
class ItemList;
class Object;
class Unit;

//----------------------------------------------------------------------------
/// One soldier in an army (wraps individual in unit)
class Soldier
{
public:
	/// construct one man of 'race' from 'unit', in 'object'
	/// 'regType' is used for riding
	Soldier(Unit *unit, Object *object, int regType, int race, int ass=0);

	/// check assigned spell
	void SetupSpell();

	/// get weapon and armor
	void SetupCombatItems();

	/// game-specific, and appears in specials.cpp
	void SetupHealing();

	//---effects
	/// check if bit 'eff' is set in effects
	int HasEffect(int eff) const;

	/// set effect (with side-effects)
	void SetEffect(int eff);

	/// clear effect (with side-effects)
	void ClearEffect(int eff);

	/// walk through all effects and clear those marked "one shot"
	void ClearOneTimeEffects();

	///@return 1 if armor is (randomly) successful
	int ArmorProtect(int weaponClass) const;

	/// give items back to original unit
	void RestoreItems();

	/// handle case where we survive (win or lose)
	void Alive(int win_state);

	/// handle case where we die
	void Dead();

public: // data
	AString name;
	Unit *unit;
	int race;
	int riding;
	int building;

	// Healing information
	int healing;
	int healtype;
	int healitem;
	int canbehealed;
	int regen;

	// Attack info
	int weapon;
	int attacktype;
	int askill;
	int attacks;
	int special;
	int slevel;

	// Defense info
	int dskill[NUM_ATTACK_TYPES];
	int armor;
	int hits;
	int maxhits;
	int damage;

	BITFIELD battleItems;
	int amuletofi;

	int effects;
};

typedef Soldier *SoldierPtr;

//----------------------------------------------------------------------------
/// All of the soldiers on one side
class Army
{
public:
	/// construct
	Army(Unit *ldr, AList *locs, int reg_type, int ass = 0);
	~Army();

	/// ?shuffles front and behind?
	void Reset();
	void endRound(Battle *b);

	/// deallocate soldiers, append spoils from monsters
	void Lose(Battle *b, ItemList *spoils);

	/// deallocate soldiers, distribute spoils
	void Win(Battle *b, ItemList *spoils);

	/// deallocate - no spoil
	void Tie(Battle *b);

	///@return 1 if anyone can be healed
	int CanBeHealed();

	/// do normal, then magical healing
	void DoHeal(Battle *b);

	/// apply hits (and regeneration if available)
	void Regenerate(Battle *b);

	/// generate spoils from wandering monsters
	void GetMonSpoils(ItemList *spoils, int mon_item, int free);

	///@return 1 if army is routed
	int Broken() const;

	///@return number of soldiers alive
	int NumAlive() const;

	///@return number of men willing to carry spoils
	int NumSpoilers() const;

	///@return number of men in front ranks? or able to advance to front?
	int CanAttack() const;

	///@return number of men in front ranks?
	int NumFront() const;

	/// shuffles attackers forward?
	Soldier* GetAttacker(int i, int &behind);

	///@return index of soldier with 'effect'
	int GetEffectNum(int effect);

	///@return index of soldier affected by 'special'
	int GetTargetNum(int special = 0);

	///@return soldier 'i'
	Soldier* GetTarget(int i);

	/// try to remove 'effect' from 'num' soldiers, @return actual number removed
	int RemoveEffects(int num, int effect);

	/// evaluate one attack on us
	int DoAnAttack( int special, int numAttacks, int attackType,
	      int attackLevel, int flags, int weaponClass, int effect,
	      int mountBonus, int *num_killed, bool riding);

	/// damage soldier 'i'
	///@return true if he dies
	bool DamageSoldier(int i);

	// in specials.cpp
	int CheckSpecialTarget(int, int);

public: // data
	SoldierPtr *soldiers;
	Unit *leader;
	ShieldList shields;
	int round;
	int tac;
	int canfront;
	int canbehind;
	int notfront;
	int notbehind;
	int count;

	int hitsalive; ///< Current number of "living hits"
	int hitstotal; ///< Number of hits at start of battle

	int kills_from[6];

private:
	void DoHealLevel(Battle *b, int type, int useItems);

	void WriteLosses(Battle *b);
};

#endif


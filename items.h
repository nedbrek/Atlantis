#ifndef ITEMS_H
#define ITEMS_H
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
class Ainfile;
class Aoutfile;
class AString;

/// Types of damage sources
enum
{
	ATTACK_COMBAT,  ///< normal melee fighting
	ATTACK_ENERGY,  ///< power of raw energy
	ATTACK_SPIRIT,  ///< spiritual assault
	ATTACK_WEATHER, ///< manipulation of the environment
	ATTACK_RIDING,  ///< weapon which requires height, leverage, and momentum
	ATTACK_RANGED,  ///< fighting at a distance with projectiles
	NUM_ATTACK_TYPES
};

/// Categories of items
enum ItemFlags
{
	IT_NORMAL   = 0x0001, ///< usually used in the sense of "not normal" for unusual objects
	IT_ADVANCED = 0x0002, ///< requires advanced skills to use and produce
	IT_TRADE    = 0x0004, ///< item is primarily for buying and selling for profit
	IT_MAN      = 0x0008, ///< item is actually a type of man who can be recruited
	IT_MONSTER  = 0x0010, ///< item is a monster (computer controlled entity)
	IT_MAGIC    = 0x0020, ///< item is magical
	IT_WEAPON   = 0x0040, ///< item can be used in combat
	IT_ARMOR    = 0x0080, ///< item protects in combat
	IT_MOUNT    = 0x0100, ///< item can be ridden
	IT_BATTLE   = 0x0200, ///< item has some application in battle (like healing potions)
	IT_SPECIAL  = 0x0400, ///< item has some other feature
	IT_TOOL     = 0x0800, ///< item is used to make items
	IT_FOOD     = 0x1000  ///< item can be used to support a unit
};

/// items which go into production of an item
struct Materials
{
	int item; ///< index of the input item
	int amt;  ///< number of input items required to produce 1 batch of this item
	const char *back_ref = nullptr;

	Materials(int i, int a)
	: item(i)
	, amt(a)
	{
	}

	Materials(const char *iabbr, int a);
};

/// items which be used to hitch another item
struct HitchItem
{
	int item; ///< index of the hitch item
	int walk;  ///< walking capacity
	const char *back_ref = nullptr;

	HitchItem(int i, int w)
	: item(i)
	, walk(w)
	{
	}

	HitchItem(const char *i, int w)
	: item(-2)
	, walk(w)
	, back_ref(i)
	{
	}
};

struct MultItem
{
	int mult_item; ///< index of item which can increase production
	int mult_val;  ///< number of additional items produced
	const char *back_ref = nullptr;

	MultItem(int mi, int mv)
	: mult_item(mi)
	, mult_val(mv)
	{
	}

	MultItem(const char *mi, int mv)
	: mult_item(-2)
	, mult_val(mv)
	, back_ref(mi)
	{
	}
};

/// Description of an item in the game
class ItemType
{
public:
	ItemType(
	    const char *n,
	    const char *p,
	    const char *a,
		 int f,
		 int ps,
		 int pl,
		 int pm,
		 int po,
		 const std::vector<Materials> &pi,
		 int ms,
		 int ml,
		 int mo,
		 const std::vector<Materials> &mi,
		 int wt,
		 int tp,
		 int bp,
		 int c,
		 int idx,
		 int bidx,
		 int wk,
		 int rd,
		 int fy,
		 int sm,
		 const std::vector<HitchItem> &hi,
		 const std::vector<MultItem> &mits
	)
	: name(n)
	, names(p)
	, abr(a)
	, flags(f)
	, pSkill(ps)
	, pLevel(pl)
	, pMonths(pm)
	, pOut(po)
	, pInput(pi)
	, mSkill(ms)
	, mLevel(ml)
	, mOut(mo)
	, mInput(mi)
	, weight(wt)
	, type(tp)
	, baseprice(bp)
	, combat(c)
	, index(idx)
	, battleindex(bidx)
	, walk(wk)
	, ride(rd)
	, fly(fy)
	, swim(sm)
	, hitchItems(hi)
	, mult_item(-1)
	, mult_val(0)
	, mult_items(mits)
	{
		if (!mult_items.empty())
		{
			mult_item = mult_items[0].mult_item;
			mult_val = mult_items[0].mult_val;
		}
	}

	///@return the productivity increase for item 'item_idx'
	int findMultVal(int item_idx) const;

public: // data
	const char *name;  ///< full name of the item
	const char *names; ///< plural
	const char *abr;   ///< short form

	enum
	{
		CANTGIVE = 0x01, ///< item cannot be target of GIVE command
		DISABLED = 0x02, ///< item is disabled in this ruleset
		ORINPUTS = 0x08, ///< item requires ANY one of its inputs (not ALL of them)
		SKILLOUT = 0x10, ///< batch size equal to the producer's skill, based on a fixed number of inputs
		NOSELL   = 0x20, ///< item is not sold by city markets
		NOBUY    = 0x40, ///< item is not bought by city markets
		NOMARKET = 0x60, ///< item does not appear in city markets
	};
	int flags;

	int pSkill; ///< skill required to produce this
	int pLevel; ///< skill level required to produce this
	int pMonths; ///< man months required to produce this
	int pOut; ///< how many of this we get in a batch
	std::vector<Materials> pInput; ///< normal production inputs

	int mSkill; ///< skill required for magical production
	int mLevel; ///< skill level for magical production
	int mOut; ///< how many of this are conjured in a batch
	std::vector<Materials> mInput; ///< inputs for magical production

	int weight; ///< in units (stones?)
	int type; ///< bitwise or of ItemFlags
	int baseprice; ///< normal price in market
	int combat; ///< bool? use in combat
	int index; ///< ?combat index?
	int battleindex; ///< ?special battle properties?

	int walk; ///< carrying capacity while moving on foot
	int ride; ///< carrying capacity while moving while mounted
	int fly;  ///< carrying capacity while moving through air
	int swim; ///< carrying capacity while moving through water

	std::vector<HitchItem> hitchItems;  ///< array of item/capacity that can be attached to for increased capacity

	int mult_item; ///< index of item which can increase production
	int mult_val;  ///< number of additional items produced
	std::vector<MultItem> mult_items;
};
extern std::vector<ItemType> ItemDefs;

/// Skill specializations for men
class ManType
{
public:
	enum Alignment
	{
		NEUTRAL,
		EVIL,
		GOOD,
		NUM_ALIGN
	};
	static const char *const ALIGN_STRS[NUM_ALIGN];

	const char *name;
	int max_skills; ///< maximum number of skills that can be learned
	int speciallevel; ///< highest level that can be attained in specialized skills
	int defaultlevel; ///< highest level that can be attained in everything else, except magic
	int magiclevel; ///< highest level that can be attained in magic skills
	int skills[6]; ///< indexes of specialized skills
	int hits;
	Alignment align;
};
extern ManType *ManDefs;

/// Properties of monsters
class MonType
{
public:
	int attackLevel; ///< effectiveness of attack
	int defense[NUM_ATTACK_TYPES]; ///< resistance to each attack type

	int numAttacks; ///< number of attacks per round
	int hits; ///< amount of damage absorbed before death
	int regen; ///< number of hit points regained each round (when enabled)

	int tactics; ///< same as the skill for men (gives first strike)
	int stealth; ///< same as the skill for men (avoid detection)
	int obs;     ///< same as the skill for men (defeat stealth)

	int special;
	int specialLevel;

	int silver;
	int spoiltype;
	int hostile; // percent
	int number;
	const char *name; ///< long name
};
extern MonType *MonDefs;

/// Attack types
enum AttackDef
{
	SLASHING,		///< e.g. sharpened edge attack (This is default)
	PIERCING,		///< e.g. spear or arrow attack
	CRUSHING,		///< e.g. mace attack
	CLEAVING,		///< e.g. axe attack
	ARMORPIERCING,	///< e.g. crossbow double bow
	MAGIC_ENERGY,	///< e.g. fire, dragon breath
	MAGIC_SPIRIT,	///< e.g. black wind
	MAGIC_WEATHER,	///< e.g. tornado
	NUM_WEAPON_CLASSES
};

/// Properties of a weapon
class WeaponType
{
public:
	enum
	{
		NEEDSKILL          = 0x001, ///< No bonus or use unless skilled
		ALWAYSREADY        = 0x002, ///< Ignore the 50% chance to attack
		NODEFENSE          = 0x004, ///< No combat defense against this weapon
		NOFOOT             = 0x008, ///< Weapon cannot be used on foot (e.g. lance)
		NOMOUNT            = 0x010, ///< Weapon cannot be used mounted (e.g. pike)
		SHORT              = 0x020, ///< Short melee weapon (e.g. shortsword, hatchet)
		LONG               = 0x040, ///< Long melee weapon (e.g. lance, pike)
		RANGED             = 0x080, ///< Missile weapon
		NOATTACKERSKILL    = 0x100, ///< Attacker gets no combat/skill defense.
		RIDINGBONUS        = 0x200, ///< Unit gets riding bonus on att and def.
		RIDINGBONUSDEFENSE = 0x400, ///< Unit gets riding bonus on def only.
	};
	int flags; ///< combination of the above

	int baseSkill;
	int orSkill;

	int weapClass;
	int attackType;

	// For numAttacks:
	// - A positive number is the number of attacks per round.
	// - A negative number is the number of rounds per attack.
	// - NUM_ATTACKS_HALF_SKILL indicates that the weapon gives as many
	//   attacks as the skill of the user divided by 2, rounded up.
	// - NUM_ATTACKS_HALF_SKILL+1 indicates that the weapon gives an extra
	//   attack above that, etc.
	// - NUM_ATTACKS_SKILL indicates the the weapon gives as many attacks
	//   as the skill of the user.
	// - NUM_ATTACKS_SKILL+1 indicates the the weapon gives as many
	//   attacks as the skill of the user + 1, etc.
	//
	enum
	{
		NUM_ATTACKS_HALF_SKILL = 50,
		NUM_ATTACKS_SKILL = 100
	};
	int numAttacks; ///< 1 for typical weapons, -2 (every other) for slow ones

	int attackBonus;  ///< amount added to attack combat factor
	int defenseBonus; ///< amount added to defense combat factor
	int mountBonus;   ///< ?
};
extern WeaponType *WeaponDefs;

/// Properties for armor
class ArmorType
{
public:
	enum
	{
		USEINASSASSINATE = 0x1 ///< can be used to protect from assassination
	};
	int flags; ///< combination of above

	int from; ///< denominator in protection calculation
	int saves[NUM_WEAPON_CLASSES]; ///< numerator in calculation (by attack type)
};
extern ArmorType *ArmorDefs;

/// Properties of a mount
class MountType
{
public:
	int skill; ///< index of skill needed to use this

	int minBonus; ///< minimum combat bonus (and minimal skill level) for this
	int maxBonus; ///< maximum combat bonus

	/// max combat bonus granted if this can normally fly but the region
	/// doesn't allow flying mounts
	int maxHamperedBonus;

	int mountSpecial; ///< If the mount has a special effect it generates when ridden in combat
	int specialLev; ///< ?amount of specialness?
};
extern MountType *MountDefs;

/// Properties of items for battle
class BattleItemType
{
public:
	enum
	{
		MAGEONLY = 0x1, ///< can only be used by those who know magic
		SPECIAL  = 0x2, ///< special
		SHIELD   = 0x4  ///< absorbs damage
	};
	int flags; ///< combination of above

	int itemNum;
	int index;
	int skillLevel;
};
extern BattleItemType *BattleItemDefs;

int ParseGiveableItem(AString *);
int ParseAllItems(const AString &token);
int ParseEnabledItem(const AString &);
int ParseBattleItem(int);

AString ItemString(int type,int num);
AString AttType(int atype);

bool IsSoldier(int);

/// Properties of an item held by a unit
struct Item : public AListElem
{
	Item(int t = 0, int n = 0);

	void Readin(Ainfile *f);
	void Writeout(Aoutfile *f) const;

	///@return a string describing this
	AString Report(int see_illusions) const;

	int type; ///< index into global table
	int num;  ///< number on hand
	int selling; ///< ?number intending to sell?
};

/// Group of Items for a unit
class ItemList : public AList
{
public:
	void Readin(Ainfile *f);
	void Writeout(Aoutfile *f);

	AString Report(int obs, int see_illusions, int no_first_comma);
	AString BattleReport();

	///@return sum of weights of all items
	int Weight();

	///@return number of items held matching 'type'
	int GetNum(int type);
	///@return number of items matching 'type' which can be sold
	int CanSell(int type);

	/// change the number of items of 'type' to 'num'
	void SetNum(int type, int num);

	/// mark 'num' items of 'type' as going to sell
	void Selling(int type, int num);
};

AString ShowSpecial(int special, int level, int expandLevel, int fromItem);

#endif

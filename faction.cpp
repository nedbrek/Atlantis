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
#include "faction.h"
#include "battle.h"
#include "object.h"
#include "unit.h"
#include "gamedata.h"
#include "game.h"
#include "fileio.h"
#include "gameio.h"
#include "astring.h"
#include <stdlib.h>

const char *as[] = {
	"Hostile",
	"Unfriendly",
	"Neutral",
	"Friendly",
	"Ally"
};
const char **AttitudeStrs = as;

const char *fs[] = {
	"War",
	"Trade",
	"Magic"
};
const char **FactionStrs = fs;

/// relationship from the owner to a target faction
class Attitude : public AListElem
{
public:
	explicit Attitude(int num = 0, int att = 0);

	void Writeout(Aoutfile *);
	void Readin(Ainfile *, ATL_VER version);

public: // data
	int factionnum;
	int attitude;
};

int ParseAttitude(AString *token)
{
	for (int i = 0; i < NATTITUDES; ++i)
		if (*token == AttitudeStrs[i])
			return i;

	return -1;
}

static
AString MonResist(int type, int val, int full)
{
	AString temp = "This monster ";

	// if printing hard numbers
	if (full)
	{
		temp += AString("has a resistance of ") + val;
	}
	else // print vague text
	{
		temp += "is ";
		if (val < 1) temp += "very susceptible";
		else if (val == 1) temp += "susceptible";
		else if (1 < val && val < 3) temp += "typically resistant"; // this is just 2
		else if (2 < val && val < 5) temp += "slightly resistant"; // 3 and 4
		else temp += "very resistant"; // 5 and above
	}

	temp += " to ";
	temp += AttType(type);
	temp += " attacks.";

	return temp;
}

static
AString WeapClass(int wclass)
{
	switch (wclass)
	{
	case SLASHING: return AString("slashing");
	case PIERCING: return AString("piercing");
	case CRUSHING: return AString("crushing");
	case CLEAVING: return AString("cleaving");
	case ARMORPIERCING: return AString("armor-piercing");
	case MAGIC_ENERGY: return AString("energy");
	case MAGIC_SPIRIT: return AString("spirit");
	case MAGIC_WEATHER: return AString("weather");
	default: return AString("unknown");
	}
}

static
AString WeapType(int flags, int wclass)
{
	AString type;
	if (flags & WeaponType::RANGED) type = "ranged";
	if (flags & WeaponType::LONG) type = "long";
	if (flags & WeaponType::SHORT) type = "short";

	type += " ";
	type += WeapClass(wclass);
	return type;
}

AString* ItemDescription(int item, int full)
{
	const ItemType &item_def = ItemDefs[item];

	if (item_def.flags & ItemType::DISABLED)
		return nullptr;

	const bool illusion = ((item_def.type & IT_MONSTER) &&
	      item_def.index == MONSTER_ILLUSION);

	AString *temp = new AString;

	// start with name abbr and weight
	*temp += AString(illusion?"illusory ":"") + item_def.name + " [" +
	    (illusion?"I":"") + item_def.abr + "], weight " + item_def.weight;

	// walking capacity
	if (item_def.walk)
	{
		const int cap = item_def.walk - item_def.weight;
		if (cap)
		{
			*temp += AString(", walking capacity ") + cap;
		}
		else
		{
			*temp += ", can walk";
		}
	}

	// Each hitch item has its own weight (larger creatures can pull more in the same wagon, for example)
	for (unsigned c = 0; c < item_def.hitchItems.size(); ++c)
	{
		const HitchItem &hitch = item_def.hitchItems[c];
		if (hitch.item == -1)
			continue;

		const int cap = item_def.walk - item_def.weight + hitch.walk;
		if (cap)
		{
			*temp += AString(", walking capacity ") + cap +
			    " when hitched to a " + ItemDefs[hitch.item].name;
		}
	}

	// riding capacity
	if (item_def.ride)
	{
		const int cap = item_def.ride - item_def.weight;
		if (cap)
		{
			*temp += AString(", riding capacity ") + cap;
		}
		else
		{
			*temp += ", can ride";
		}
	}

	// swimming capacity
	if (item_def.swim)
	{
		const int cap = item_def.swim - item_def.weight;
		if (cap)
		{
			*temp += AString(", swimming capacity ") + cap;
		}
		else
		{
			*temp += ", can swim";
		}
	}

	// flying capacity
	if (item_def.fly)
	{
		const int cap = item_def.fly - item_def.weight;
		if (cap)
		{
			*temp += AString(", flying capacity ") + cap;
		}
		else
		{
			*temp += ", can fly";
		}
	}

	// withdraw cost
	if (Globals->ALLOW_WITHDRAW)
	{
		if ((item_def.type & IT_NORMAL) && item != I_SILVER)
		{
			*temp += AString(", costs ") + (item_def.baseprice*5/2) +
			    " silver to withdraw";
		}
	}

	*temp += "."; // end first sentence

	// man specific description
	if (item_def.type & IT_MAN)
	{
		// pull ManDef index
		const int man = item_def.index;

		// number of skills
		*temp += " This race may know ";
		if (ManDefs[man].max_skills == -1)
			*temp += "any number of";
		else
			*temp += ManDefs[man].max_skills;
		*temp += " skills.";

		// hit points
		const int hits = ManDefs[man].hits > 0 ? ManDefs[man].hits : 1;

		*temp += AString(" This race takes ") + hits + " " +
		    ((hits > 1) ? "hits" : "hit") + " to kill.";

		// alignment
		*temp += AString(" This race has alignment ") +
		    ManType::ALIGN_STRS[ManDefs[man].align] + ".";

		// skill specializations (max level)
		*temp += " This race may study ";
		bool found = false;
		const unsigned len = sizeof(ManDefs[man].skills) / sizeof(ManDefs[man].skills[0]);
		for (unsigned c = 0; c < len; ++c)
		{
			const int skill = ManDefs[man].skills[c];
			if (skill != -1)
			{
				if (SkillDefs[skill].flags & SkillType::DISABLED)
					continue;

				if (found) *temp += ", ";
				if (found && c == len - 1) *temp += "and ";
				found = true;
				*temp += SkillStrs(ManDefs[man].skills[c]);
			}
		}

		if (found)
		{
			*temp += AString(" to level ") + ManDefs[man].speciallevel +
			    " and all other non-magic skills to level " + ManDefs[man].defaultlevel + ".";
		}
		else
		{
			*temp += AString("all non-magic skills to level ") +
			    ManDefs[man].defaultlevel + ".";
		}

		// if non-leaders can't study magic
		if ((Globals->MAGE_NONLEADERS || item == I_LEADERS) && ManDefs[man].magiclevel != 0)
		{
			*temp += AString(" This race may study all magic skills to ") + ManDefs[man].magiclevel + ".";
		}
		else
		{
			*temp += AString(" This race may not study magic skills.");
		}
	}

	// monster specific description
	if (item_def.type & IT_MONSTER)
	{
		*temp += " This is a monster.";

		const int mon = item_def.index;

		*temp += AString(" This monster attacks with a combat skill of ") +
		    MonDefs[mon].attackLevel + ".";

		for (int c = 0; c < NUM_ATTACK_TYPES; ++c)
		{
			*temp += AString(" ") + MonResist(c, MonDefs[mon].defense[c], full);
		}

		// special attack
		if (MonDefs[mon].special && MonDefs[mon].special != -1)
		{
			*temp += AString(" ") + "Monster can cast " +
			    ShowSpecial(MonDefs[mon].special, MonDefs[mon].specialLevel, 1, 0);
		}

		// if full info
		if (full)
		{
			const int hits = MonDefs[mon].hits > 0 ? MonDefs[mon].hits : 1;
			const int atts = MonDefs[mon].numAttacks > 0 ? MonDefs[mon].numAttacks : 1;

			*temp += AString(" This monster has ") + atts + " melee " +
			    ((atts > 1)?"attacks":"attack") + " per round and takes " +
			    hits + " " + ((hits > 1)?"hits":"hit") + " to kill.";

			const int regen = MonDefs[mon].regen;
			if (regen > 0)
			{
				*temp += AString(" This monsters regenerates ") + regen +
				    " hits per round of battle.";
			}

			*temp += AString(" This monster has a tactics score of ") +
			    MonDefs[mon].tactics + ", a stealth score of " +
			    MonDefs[mon].stealth + ", and an observation score of " +
			    MonDefs[mon].obs + ".";
		}

		// loot info
		*temp += " This monster might have ";
		if (MonDefs[mon].spoiltype != -1)
		{
			if (MonDefs[mon].spoiltype & IT_MAGIC)
				*temp += "magic items and ";
			if (MonDefs[mon].spoiltype & IT_ADVANCED)
				*temp += "advanced items and ";
			if (MonDefs[mon].spoiltype & IT_NORMAL)
				*temp += "normal or trade items and ";
		}
		*temp += "silver as treasure.";
	}

	// weapon specific description
	if (item_def.type & IT_WEAPON)
	{
		const int wep = item_def.index;
		WeaponType *pW = &WeaponDefs[wep];

		*temp += " This is a ";
		*temp += WeapType(pW->flags, pW->weapClass) + " weapon.";

		if (pW->flags & WeaponType::NEEDSKILL)
		{
			*temp += AString(" Knowledge of ") + SkillStrs(pW->baseSkill);
			if (pW->orSkill != -1)
				*temp += AString(" or ") + SkillStrs(pW->orSkill);
			*temp += " is needed to wield this weapon.";
		}
		else
		{
			if (pW->baseSkill == -1 && pW->orSkill == -1)
				*temp += " No skill is needed to wield this weapon.";
		}

		int flag = 0;
		if (pW->attackBonus != 0)
		{
			*temp += " This weapon grants a ";
			*temp += ((pW->attackBonus > 0) ? "bonus of " : "penalty of ");
			*temp += abs(pW->attackBonus);
			*temp += " on attack";
			flag = 1;
		}

		if (pW->defenseBonus != 0)
		{
			if (flag)
			{
				if (pW->attackBonus == pW->defenseBonus)
				{
					*temp += " and defense.";
					flag = 0;
				}
				else
				{
					*temp += " and a ";
				}
			}
			else
			{
				*temp += " This weapon grants a ";
				flag = 1;
			}

			if (flag)
			{
				*temp += ((pW->defenseBonus > 0) ? "bonus of " : "penalty of ");
				*temp += abs(pW->defenseBonus);
				*temp += " on defense.";
				flag = 0;
			}
		}

		if (flag) *temp += ".";

		if (pW->mountBonus && full)
		{
			*temp += " This weapon ";
			if (pW->attackBonus != 0 || pW->defenseBonus != 0)
				*temp += "also ";
			*temp += "grants a ";
			*temp += ((pW->mountBonus > 0) ? "bonus of " : "penalty of ");
			*temp += abs(pW->mountBonus);
			*temp += " against mounted opponents.";
		}

		if (pW->flags & WeaponType::NOFOOT)
			*temp += " Only mounted troops may use this weapon.";
		else if (pW->flags & WeaponType::NOMOUNT)
			*temp += " Only foot troops may use this weapon.";

		if (pW->flags & WeaponType::RIDINGBONUS)
		{
			*temp += " Wielders of this weapon, if mounted, get their riding "
			    "skill bonus on combat attack and defense.";
		}
		else if (pW->flags & WeaponType::RIDINGBONUSDEFENSE)
		{
			*temp += " Wielders of this weapon, if mounted, get their riding "
				"skill bonus on combat defense.";
		}

		if (pW->flags & WeaponType::NODEFENSE)
		{
			*temp += " Defenders are treated as if they have an "
			    "effective combat skill of 0.";
		}

		if (pW->flags & WeaponType::NOATTACKERSKILL)
		{
			*temp += " Attackers do not get skill bonus on defense.";
		}

		if (pW->flags & WeaponType::ALWAYSREADY)
		{
			*temp += " Wielders of this weapon never miss a round to ready their weapon.";
		}
		else
		{
			*temp += " There is a 50% chance that the wielder of this weapon "
				"gets a chance to attack in any given round.";
		}

		if (full)
		{
			*temp += AString(" This weapon attacks versus the target's ") +
				"defense against " + AttType(pW->attackType) + " attacks.";

			const int atts = pW->numAttacks;
			*temp += AString(" This weapon allows ");
			if (atts > 0)
			{
				if (atts >= WeaponType::NUM_ATTACKS_HALF_SKILL)
				{
					int max = WeaponType::NUM_ATTACKS_HALF_SKILL;
					const char *attd = "half the skill level (rounded up)";
					if (atts >= WeaponType::NUM_ATTACKS_SKILL)
					{
						max = WeaponType::NUM_ATTACKS_SKILL;
						attd = "the skill level";
					}
					*temp += "a number of attacks equal to ";
					*temp += attd;
					*temp += " of the attacker";
					const int val = atts - max;
					if (val > 0) *temp += AString(" plus ") + val;
				}
				else
				{
					*temp += AString(atts) + ((atts==1)?" attack ":" attacks");
				}
				*temp += " per round.";
			}
			else // 1/n encoded in negative space
			{
				*temp += "1 attack every ";
				if (atts == -1) *temp += "round.";
				else *temp += AString(-atts) + " rounds.";
			}
		}
	}

	// armor specific description
	if (item_def.type & IT_ARMOR)
	{
		*temp += " This is a type of armor.";
		const int arm = item_def.index;
		ArmorType *pA = &ArmorDefs[arm];

		*temp += " This armor will protect its wearer ";
		for (int i = 0; i < NUM_WEAPON_CLASSES; ++i)
		{
			if (i == NUM_WEAPON_CLASSES - 1)
			{
				*temp += ", and ";
			}
			else if (i > 0)
			{
				*temp += ", ";
			}

			const int percent = (int)(((float)pA->saves[i]*100.0) /
			     (float)pA->from+0.5);
			*temp += AString(percent) + "% of the time versus " +
			    WeapClass(i) + " attacks";
		}

		*temp += ".";

		if (full)
		{
			if (pA->flags & ArmorType::USEINASSASSINATE)
			{
				*temp += " This armor may be worn during assassination attempts.";
			}
		}
	}

	// tool specific description
	if (item_def.type & IT_TOOL)
	{
		*temp += " This is a tool.";
		*temp += " This item increases the production of ";

		// find last item (for commas and "and")
		unsigned last = -1;
		for (int i = ItemDefs.size() - 1; i > 0; --i)
		{
			const ItemType &itm = ItemDefs[i];
			if (itm.flags & ItemType::DISABLED)
				continue;

			if (itm.findMultVal(item) != -1)
			{
				last = i;
				break;
			}
		}

		int comma = 0;
		for (unsigned i = 0; i < ItemDefs.size(); ++i)
		{
			const ItemType &itm = ItemDefs[i];
		   if (itm.flags & ItemType::DISABLED)
				continue;

			const int mult_val = itm.findMultVal(item);
			if (mult_val == -1)
				continue;

			if (comma)
			{
				if (last == i)
				{
					if (comma > 1)
						*temp += ",";

					*temp += " and ";
				}
				else
				{
					*temp += ", ";
				}
			}
			comma++;

			if (i == I_SILVER)
			{
				*temp += "entertainment";
			}
			else
			{
				*temp += itm.names;
			}
			*temp += AString(" by ") + mult_val;
		}
		*temp += ".";
	}

	// trade good specific description
	if (item_def.type & IT_TRADE)
	{
		*temp += " This is a trade good.";

		if (full)
		{
			if (Globals->RANDOM_ECONOMY)
			{
				int maxbuy, minbuy, maxsell, minsell;
				if (Globals->MORE_PROFITABLE_TRADE_GOODS)
				{
					minsell = (item_def.baseprice*250)/100;
					maxsell = (item_def.baseprice*350)/100;
					minbuy = (item_def.baseprice*100)/100;
					maxbuy = (item_def.baseprice*190)/100;
				}
				else
				{
					minsell = (item_def.baseprice*150)/100;
					maxsell = (item_def.baseprice*200)/100;
					minbuy = (item_def.baseprice*100)/100;
					maxbuy = (item_def.baseprice*150)/100;
				}
				*temp += AString(" This item can be bought for between ") +
				    minbuy + " and " + maxbuy + " silver.";
				*temp += AString(" This item can be sold for between ") +
				    minsell+ " and " + maxsell+ " silver.";
			}
			else
			{
				*temp += AString(" This item can be bought and sold for ") +
				    item_def.baseprice + " silver.";
			}
		}
	}

	// mount specific description
	if (item_def.type & IT_MOUNT)
	{
		*temp += " This is a mount.";

		const int mnt = item_def.index;
		MountType *pM = &MountDefs[mnt];

		if (pM->skill == -1)
		{
			*temp += " No skill is required to use this mount.";
		}
		else if(SkillDefs[pM->skill].flags & SkillType::DISABLED)
		{
			*temp += " This mount is unridable.";
		}
		else
		{
			*temp += AString(" This mount requires ") + SkillStrs(pM->skill) +
			    " of at least level " + pM->minBonus + " to ride in combat.";
		}

		*temp += AString(" This mount gives a minimum bonus of +") +
		    pM->minBonus + " when ridden into combat.";
		*temp += AString(" This mount gives a maximum bonus of +") +
		    pM->maxBonus + " when ridden into combat.";

		if (full)
		{
			if (item_def.fly)
			{
				*temp += AString(" This mount gives a maximum bonus of +") +
				    pM->maxHamperedBonus + " when ridden into combat in terrain" +
				    " which allows ridden mounts but not flying mounts.";
			}
			if (pM->mountSpecial != -1)
			{
				*temp += AString(" When ridden, this mount causes ") +
				    ShowSpecial(pM->mountSpecial, pM->specialLev, 1, 0);
			}
		}
	}

	// production specific description
	if (item_def.pSkill != -1 &&
	    !(SkillDefs[item_def.pSkill].flags & SkillType::DISABLED))
	{
		*temp += AString(" Units with ") + SkillStrs(item_def.pSkill) +
		    " " + item_def.pLevel + " may PRODUCE ";

		if (item_def.flags & ItemType::SKILLOUT)
			*temp += "a number of this item equal to their skill level";
		else
			*temp += "this item";

		const unsigned len = item_def.pInput.size();
		int count = 0; // commas
		int tot = len;
		for (unsigned c = 0; c < len; c++)
		{
			const int itm = item_def.pInput[c].item;
			if (itm == -1 || (ItemDefs[itm].flags & ItemType::DISABLED))
			{
				tot--;
				continue;
			}

			const int amt = item_def.pInput[c].amt;
			if (count == 0)
			{
				*temp += " from ";
				if (item_def.flags & ItemType::ORINPUTS)
					*temp += "any of ";
			}
			else if (count == tot)
			{
				if(c > 1) *temp += ",";
				*temp += " and ";
			}
			else
			{
				*temp += ", ";
			}
			count++;
			*temp += AString(amt) + " " + ItemDefs[itm].names;
		}

		if (item_def.pOut)
		{
			*temp += AString(" at a rate of ") + item_def.pOut;
			if (item_def.pMonths)
			{
				if (item_def.pMonths == 1)
				{
					*temp += " per man-month.";
				}
				else
				{
					*temp += AString(" per ") + item_def.pMonths +
						" man-months.";
				}
			}
		}
	}

	// magic production specific description
	if (item_def.mSkill != -1 &&
	    !(SkillDefs[item_def.mSkill].flags & SkillType::DISABLED))
	{
		*temp += AString(" Units with ") + SkillStrs(item_def.mSkill) +
		    " of at least level " + item_def.mLevel +
		    " may attempt to create this item via magic";

		const unsigned len = item_def.mInput.size();

		int count = 0;
		int tot = len;
		for (unsigned c = 0; c < len; ++c)
		{
			const int itm = item_def.mInput[c].item;
			const int amt = item_def.mInput[c].amt;
			if (itm == -1 || (ItemDefs[itm].flags & ItemType::DISABLED))
			{
				tot--;
				continue;
			}

			if (count == 0)
			{
				*temp += " at a cost of ";
			}
			else if (count == tot)
			{
				if(c > 1) *temp += ",";
				*temp += " and ";
			}
			else
			{
				*temp += ", ";
			}
			count++;
			*temp += AString(amt) + " " + ItemDefs[itm].names;
		}
		if (count)
		{
			*temp += " (costs are consumed even if the attempt fails)";
		}
		*temp += ".";
	}

	// battle item specific description
	if ((item_def.type & IT_BATTLE) && full)
	{
		*temp += " This item is a miscellaneous combat item.";
		for (int i = 0; i < NUMBATTLEITEMS; ++i)
		{
			BattleItemType &bitem = BattleItemDefs[i];
			if (bitem.itemNum == item)
			{
				if (bitem.flags & BattleItemType::MAGEONLY)
				{
					*temp += " This item may only be used by a mage";
					if (Globals->APPRENTICES_EXIST)
						*temp += " or an apprentice";

					*temp += ".";
				}

				*temp += AString(" ") + "Item can cast " +
				   ShowSpecial(bitem.index, bitem.skillLevel, 1, 1);
			}
		}
	}

	if ((item_def.flags & ItemType::CANTGIVE) && full)
	{
		*temp += " This item cannot be given to other units.";
	}

	return temp;
}

FactionVector::FactionVector(int size)
{
	vector = new Faction *[size];
	vectorsize = size;
	ClearVector();
}

FactionVector::~FactionVector()
{
	delete[] vector;
}

void FactionVector::ClearVector()
{
	for (int i = 0; i < vectorsize; ++i)
		vector[i] = nullptr;
}

void FactionVector::SetFaction(int x, Faction *fac)
{
	vector[x] = fac;
}

Faction* FactionVector::GetFaction(int x)
{
	return vector[x];
}

Attitude::Attitude(int num, int att)
: factionnum(num)
, attitude(att)
{
}

void Attitude::Writeout(Aoutfile *f)
{
	f->PutInt(factionnum);
	f->PutInt(attitude);
}

void Attitude::Readin(Ainfile *f, ATL_VER v)
{
	factionnum = f->GetInt();
	attitude = f->GetInt();
}

Faction::Faction(Game &g)
: game_(g)
{
	exists = 1;
	name = 0;
	for (int i = 0; i < NFACTYPES; ++i) {
		type[i] = 1;
	}
	lastchange = -6;
	address = 0;
	password = 0;
	times = 0;
	temformat = TEMPLATE_OFF;
	quit = 0;
	defaultattitude = A_NEUTRAL;
	unclaimed = 0;
	pReg = NULL;
	noStartLeader = 0;
	alignments_ = ALL_NEUTRAL;
}

Faction::Faction(Game &g, int n)
: game_(g)
{
	exists = 1;
	num = n;
	for (int i = 0; i < NFACTYPES; ++i) {
		type[i] = 1;
	}
	lastchange = -6;
	name = new AString;
	*name = AString("Faction (") + AString(num) + AString(")");
	address = new AString("NoAddress");
	password = new AString("none");
	times = 1;
	temformat = TEMPLATE_LONG;
	defaultattitude = A_NEUTRAL;
	quit = 0;
	unclaimed = 0;
	pReg = NULL;
	noStartLeader = 0;
	alignments_ = ALL_NEUTRAL;
}

Faction::~Faction()
{
	delete name;
	delete address;
	delete password;
	attitudes.DeleteAll();
}

void Faction::Writeout(Aoutfile *f)
{
	f->PutInt(num);

	for (int i = 0; i < NFACTYPES; ++i) {
		f->PutInt(type[i]);
	}

	f->PutInt(lastchange);
	f->PutInt(lastorders);
	f->PutInt(unclaimed);
	f->PutStr(*name);
	f->PutStr(*address);
	f->PutStr(*password);
	f->PutInt(times);
	f->PutInt(temformat);

	skills.Writeout(f);
	f->PutInt(-1);
	items.Writeout(f);
	f->PutInt(defaultattitude);
	f->PutInt(attitudes.Num());
	forlist((&attitudes))
		((Attitude*)elem)->Writeout(f);
	f->PutInt(alignments_);
}

void Faction::Readin(Ainfile *f, ATL_VER v)
{
	num = f->GetInt();

	for (int i = 0; i < NFACTYPES; ++i) {
		type[i] = f->GetInt();
	}

	lastchange = f->GetInt();
	lastorders = f->GetInt();
	unclaimed = f->GetInt();
	name = f->GetStr();
	address = f->GetStr();
	password = f->GetStr();
	times = f->GetInt();
	temformat = f->GetInt();

	skills.Readin(f);
	defaultattitude = f->GetInt();

	// Is this a new version of the game file
	if (defaultattitude == -1)
	{
		items.Readin(f);
		defaultattitude = f->GetInt();
	}

	// attitudes
	int n = f->GetInt();
	for (int i = 0; i < n; ++i)
	{
		Attitude * a = new Attitude;
		a->Readin(f, v);
		if (a->factionnum == num)
		{
			delete a;
		}
		else
		{
			attitudes.Add(a);
		}
	}

	alignments_ = Alignments(f->GetInt());
}

void Faction::View()
{
	AString temp;
	temp = AString("Faction ") + num + AString(" : ") + *name;
	Awrite(temp);
}

void Faction::SetName(AString *s)
{
	if (s)
	{
		AString *newname = s->getlegal();
		delete s;
		if (!newname)
			return;

		delete name;
		*newname += AString(" (") + num + ")";
		name = newname;
	}
}

void Faction::SetNameNoChange(AString *s)
{
	if (s)
	{
		delete name;
		name = new AString( *s );
	}
}

void Faction::SetAddress(const AString &strNewAddress)
{
	delete address;
	address = new AString(strNewAddress);
}

AString Faction::FactionTypeStr()
{
	if (IsNPC())
		return AString("NPC");

	if (Globals->FACTION_LIMIT_TYPE == GameDefs::FACLIM_UNLIMITED)
		return AString("Unlimited");

	if (Globals->FACTION_LIMIT_TYPE == GameDefs::FACLIM_MAGE_COUNT)
		return AString("Normal");

	AString temp;
	if (Globals->FACTION_LIMIT_TYPE == GameDefs::FACLIM_FACTION_TYPES)
	{
		int comma = 0;
		for (int i = 0; i < NFACTYPES; ++i)
		{
			if (type[i])
			{
				if (comma)
				{
					temp += ", ";
				}
				else
				{
					comma = 1;
				}
				temp += AString(FactionStrs[i]) + " " + type[i];
			}
		}

		if (!comma)
			return AString("none");
	}
	return temp;
}

void Faction::WriteReport(Areport *f, Game *pGame)
{
	if (IsNPC() && num == 1)
	{
		if (Globals->GM_REPORT || (pGame->currentMonth() == 0 && pGame->currentYear() == 1))
		{
			int i, j;
			// Put all skills, items and objects in the GM report
			shows.DeleteAll();
			for (i = 0; i < NSKILLS; i++)
			{
				for (j = 1; j < 6; j++)
				{
					shows.Add(new ShowSkill(i, j));
				}
			}

			if (shows.Num())
			{
				f->PutStr("Skill reports:" );
				forlist(&shows) {
					AString *string = ((ShowSkill *)elem)->Report(this);
					if (string) {
						f->PutStr("");
						f->PutStr(*string);
						delete string;
					}
				}
				shows.DeleteAll();
				f->EndLine();
			}

			itemshows.DeleteAll();
			for (unsigned i = 0; i < ItemDefs.size(); i++)
			{
				AString *show = ItemDescription(i, 1);
				if (show) {
					itemshows.Add(show);
				}
			}
			if (itemshows.Num()) {
				f->PutStr("Item reports:");
				forlist(&itemshows) {
					f->PutStr("");
					f->PutStr(*((AString *)elem));
				}
				itemshows.DeleteAll();
				f->EndLine();
			}

			objectshows.DeleteAll();
			for (unsigned i = 0; i < NOBJECTS; i++) {
				AString *show = ObjectDescription(i);
				if(show) {
					objectshows.Add(show);
				}
			}
			if (objectshows.Num()) {
				f->PutStr("Object reports:");
				forlist(&objectshows) {
					f->PutStr("");
					f->PutStr(*((AString *)elem));
				}
				objectshows.DeleteAll();
				f->EndLine();
			}

			present_regions.DeleteAll();
			forlist(&(pGame->getRegions())) {
				ARegion *reg = (ARegion *)elem;
				ARegionPtr *ptr = new ARegionPtr;
				ptr->ptr = reg;
				present_regions.Add(ptr);
			}
			{
				forlist(&present_regions) {
					((ARegionPtr*)elem)->ptr->WriteReport(f, this,
														  pGame->currentMonth(),
														  &(pGame->getRegions()));
				}
			}
			present_regions.DeleteAll();
		}
		deleteAll(errors_);
		events.DeleteAll();
		battles.DeleteAll();
		return;
	}

	f->PutStr("Atlantis Report For:");
	if (Globals->FACTION_LIMIT_TYPE == GameDefs::FACLIM_MAGE_COUNT ||
	    Globals->FACTION_LIMIT_TYPE == GameDefs::FACLIM_UNLIMITED)
	{
		f->PutStr(*name);
	}
	else if (Globals->FACTION_LIMIT_TYPE == GameDefs::FACLIM_FACTION_TYPES)
	{
		f->PutStr(*name + " (" + FactionTypeStr() + ")");
	}

	f->PutStr(AString(MonthNames[ pGame->currentMonth() ]) + ", Year " + pGame->currentYear() );
	f->EndLine();

	f->PutStr( AString("Atlantis Engine Version: ") +
	    ATL_VER_STRING( CURRENT_ATL_VER ));
	f->PutStr( AString( Globals->RULESET_NAME ) + ", Version: " +
	    ATL_VER_STRING( Globals->RULESET_VERSION ));
	f->EndLine();

	if (!times)
	{
		f->PutStr("Note: The Times is not being sent to you.");
		f->EndLine();
	}

	if (*password == "none")
	{
		f->PutStr("REMINDER: You have not set a password for your faction!");
		f->EndLine();
	}

	if (Globals->MAX_INACTIVE_TURNS != -1)
	{
		int cturn = pGame->TurnNumber() - lastorders;
		if ((cturn >= (Globals->MAX_INACTIVE_TURNS - 3)) && !IsNPC())
		{
			cturn = Globals->MAX_INACTIVE_TURNS - cturn;
			f->PutStr( AString("WARNING: You have ") + cturn +
			    AString(" turns until your faction is automatically ")+
			    AString("removed due to inactivity!"));
			f->EndLine();
		}
	}

	if (!exists)
	{
		if (quit == QUIT_AND_RESTART)
		{
			f->PutStr( "You restarted your faction this turn. This faction "
			    "has been removed, and a new faction has been started "
			    "for you. (Your new faction report will come in a "
			    "separate message.)" );
		}
		else if (quit == QUIT_GAME_OVER)
		{
			f->PutStr( "I'm sorry, the game has ended. Better luck in "
			    "the next game you play!" );
		}
		else if (quit == QUIT_WON_GAME)
		{
			f->PutStr("Congratulations, you have won the game!");
		}
		else
		{
			f->PutStr("I'm sorry, your faction has been eliminated.");
			f->PutStr("If you wish to restart, please let the "
			    "Gamemaster know, and you will be restarted for "
			    "the next available turn." );
		}

		f->PutStr("");
	}

	f->PutStr("Faction Status:");
	if (Globals->FACTION_LIMIT_TYPE == GameDefs::FACLIM_MAGE_COUNT)
	{
		f->PutStr( AString("Mages: ") + nummages + " (" +
		    pGame->AllowedMages( this ) + ")");
		if (Globals->APPRENTICES_EXIST)
		{
			f->PutStr( AString("Apprentices: ") + numapprentices + " (" +
			    pGame->AllowedApprentices(this)+ ")");
		}
	}
	else if (Globals->FACTION_LIMIT_TYPE == GameDefs::FACLIM_FACTION_TYPES)
	{
		f->PutStr( AString("Tax Regions: ") + war_regions.Num() + " (" +
		    pGame->AllowedTaxes( this ) + ")");
		f->PutStr( AString("Trade Regions: ") + trade_regions.Num() + " (" +
		    pGame->AllowedTrades( this ) + ")");
		f->PutStr( AString("Mages: ") + nummages + " (" +
		    pGame->AllowedMages( this ) + ")");

		if (Globals->APPRENTICES_EXIST)
		{
			f->PutStr( AString("Apprentices: ") + numapprentices + " (" +
			    pGame->AllowedApprentices(this)+ ")");
		}
	}

	f->PutStr("");

	// alignment (TODO: ensure mapping from faction alignment to man alignment
	f->PutStr(AString("Your faction is ") + ManType::ALIGN_STRS[alignments_] + ".");

	f->PutStr("");

	if (!errors_.empty())
	{
		f->PutStr("Errors during turn:");
		for (auto p : errors_)
		{
			f->PutStr(*p);
			delete p;
		}
		errors_.clear();

		f->EndLine();
	}

	if (battles.Num())
	{
		f->PutStr("Battles during turn:");
		forlist(&battles) {
			((BattlePtr*)elem)->ptr->Report(f, this);
		}
		battles.DeleteAll();
	}

	if (events.Num())
	{
		f->PutStr("Events during turn:");
		forlist((&events)) {
			f->PutStr(*((AString*)elem));
		}
		events.DeleteAll();
		f->EndLine();
	}

	if (shows.Num())
	{
		f->PutStr("Skill reports:");
		forlist(&shows)
		{
			AString *string = ((ShowSkill*)elem)->Report(this);
			if (string)
			{
				f->PutStr("");
				f->PutStr(*string);
				delete string;
			}
		}
		shows.DeleteAll();
		f->EndLine();
	}

	if (itemshows.Num())
	{
		f->PutStr("Item reports:");
		forlist(&itemshows)
		{
			f->PutStr("");
			f->PutStr(*((AString*)elem));
		}
		itemshows.DeleteAll();
		f->EndLine();
	}

	if (objectshows.Num())
	{
		f->PutStr("Object reports:");
		forlist(&objectshows)
		{
			f->PutStr("");
			f->PutStr(*((AString *)elem));
		}
		objectshows.DeleteAll();
		f->EndLine();
	}

	// Attitudes
	AString temp = AString("Declared Attitudes (default ") +
	    AttitudeStrs[defaultattitude] + "):";
	f->PutStr(temp);

	for (int i = 0; i < NATTITUDES; ++i)
	{
		temp = AString(AttitudeStrs[i]) + " : ";
		int j = 0;
		forlist((&attitudes))
		{
			Attitude *a = (Attitude*)elem;
			if (a->attitude == i)
			{
				if (j) temp += ", ";
				temp += *(pGame->getFaction(a->factionnum)->name);
				j = 1;
			}
		}
		if (!j) temp += "none";
		temp += ".";
		f->PutStr(temp);
	}
	f->EndLine();

	temp = AString("Unclaimed silver: ") + unclaimed + ".";
	f->PutStr(temp);
	f->PutStr("");

	forlist(&present_regions) {
		((ARegionPtr*)elem)->ptr->WriteReport(f, this, pGame->currentMonth(),
		     &pGame->getRegions());
	}

	if (temformat != TEMPLATE_OFF)
	{
		f->PutStr("");

		switch (temformat)
		{
		case TEMPLATE_SHORT: f->PutStr("Orders Template (Short Format):"); break;

		case TEMPLATE_LONG: f->PutStr("Orders Template (Long Format):"); break;

		case TEMPLATE_MAP: f->PutStr("Orders Template (Map Format):"); break;
		}
		f->PutStr("");

		temp = AString("#atlantis ") + num;
		if (!(*password == "none"))
		{
			temp += AString(" \"") + *password + "\"";
		}
		f->PutStr(temp);

		forlist((&present_regions))
		{
			((ARegionPtr*)elem)->ptr->WriteTemplate(f, this,
			   &pGame->getRegions(), pGame->currentMonth());
		}
	}
	else
	{
		f->PutStr("");
		f->PutStr("Orders Template (Off)");
	}

	f->PutStr("");
	f->PutStr("#end");
	f->EndLine();

	present_regions.DeleteAll();
}

void Faction::WriteFacInfo(Aoutfile *file)
{
	file->PutStr( AString( "Faction: " ) + num );
	file->PutStr( AString( "Name: " ) + *name );
	file->PutStr( AString( "Email: " ) + *address );
	file->PutStr( AString( "Password: " ) + *password );
	file->PutStr( AString( "LastOrders: " ) + lastorders );
	file->PutStr( AString( "SendTimes: " ) + times );

	for (auto pStr : extraPlayers_)
	{
		file->PutStr(*pStr);
		delete pStr;
	}

	extraPlayers_.clear();
}

void Faction::CheckExist(ARegionList *regs)
{
	if (IsNPC())
		return;

	exists = 0;
	forlist(regs)
	{
		ARegion *reg = (ARegion*)elem;
		if (reg->Present(this))
		{
			exists = 1;
			return;
		}
	}
}

void Faction::Error(const AString &s)
{
	if (IsNPC())
		return;

	if (errors_.size() > 1000)
	{
		if (errors_.size() == 1001)
		{
			errors_.emplace_back(new AString("Too many errors!"));
		}
		return;
	}

	errors_.emplace_back(new AString(s));
}

void Faction::Event(const AString &s)
{
	if (!IsNPC())
		events.Add(new AString(s));
}

void Faction::RemoveAttitude(int f)
{
	forlist((&attitudes))
	{
		Attitude *a = (Attitude*)elem;
		if (a->factionnum == f)
		{
			attitudes.Remove(a);
			delete a;
			return;
		}
	}
}

int Faction::GetAttitude(int n)
{
	// self
	if (n == num)
		return A_ALLY;

	// alignment check
	int max_relation = A_ALLY;
	if (Globals->ALIGN_RESTRICT_RELATIONS && alignments_ != ALL_NEUTRAL)
	{
		Faction *t = game_.getFaction(n);
		if (t->alignments_ != ALL_NEUTRAL && alignments_ != t->alignments_)
			max_relation = A_UNFRIENDLY;
	}

	forlist((&attitudes))
	{
		Attitude *a = (Attitude *) elem;
		if (a->factionnum == n)
			return std::min(max_relation, a->attitude);
	}
	return std::min(max_relation, defaultattitude);
}

void Faction::SetAttitude(int num, int att)
{
	forlist((&attitudes))
	{
		Attitude *a = (Attitude*)elem;
		if (a->factionnum == num)
		{
			if (att == -1)
			{
				attitudes.Remove(a);
				delete a;
				return;
			}
			a->attitude = att;
			return;
		}
	}

	// if not remove attitude
	if (att != -1)
		attitudes.Add(new Attitude(num, att));
}

int Faction::CanCatch(ARegion *r, Unit *t)
{
	if (TerrainDefs[r->type].similar_type == R_OCEAN)
		return 1; // can catch everyone at sea

	int def = t->GetDefenseRiding();

	forlist(&r->objects)
	{
		Object *o = (Object*)elem;
		for(auto &u : o->getUnits())
		{
			if (u == t && o->type != O_DUMMY)
				return 1;

			if (u->faction == this && u->GetAttackRiding() >= def)
				return 1;
		}
	}
	return 0;
}

int Faction::CanSee(ARegion *r, Unit *u, int practise)
{
	if (u->faction == this)
		return 2; // self

	if (u->reveal == REVEAL_FACTION)
		return 2; // easy to spot

	int retval = 0;
	if (u->reveal == REVEAL_UNIT)
		retval = 1;

	bool detfac = false;
	forlist((&r->objects))
	{
		Object *obj = (Object*)elem;
		const bool dummy = (obj->type == O_DUMMY);

		for(auto &temp : obj->getUnits())
		{
			if (u == temp && !dummy)
				retval = 1;

			if (temp->faction == this)
			{
				if (temp->GetSkill(S_OBSERVATION) > u->GetSkill(S_STEALTH))
				{
					if (practise)
					{
						temp->Practise(S_OBSERVATION);
						temp->Practise(S_TRUE_SEEING);
						retval = 2;
					}
					else
						return 2;
				}
				else
				{
					if (temp->GetSkill(S_OBSERVATION) == u->GetSkill(S_STEALTH))
					{
						if (practise)
						{
							temp->Practise(S_OBSERVATION);
							temp->Practise(S_TRUE_SEEING);
						}
						if (retval < 1)
							retval = 1;
					}
				}

				// mind reading 3 allows detecting faction
				if (temp->GetSkill(S_MIND_READING) > 2)
					detfac = true;
			}
		}
	}

	if (retval == 1 && detfac)
		return 2;

	return retval;
}

void Faction::DefaultOrders()
{
	war_regions.DeleteAll();
	trade_regions.DeleteAll();
	numshows = 0;
}

void Faction::TimesReward()
{
	if (Globals->TIMES_REWARD)
	{
		Event(AString("Times reward of ") + Globals->TIMES_REWARD + " silver.");
		unclaimed += Globals->TIMES_REWARD;
	}
}

void Faction::SetNPC()
{
	for (int i = 0; i < NFACTYPES; ++i)
		type[i] = -1;
}

int Faction::IsNPC()
{
	return (type[F_WAR] == -1) ? 1 : 0;
}

Faction* GetFaction(AList *facs, int n)
{
	forlist(facs)
	{
		if (((Faction*)elem)->num == n)
			return (Faction*)elem;
	}

	return nullptr;
}

Faction* GetFaction2(AList *facs, int n)
{
	forlist(facs)
	{
		if (((FactionPtr*)elem)->ptr->num == n)
			return ((FactionPtr*)elem)->ptr;
	}
	return nullptr;
}

void Faction::DiscoverItem(int item, int force, int full)
{
	const int seen = items.GetNum(item);
	if (!seen)
	{
		if (full)
		{
			items.SetNum(item, 2);
		}
		else
		{
			items.SetNum(item, 1);
		}
		force = 1;
	}
	else
	{
		if (seen == 1)
		{
			if (full)
			{
				items.SetNum(item, 2);
			}
			force = 1;
		}
		else
		{
			full = 1;
		}
	}

	if (force)
		itemshows.Add(ItemDescription(item, full));
}

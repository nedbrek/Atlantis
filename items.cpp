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
#include "object.h"
#include "gamedata.h"
#include "fileio.h"
#include "astring.h"

const char *const ManType::ALIGN_STRS[ManType::NUM_ALIGN] =
{
	"neutral",
	"evil",
	"good"
};

AString AttType(int atype)
{
	switch (atype)
	{
	case ATTACK_COMBAT:    return "melee";
	case ATTACK_ENERGY:    return "energy";
	case ATTACK_SPIRIT:    return "spirit";
	case ATTACK_WEATHER:   return "weather";
	case ATTACK_RIDING:    return "riding";
	case ATTACK_RANGED:    return "ranged";
	case NUM_ATTACK_TYPES: return "non-resistable";
	}

	return "unknown";
}

static AString DefType(int atype)
{
	return atype == NUM_ATTACK_TYPES ? "all" : AttType(atype);
}

int ParseAllItems(const AString *token)
{
	//foreach item definition
	for (unsigned i = 0; i < ItemDefs.size(); ++i)
	{
		// if the current item is an illusionary version of a monster
		if ((ItemDefs[i].type & IT_MONSTER) &&
		     ItemDefs[i].index == MONSTER_ILLUSION)
		{
			if (*token == (AString("i") + ItemDefs[i].name) ||
			    *token == (AString("i") + ItemDefs[i].names) ||
			    *token == (AString("i") + ItemDefs[i].abr))
			{
				return i;
			}
			continue;
		}

		// normal check
		if (*token == ItemDefs[i].name ||
		    *token == ItemDefs[i].names ||
		    *token == ItemDefs[i].abr)
		{
			return i;
		}
	}

	return -1;
}

int ParseEnabledItem(const AString *token)
{
	for (unsigned i = 0; i < ItemDefs.size(); ++i)
	{
		if (ItemDefs[i].flags & ItemType::DISABLED)
			continue;

		// if the current item is an illusionary version of a monster
		if ((ItemDefs[i].type & IT_MONSTER) &&
		     ItemDefs[i].index == MONSTER_ILLUSION)
		{
			if (*token == (AString("i") + ItemDefs[i].name) ||
			    *token == (AString("i") + ItemDefs[i].names) ||
			    *token == (AString("i") + ItemDefs[i].abr))
			{
				return i;
			}
			continue;
		}

		if (*token == ItemDefs[i].name ||
		    *token == ItemDefs[i].names ||
		    *token == ItemDefs[i].abr)
		{
			return i;
		}
	}

	return -1;
}

int ParseGiveableItem(AString *token)
{
	int r = -1;
	for (unsigned i = 0; i < ItemDefs.size(); ++i)
	{
		if (ItemDefs[i].flags & ItemType::DISABLED)
			continue;

		if (ItemDefs[i].flags & ItemType::CANTGIVE)
			continue;

		if ((ItemDefs[i].type & IT_MONSTER) &&
		     ItemDefs[i].index == MONSTER_ILLUSION)
		{
			if (*token == (AString("i") + ItemDefs[i].name) ||
			    *token == (AString("i") + ItemDefs[i].names) ||
			    *token == (AString("i") + ItemDefs[i].abr))
			{
				return i;
			}
			continue;
		}

		if (*token == ItemDefs[i].name ||
		    *token == ItemDefs[i].names ||
		    *token == ItemDefs[i].abr)
		{
			return i;
		}
	}

	return r;
}

int ParseBattleItem(int item)
{
	for (int i = 0; i < NUMBATTLEITEMS; ++i)
	{
		if (item == BattleItemDefs[i].itemNum)
			return i;
	}
	return -1;
}

AString ItemString(int type, int num)
{
	// use singular
	if (num == 1)
		return AString(ItemDefs[type].name) + " [" + ItemDefs[type].abr + "]";

	// special number
	if (num == -1)
	{
		return AString("unlimited ") + ItemDefs[type].names + " ["
		      + ItemDefs[type].abr + "]";
	}

	return AString(num) + " " + ItemDefs[type].names + " ["
	      + ItemDefs[type].abr + "]";
}

static AString EffectStr(int effect)
{
	AString temp = EffectDefs[effect].name;

	AString temp2;
	bool comma = false;

	if (EffectDefs[effect].attackVal)
	{
		temp2 += AString(EffectDefs[effect].attackVal) + " to attack";
		comma = true;
	}

	for (int i = 0; i < 4; ++i)
	{
		if (EffectDefs[effect].defMods[i].type == -1)
			continue;

		if (comma)
			temp2 += ", ";

		temp2 += AString(EffectDefs[effect].defMods[i].val) + " versus "
		      + DefType(EffectDefs[effect].defMods[i].type) + " attacks";
		comma = true;
	}

	if (comma)
	{
		temp += AString(" (") + temp2 + ")";

		if (EffectDefs[effect].flags & EffectType::EFF_ONESHOT)
		{
			temp += " for their next attack";
		}
		else
		{
			temp += " for the rest of the battle";
		}
	}
	temp += ".";

	if (EffectDefs[effect].cancelEffect != -1)
	{
		if (comma)
			temp += " ";
		temp += AString("This effect cancels out the effects of ")
		      + EffectDefs[EffectDefs[effect].cancelEffect].name + ".";
	}

	return temp;
}

AString ShowSpecial(int special, const int level, const int expandLevel, const int fromItem)
{
	// sanitize input
	if (special < 0 || special > (NUMSPECIALS - 1))
		special = 0;

	const SpecialType *const spd = &SpecialDefs[special];

	AString temp;
	temp += spd->specialname;
	temp += AString(" in battle");

	if (expandLevel)
		temp += AString(" at a skill level of ") + level;

	temp += ".";

	if (fromItem)
		temp += " This ability only affects the possessor of the item.";

	bool comma = false;
	int last = -1;
	int val;

	if ((spd->targflags & SpecialType::HIT_BUILDINGIF) ||
	    (spd->targflags & SpecialType::HIT_BUILDINGEXCEPT))
	{
		temp += " This ability will ";
		if (spd->targflags & SpecialType::HIT_BUILDINGEXCEPT)
		{
			temp += "not ";
		}
		else
		{
			temp += "only ";
		}

		temp += "target units which are inside the following structures: ";
		for (int i = 0; i < 3; ++i)
		{
			if (spd->buildings[i] == -1)
				continue;

			if (ObjectDefs[spd->buildings[i]].flags & ObjectType::DISABLED)
				continue;

			if (last == -1)
			{
				last = i;
				continue;
			}

			temp += AString(ObjectDefs[spd->buildings[last]].name) + ", ";
			last = i;
			comma = true;
		}

		if (comma)
		{
			temp += "or ";
		}
		temp += AString(ObjectDefs[spd->buildings[last]].name) + ".";
	}

	if ((spd->targflags & SpecialType::HIT_SOLDIERIF) ||
	    (spd->targflags & SpecialType::HIT_SOLDIEREXCEPT) ||
	    (spd->targflags & SpecialType::HIT_MOUNTIF) ||
	    (spd->targflags & SpecialType::HIT_MOUNTEXCEPT))
	{
		temp += " This ability will ";

		if ((spd->targflags & SpecialType::HIT_SOLDIEREXCEPT) ||
		    (spd->targflags & SpecialType::HIT_MOUNTEXCEPT))
		{
			temp += "not ";
		}
		else
		{
			temp += "only ";
		}

		temp += "target ";

		if ((spd->targflags & SpecialType::HIT_MOUNTIF) ||
		    (spd->targflags & SpecialType::HIT_MOUNTEXCEPT))
		{
			temp += "units mounted on ";
		}

		comma = false;
		last = -1;

		for (int i = 0; i < 7; ++i)
		{
			if (spd->targets[i] == -1)
				continue;

			if (ItemDefs[spd->targets[i]].flags & ItemType::DISABLED)
				continue;

			if (last == -1)
			{
				last = i;
				continue;
			}

			temp += AString(ItemDefs[spd->targets[last]].names) + " ["
			      + ItemDefs[spd->targets[last]].abr + "], ";

			last = i;
			comma = true;
		}

		if (comma)
		{
			temp += "or ";
		}
		temp += AString(ItemDefs[spd->targets[last]].names) + " ["
		      + ItemDefs[spd->targets[last]].abr + "].";
	}

	if ((spd->targflags & SpecialType::HIT_EFFECTIF) ||
	    (spd->targflags & SpecialType::HIT_EFFECTEXCEPT))
	{
		temp += " This ability will ";

		if (spd->targflags & SpecialType::HIT_EFFECTEXCEPT)
		{
			temp += "not ";
		}
		else
		{
			temp += "only ";
		}

		temp += "target creatures which are currently affected by ";

		for (int i = 0; i < 3; ++i)
		{
			if (spd->effects[i] == -1)
				continue;

			if (last == -1)
			{
				last = i;
				continue;
			}

			temp += AString(EffectDefs[spd->effects[last]].name) + ", ";
			last = i;
			comma = true;
		}

		if (comma)
		{
			temp += "or ";
		}

		temp += AString(EffectDefs[spd->effects[last]].name) + ".";
	}

	if (spd->targflags & SpecialType::HIT_ILLUSION)
	{
		temp += " This ability will only target illusions.";
	}

	if (spd->targflags & SpecialType::HIT_NOMONSTER)
	{
		temp += " This ability cannot target monsters.";
	}

	if (spd->effectflags & SpecialType::FX_NOBUILDING)
	{
		temp += AString(" The bonus given to units inside buildings is ")
		      + "not effective against this ability.";
	}

	if (spd->effectflags & SpecialType::FX_SHIELD)
	{
		temp += " This ability provides a shield against all ";

		comma = false;
		last = -1;

		for (int i = 0; i < 4; ++i)
		{
			if (spd->shield[i] == -1)
				continue;

			if (last == -1)
			{
				last = i;
				continue;
			}

			temp += DefType(spd->shield[last]) + ", ";
			last = i;
			comma = true;
		}

		if (comma)
		{
			temp += "and ";
		}
		temp += DefType(spd->shield[last]) + " attacks against the entire"
		      + " army at a level equal to the skill level of the ability.";
	}

	if (spd->effectflags & SpecialType::FX_DEFBONUS)
	{
		temp += " This ability provides ";

		comma = false;
		last = -1;

		for (int i = 0; i < 4; ++i)
		{
			if (spd->defs[i].type == -1)
				continue;

			if (last == -1)
			{
				last = i;
				continue;
			}

			val = spd->defs[last].val;
			if (expandLevel)
			{
				if (spd->effectflags & SpecialType::FX_USE_LEV)
					val *= level;
			}

			temp += AString("a defensive bonus of ") + val;
			if (!expandLevel)
			{
				temp += " per skill level";
			}
			temp += AString(" versus ") + DefType(spd->defs[last].type)
			      + " attacks, ";
			last = i;
			comma = true;
		}

		if (comma)
		{
			temp += "and ";
		}

		val = spd->defs[last].val;
		if (expandLevel)
		{
			if (spd->effectflags & SpecialType::FX_USE_LEV)
				val *= level;
		}

		temp += AString("a defensive bonus of ") + val;

		if (!expandLevel)
		{
			temp += " per skill level";
		}

		temp += AString(" versus ") + DefType(spd->defs[last].type) + " attacks";
		temp += " to the casting mage.";
	}

	// now the damages
	for (int i = 0; i < 4; ++i)
	{
		if (spd->damage[i].type == -1)
			continue;

		temp += AString(" This ability does between ") + spd->damage[i].minnum
		      + " and ";

		val = spd->damage[i].value * 2;

		if (expandLevel)
		{
			if (spd->effectflags & SpecialType::FX_USE_LEV)
				val *= level;
		}

		temp += AString(val);
		if (!expandLevel)
		{
			temp += " times the skill level of the mage";
		}
		temp += AString(" ") + AttType(spd->damage[i].type) + " attacks.";
		if (spd->damage[i].effect)
		{
			temp += " Each attack causes the target to be effected by ";
			temp += EffectStr(spd->damage[i].effect);
		}
	}

	return temp;
}

bool IsSoldier(int item)
{
	return (ItemDefs[item].type & IT_MAN) || (ItemDefs[item].type & IT_MONSTER);
}

//----------------------------------------------------------------------------
Item::Item(int t, int n)
: type(t)
, num(n)
, selling(0)
{
}

AString Item::Report(int see_illusions) const
{
	AString ret = ItemString(type, num);
	if (see_illusions && (ItemDefs[type].type & IT_MONSTER) &&
	    ItemDefs[type].index == MONSTER_ILLUSION)
	{
		ret += " (illusion)";
	}
	return ret;
}

void Item::Writeout(Aoutfile *f) const
{
	f->PutInt(type);
	f->PutInt(num);
}

void Item::Readin(Ainfile *f)
{
	type = f->GetInt();
	num = f->GetInt();
}

//----------------------------------------------------------------------------
void ItemList::Writeout(Aoutfile *f)
{
	f->PutInt(Num());
	forlist(this)
		((Item*)elem)->Writeout(f);
}

void ItemList::Readin(Ainfile *f)
{
	const int i = f->GetInt();
	for (int j = 0; j < i; ++j)
	{
		Item *temp = new Item;
		temp->Readin(f);
		if (temp->num < 1)
		{
			delete temp;
		}
		else
		{
			Add(temp);
		}
	}
}

int ItemList::GetNum(int t)
{
	forlist(this)
	{
		Item *i = (Item*)elem;
		if (i->type == t)
			return i->num;
	}
	return 0;
}

int ItemList::Weight()
{
	int wt = 0;
	forlist(this)
	{
		Item *i = (Item*)elem;
		wt += ItemDefs[i->type].weight * i->num;
	}
	return wt;
}

int ItemList::CanSell(int t)
{
	forlist(this)
	{
		Item *i = (Item*)elem;
		if (i->type == t)
			return i->num - i->selling;
	}
	return 0;
}

void ItemList::Selling(int t, int n)
{
	forlist(this)
	{
		Item *i = (Item*)elem;
		if (i->type == t)
			i->selling += n;
	}
}

AString ItemList::Report(const int obs, const int seeillusions, int nofirstcomma)
{
	AString temp;
	forlist(this)
	{
		const Item *const i = (Item*)elem;

		// if we can't see everything and item is not bulky
		if (obs != 2 && ItemDefs[i->type].weight == 0)
			continue;

		if (nofirstcomma)
			nofirstcomma = 0;
		else
			temp += ", ";

		temp += i->Report(seeillusions);
	}

	return temp;
}

AString ItemList::BattleReport()
{
	AString temp;
	forlist(this)
	{
		const Item *const i = (Item*)elem;
		if (!ItemDefs[i->type].combat)
			continue;

		temp += ", ";
		temp += i->Report(0);

		if (ItemDefs[i->type].type & IT_MONSTER)
		{
			const MonType &mondef = MonDefs[ItemDefs[i->type].index];
			temp += AString(" (Combat ") + mondef.attackLevel + "/"
			      + mondef.defense[ATTACK_COMBAT] + ", Attacks "
			      + mondef.numAttacks + ", Hits " + mondef.hits + ", Tactics "
			      + mondef.tactics + ")";
		}
	}

	return temp;
}

void ItemList::SetNum(int t, int n)
{
	// if item is going away
	if (n == 0)
	{
		forlist(this)
		{
			Item *i = (Item*)elem;
			if (i->type == t)
			{
				Remove(i);
				delete i;
				return;
			}
		}
		return;
	}

	// check existing items for match
	forlist(this)
	{
		Item *i = (Item*)elem;
		if (i->type == t)
		{
			// found it
			i->num = n;
			return;
		}
	}

	//else add an item
	Add(new Item(t, n));
}

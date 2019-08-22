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
#include "unit.h"
#include "aregion.h"
#include "faction.h"
#include "gamedata.h"
#include "gameio.h"
#include "object.h"
#include "orders.h"
#include <cstdlib>
#include <map>

//----------------------------------------------------------------------------
UnitId::UnitId(bool)
: unitnum(-1) // nobody
, alias  (0)
, faction(0)
{
}

UnitId::UnitId(int unit_num, int alias, int faction, bool r)
: unitnum(unit_num)
, alias  (alias)
, faction(faction)
, rename(r)
{
	if (unit_num < 0)
	{
		std::cerr << "UnitId: Unit num must greater than 0" << std::endl;
		exit(1);
	}
	if (unit_num != 0 && alias != 0)
	{
		std::cerr << "UnitId: Unit num must be 0 for alias to work" << std::endl;
		exit(1);
	}
	else if (unit_num == 0 && alias == 0)
	{
		std::cerr << "UnitId: Unit num must be non-zero when no alias given" << std::endl;
		exit(1);
	}
}

UnitId::~UnitId()
{
}

AString UnitId::Print(ARegion *region, int fac_num) const
{
	if (unitnum)
		return AString(unitnum);

	if (faction)
	{
		return AString("faction ") + AString(faction) + " new " +
		   AString(alias);
	}

	if (region && rename)
	{
		Unit *u = region->GetUnitId(this, fac_num);
		if (u)
			return AString(u->num);
	}

	return AString("new ") + AString(alias);
}

Unit* UnitId::find(AList &l, int f) const
{
	if (unitnum == -1)
		return NULL;

	if (unitnum > 0)
	{
		return Unit::findByNum(l, unitnum);
	}

	if (faction)
	{
		return Unit::findByFaction(l, alias, faction);
	}

	return Unit::findByFaction(l, alias, f);
}

UnitPtr* GetUnitList(AList *list, Unit *u)
{
	forlist(list)
	{
		UnitPtr *p = (UnitPtr*)elem;
		if (p->ptr == u)
			return p;
	}
	return NULL;
}

//----------------------------------------------------------------------------
Unit::Unit(int seq, Faction *f, int a)
{
	faction = f;
	formfaction = f;
	object = NULL;
	name = new AString;
	*name = AString("Unit (") + seq + ")";
	describe = NULL;
	num = seq;
	type = U_NORMAL;
	alias = a;
	gm_alias = 0;
	guard = GUARD_NONE;
	reveal = REVEAL_NONE;
	flags = 0;
	taxing = TAX_NONE;
	movepoints = 0;
	canattack = 1;
	nomove = 0;
	combat = -1;
	readyItem = -1;

	for(int i = 0; i < MAX_READY; ++i)
	{
		readyWeapon[i] = -1;
		readyArmor[i] = -1;
	}

	needed = 0;
	hunger = 0;
	stomach_space = 0;
	losses = 0;
	free = 0;
	practised = 0;
	destroy = 0;
	enter = 0;
	build = NULL;
	leftShip = 0;
	promote = NULL;
	castorders = NULL;
	teleportorders = NULL;
	stealorders = NULL;
	monthorders = NULL;
	attackorders = NULL;
	evictorders = NULL;
	advancefrom = NULL;
	inTurnBlock = 0;
	presentMonthOrders = NULL;
	presentTaxing = 0;
	former = NULL;

	ClearOrders();
}

Unit::~Unit()
{
	delete monthorders;
	delete presentMonthOrders;
	delete attackorders;
	delete stealorders;
	delete name;
	delete describe;
}

Unit* Unit::findByNum(AList &l, int num)
{
	forlist((&l))
		if (((Unit*)elem)->num == num)
			return (Unit*)elem;

	return NULL;
}

Unit* Unit::findByFaction(AList &l, int alias, int faction)
{
	// First search for units with the 'formfaction'
	forlist((&l))
	{
		if (((Unit*)elem)->alias == alias &&
		    ((Unit*)elem)->formfaction->num == faction)
			return (Unit*)elem;
	}

	// Now search against their current faction
	{
		forlist((&l))
		{
			if (((Unit*)elem)->alias == alias &&
			    ((Unit*)elem)->faction->num == faction)
				return (Unit*)elem;
		}
	}

	return NULL;
}

void Unit::SetMonFlags()
{
	guard = GUARD_AVOID;
	SetFlag(FLAG_HOLDING, 1);
}

void Unit::MakeWMon(const char *monname, int mon, int num)
{
	SetName(new AString(monname));

	type = U_WMON;
	items.SetNum(mon, num);
	SetMonFlags();
	if (Globals->WMONSTER_SPOILS_RECOVERY)
		free = Globals->MONSTER_NO_SPOILS + Globals->MONSTER_SPOILS_RECOVERY;
}

void Unit::Writeout(Aoutfile *s)
{
	s->PutStr(*name);
	if (describe)
		s->PutStr(*describe);
	else
		s->PutStr("none");

	s->PutInt(num);
	s->PutInt(type);
	s->PutInt(faction->num);
	s->PutInt(guard);
	s->PutInt(reveal);
	s->PutInt(-3);
	s->PutInt(free);
	s->PutInt(readyItem);

	for(int i = 0; i < MAX_READY; ++i) {
		s->PutInt(readyWeapon[i]);
		s->PutInt(readyArmor[i]);
	}

	s->PutInt(flags);
	items.Writeout(s);
	skills.Writeout(s);
	s->PutInt(combat);
}

void Unit::Readin(Ainfile *s, AList *facs, ATL_VER v)
{
	name = s->GetStr();
	describe = s->GetStr();
	if (*describe == "none")
	{
		delete describe;
		describe = NULL;
	}

	num = s->GetInt();
	type = s->GetInt();

	int i = s->GetInt();
	faction = GetFaction(facs, i);

	guard = s->GetInt();
	if (guard == GUARD_ADVANCE) guard = GUARD_NONE;
	if (guard == GUARD_SET) guard = GUARD_GUARD;

	reveal = s->GetInt();

	// Handle the new 'ready item', ready weapons/armor, and free
	free = 0;
	readyItem = -1;
	for (int i = 0; i < MAX_READY; ++i)
	{
		readyWeapon[i] = -1;
		readyArmor [i] = -1;
	}

	// Negative indicates extra values inserted vs older games
	i = s->GetInt();
	switch (i)
	{
		default: // ie >= 0
			flags = i;
			break;

		case -1:
			readyItem = s->GetInt();
			flags = s->GetInt();
			break;

		case -2:
			readyItem = s->GetInt();
			for (i = 0; i < MAX_READY; ++i) {
				readyWeapon[i] = s->GetInt();
				readyArmor[i] = s->GetInt();
			}
			flags = s->GetInt();
			break;

		case -3:
			free = s->GetInt();
			readyItem = s->GetInt();
			for (i = 0; i < MAX_READY; ++i) {
				readyWeapon[i] = s->GetInt();
				readyArmor[i] = s->GetInt();
			}
			flags = s->GetInt();
			break;
	}

	items.Readin(s);
	skills.Readin(s);
	combat = s->GetInt();
}

AString Unit::MageReport() const
{
	if (combat != -1)
		return AString(". Combat spell: ") + SkillStrs(combat);

	return AString();
}

AString Unit::ReadyItem() const
{
	AString temp; // return value
	int i;

	int item = 0;
	AString weaponstr;
	for (i = 0; i < MAX_READY; ++i)
	{
		const int ready = readyWeapon[i];
		if (ready != -1)
		{
			if (item) weaponstr += ", ";
			weaponstr += ItemString(ready, 1);
			++item;
		}
	}

	if (item)
	{
		temp += AString(". Ready weapon");
		if (item > 1)
			temp += "s";
		temp += AString(": ") + weaponstr;
	}

	item = 0;

	AString armorstr;
	for (i = 0; i < MAX_READY; ++i)
	{
		const int ready = readyArmor[i];
		if (ready != -1)
		{
			if (item) armorstr += ", ";
			armorstr += ItemString(ready, 1);
			++item;
		}
	}

	if (item > 0)
		armorstr = AString("Ready armor: ") + armorstr;

	const int armor = item;
	item = 0;

	if (armor)
	{
		temp += ". ";
		temp += armorstr;
	}

	// if orders include a ready item
	if (readyItem != -1)
	{
		temp += ". ";
		temp += AString("Ready item: ") + ItemString(readyItem, 1);
	}

	return temp;
}

AString Unit::StudyableSkills()
{
	AString temp;
	bool j = false;

	for (int sk = 0; sk < NSKILLS; ++sk)
	{
		// Magic skills without depencies should not be skipped
		if (SkillDefs[sk].depends[0].skill == -1 && !(SkillDefs[sk].flags & SkillType::MAGIC))
			continue;

		// CanStudy gets prereqs, does not check max level
		if (!CanStudy(sk))
			continue;

		// Check for maximum skill level for magic foundation skills
		if (SkillDefs[sk].flags & SkillType::FOUNDATION) {
			// This is only used with mages to display studiable magic skills
			if (type != U_MAGE)
				return temp;

			// Compare current skill level to maximum level
			if (GetRealSkill(sk) >= SkillMax(sk))
				continue;
		}

		if (j)
		{
			temp += ", ";
		}
		else
		{
			temp += ". Can Study: ";
			j = true;
		}

		temp += SkillStrs(sk);
	}

	return temp;
}

AString Unit::GetName(int obs)
{
	AString ret = *name;

	// if faction can be seen
	if (reveal == REVEAL_FACTION || obs > GetSkill(S_STEALTH))
	{
		ret += ", ";
		ret += *faction->name;
	}

	return ret;
}

int Unit::CanGetSpoil(const Item *i)
{
	if (!i)
		return 0;

	const int weight = ItemDefs[i->type].weight;
	if (!weight)
		return 1; // any unit can carry 0 weight spoils

	// unit is avoiding spoils
	if (flags & FLAG_NOSPOILS) return 0;

	// check spoils flags
	// (only pick up items with their own carrying capacity)
	const int fly  = ItemDefs[i->type].fly;
	const int ride = ItemDefs[i->type].ride;
	const int walk = ItemDefs[i->type].walk;

	if ((flags & FLAG_FLYSPOILS ) && fly  < weight) return 0; // only flying
	if ((flags & FLAG_WALKSPOILS) && walk < weight) return 0; // only walking
	if ((flags & FLAG_RIDESPOILS) && ride < weight) return 0; // only riding

	return 1; // all spoils
}

AString Unit::SpoilsReport()
{
	AString temp;

	if (GetFlag(FLAG_NOSPOILS)) temp = ", weightless battle spoils";
	else if (GetFlag(FLAG_FLYSPOILS)) temp = ", flying battle spoils";
	else if (GetFlag(FLAG_WALKSPOILS)) temp = ", walking battle spoils";
	else if (GetFlag(FLAG_RIDESPOILS)) temp = ", riding battle spoils";

	return temp;
}

void Unit::WriteReport(Areport *f, int obs, int truesight, int detfac, int autosee)
{
	int stealth = GetSkill(S_STEALTH);
	if (obs == -1)
	{
		// self report
		obs = 2;
	}
	else
	{
		if (obs < stealth)
		{
			// cannot be seen
			if (reveal == REVEAL_FACTION)
			{
				obs = 1;
			}
			else
			{
				if (guard != GUARD_GUARD && reveal != REVEAL_UNIT && !autosee)
					return;

				obs = 0;
			}
		}
		else
		{
			if (obs == stealth)
			{
				// can see unit, but not Faction
				if (reveal == REVEAL_FACTION)
				{
					obs = 1;
				}
				else
				{
					obs = 0;
				}
			}
			else
			{
				// can see unit and Faction
				obs = 1;
			}
		}
	}

	// Setup True Sight
	if (obs == 2)
	{
		truesight = 1;
	}
	else
	{
		if (GetSkill(S_ILLUSION) > truesight)
		{
			truesight = 0;
		}
		else
		{
			truesight = 1;
		}
	}

	if (detfac && obs != 2)
		obs = 1;

	// Write the report
	AString temp;
	if (obs == 2)
		temp += AString("* ");
	else
		temp += AString("- ");

	temp += *name;

	if (guard == GUARD_GUARD)
		temp += ", on guard";

	if (obs > 0)
	{
		temp += AString(", ") + *faction->name;
		if (guard == GUARD_AVOID) temp += ", avoiding";
		if (GetFlag(FLAG_BEHIND)) temp += ", behind";
	}

	if (obs == 2)
	{
		if (reveal == REVEAL_UNIT) temp += ", revealing unit";
		if (reveal == REVEAL_FACTION) temp += ", revealing faction";
		if (GetFlag(FLAG_HOLDING)) temp += ", holding";
		if (GetFlag(FLAG_AUTOTAX)) temp += ", taxing";
		if (GetFlag(FLAG_NOAID)) temp += ", receiving no aid";
		if (GetFlag(FLAG_CONSUMING_UNIT)) temp += ", consuming unit's food";
		if (GetFlag(FLAG_CONSUMING_FACTION))
			temp += ", consuming faction's food";
		if (GetFlag(FLAG_NOCROSS_WATER)) temp += ", won't cross water";
		if (GetFlag(FLAG_SHARE)) temp += ", sharing";
		temp += SpoilsReport();
	}

	temp += items.Report(obs, truesight, 0);

	if (obs == 2)
	{
		temp += ". Weight: ";
		temp += AString(items.Weight());
		temp += ". Capacity: ";
		temp += AString(FlyingCapacity());
		temp += "/";
		temp += AString(RidingCapacity());
		temp += "/";
		temp += AString(WalkingCapacity());
		temp += "/";
		temp += AString(SwimmingCapacity());
		temp += ". Upkeep: $";
		temp += AString(MaintCost());
		temp += ". Skills: ";
		temp += skills.Report(GetMen());
	}

	if (obs == 2 && (type == U_MAGE || type == U_GUARDMAGE))
	{
		temp += MageReport();
	}

	if (obs == 2)
	{
		temp += ReadyItem();
		temp += StudyableSkills();
	}

	if (describe)
	{
		temp += AString("; ") + *describe;
	}

	temp += ".";
	f->PutStr(temp);
}

AString Unit::TemplateReport()
{
	// write the report
	AString temp = *name;

	if (guard == GUARD_GUARD) temp += ", on guard";
	if (guard == GUARD_AVOID) temp += ", avoiding";
	if (GetFlag(FLAG_BEHIND)) temp += ", behind";
	if (reveal == REVEAL_UNIT) temp += ", revealing unit";
	if (reveal == REVEAL_FACTION) temp += ", revealing faction";
	if (GetFlag(FLAG_HOLDING)) temp += ", holding";
	if (GetFlag(FLAG_AUTOTAX)) temp += ", taxing";
	if (GetFlag(FLAG_NOAID)) temp += ", receiving no aid";
	if (GetFlag(FLAG_CONSUMING_UNIT)) temp += ", consuming unit's food";
	if (GetFlag(FLAG_CONSUMING_FACTION)) temp += ", consuming faction's food";
	if (GetFlag(FLAG_NOCROSS_WATER)) temp += ", won't cross water";
	if (GetFlag(FLAG_SHARE)) temp += ", sharing";
	temp += SpoilsReport();

	temp += items.Report(2, 1, 0);
	temp += ". Weight: ";
	temp += AString(items.Weight());
	temp += ". Capacity: ";
	temp += AString(FlyingCapacity());
	temp += "/";
	temp += AString(RidingCapacity());
	temp += "/";
	temp += AString(WalkingCapacity());
	temp += "/";
	temp += AString(SwimmingCapacity());
	temp += ". Upkeep: $";
	temp += AString(MaintCost());
	temp += ". Skills: ";
	temp += skills.Report(GetMen());

	if (type == U_MAGE || type == U_GUARDMAGE)
	{
		temp += MageReport();
	}
	temp += ReadyItem();
	temp += StudyableSkills();

	if (describe)
		temp += AString("; ") + *describe;

	temp += ".";
	return temp;
}

AString* Unit::BattleReport(int obs)
{
  AString *temp = new AString;

  // if battle might reveal faction
  if (Globals->BATTLE_FACTION_INFO)
	  *temp += GetName(obs);
  else
	  *temp += *name;

  if (GetFlag(FLAG_BEHIND)) *temp += ", behind";

  *temp += items.BattleReport();

  int lvl = GetRealSkill(S_TACTICS);
  if (lvl)
  {
		*temp += ", ";
		*temp += SkillDefs[S_TACTICS].name;
		*temp += " ";
		*temp += lvl;
  }

  lvl = GetRealSkill(S_COMBAT);
  if (lvl)
  {
		*temp += ", ";
		*temp += SkillDefs[S_COMBAT].name;
		*temp += " ";
		*temp += lvl;
  }

  lvl = GetRealSkill(S_LONGBOW);
  if (lvl)
  {
		*temp += ", ";
		*temp += SkillDefs[S_LONGBOW].name;
		*temp += " ";
		*temp += lvl;
  }

  lvl = GetRealSkill(S_CROSSBOW);
  if (lvl)
  {
		*temp += ", ";
		*temp += SkillDefs[S_CROSSBOW].name;
		*temp += " ";
		*temp += lvl;
  }

  lvl = GetRealSkill(S_RIDING);
  if (lvl)
  {
		*temp += ", ";
		*temp += SkillDefs[S_RIDING].name;
		*temp += " ";
		*temp += lvl;
  }

  if (describe)
  {
		*temp += "; ";
		*temp += *describe;
  }

  *temp += ".";
  return temp;
}

void Unit::ClearOrders()
{
	taxing = TAX_NONE;
	movepoints = 0;
	canattack = 1;
	nomove = 0;
	destroy = 0;
	enter = 0;
	build = NULL;
	leftShip = 0;
	promote = NULL;
	delete castorders; castorders = NULL;
	delete teleportorders; teleportorders = NULL;
	delete stealorders; stealorders = NULL;
	delete monthorders; monthorders = NULL;
	delete attackorders; attackorders = NULL;
	delete evictorders; evictorders = NULL;
	advancefrom = NULL;
	inTurnBlock = 0;
	delete presentMonthOrders; presentMonthOrders = NULL;
	presentTaxing = 0;
}

void Unit::ClearCastOrders()
{
	delete castorders; castorders = NULL;
	delete teleportorders; teleportorders = NULL;
}

void Unit::DefaultOrders(Object *obj)
{
	ClearOrders();

	if (type == U_WMON)
	{
		// monster orders
		if (ObjectDefs[obj->type].monster == -1)
		{
			MoveOrder *o = new MoveOrder;

			// monsters randomly "advance" (move aggressively)
			int aper = Hostile();
			aper *= Globals->MONSTER_ADVANCE_HOSTILE_PERCENT;
			aper /= 100;

			if (aper < Globals->MONSTER_ADVANCE_MIN_PERCENT)
				aper = Globals->MONSTER_ADVANCE_MIN_PERCENT;

			const int n = getrandom(100);
			o->advancing = (n < aper) ? 1 : 0;

			MoveDir *d = new MoveDir;
			d->dir = getrandom(NDIRS);
			o->dirs.Add(d);
			monthorders = o;
		}
	}
	else if (type == U_GUARD)
	{
		if (guard != GUARD_GUARD)
			guard = GUARD_SET;
	}
	else if (type == U_GUARDMAGE)
	{
		combat = S_FIRE;
	}
	else
	{
		// Set up default orders for any faction unit which submits none
		if (obj->region->type != R_NEXUS)
		{
			if (GetFlag(FLAG_AUTOTAX) &&
			    Globals->TAX_PILLAGE_MONTH_LONG && Taxers())
			{
				taxing = TAX_AUTO;
			}
			else
			{
				ProduceOrder *order = new ProduceOrder;
				order->skill = -1;
				order->item = I_SILVER;
				monthorders = order;
			}
		}
	}
}

void Unit::PostTurn(ARegion *r)
{
	if (type == U_WMON)
	{
		// remove items from monsters
		forlist(&items)
		{
			Item *i = (Item*)elem;
			if (!(ItemDefs[i->type].type & IT_MONSTER))
			{
				items.Remove(i);
				delete i;
			}
		}

		if (free > 0)
			--free;
	}
}

void Unit::SetName(AString *s)
{
	if (!s)
		return;

	AString *newname = s->getlegal();
	delete s;

	if (!newname)
		return;

	*newname += AString(" (") + num + ")";

	delete name;
	name = newname;
}

void Unit::SetDescribe(AString *s)
{
	delete describe;
	if (s)
	{
		AString *newname = s->getlegal();
		delete s;
		describe = newname;
	}
	else
		describe = NULL;
}

int Unit::IsAlive()
{
	if (type == U_MAGE || type == U_APPRENTICE)
		return GetMen() ? 1 : 0;

	forlist(&items)
	{
		Item *i = (Item*)elem;

		if (IsSoldier(i->type) && i->num > 0)
			return 1;
	}

	return 0;
}

void Unit::SetMen(int t, int n)
{
	if (ItemDefs[t].type & IT_MAN)
	{
		// adjust skills for loss
		const int oldmen = GetMen();
		items.SetNum(t, n);

		const int newmen = GetMen();
		if (newmen < oldmen)
		{
			delete skills.Split(oldmen, oldmen - newmen);
		}
	}
	else
	{
		// probably a monster
		items.SetNum(t, n);
	}
}

int Unit::GetMons()
{
	int n = 0;
	forlist(&items)
	{
		const Item *i = (const Item*)elem;

		if (ItemDefs[i->type].type & IT_MONSTER)
			n += i->num;
	}

	return n;
}

int Unit::GetMen(int t)
{
	return items.GetNum(t);
}

int Unit::GetMen()
{
	int n = 0;
	forlist(&items)
	{
		const Item *i = (const Item*)elem;

		if (ItemDefs[i->type].type & IT_MAN)
			n += i->num;
	}

	return n;
}

int Unit::GetSoldiers()
{
	int n = 0;
	forlist(&items)
	{
		const Item *i = (const Item*)elem;

		if (IsSoldier(i->type))
			n += i->num;
	}

	return n;
}

void Unit::SetMoney(int n)
{
	items.SetNum(I_SILVER, n);
}

int Unit::GetMoney()
{
	return items.GetNum(I_SILVER);
}

int Unit::canConsume(int itemId, int hint)
{
	int num_items = 0;
	//foreach object in parent region
	//(units in buildings can borrow from those in the field)
	forlist(&object->region->objects)
	{
		Object *o = (Object*)elem;
		//foreach unit in the object
		forlist(&o->units)
		{
			Unit *u = (Unit*)elem;
			if (u != this && (u->faction != faction || !u->GetFlag(FLAG_SHARE)))
				continue;

			num_items += u->items.GetNum(itemId);

			// early out
			if (num_items >= hint)
				return num_items;
		}
	}

	return num_items;;
}

void Unit::consume(int itemId, int num)
{
	if (num < 0)
	{
		std::cerr << "Error in logic, tried to consume negative quantity" << std::endl;
		return;
	}

	// see how many we have
	const int num_items = items.GetNum(itemId);

	// if we have enough
	if (num_items >= num)
	{
		items.SetNum(itemId, num_items - num);
		return; // done
	}
	//else use all we have
	items.SetNum(itemId, 0);
	num -= num_items;

	//foreach object in parent region
	//(units in buildings can borrow from those in the field)
	forlist(&object->region->objects)
	{
		Object *o = (Object*)elem;
		//foreach unit in the object
		forlist(&o->units)
		{
			Unit *u = (Unit*)elem;
			if (u == this || u->faction != faction || !u->GetFlag(FLAG_SHARE))
				continue;

			const int num_items = u->items.GetNum(itemId);
			if (num_items >= num)
			{
				u->items.SetNum(itemId, num_items - num);
				return;
			}

			u->items.SetNum(itemId, 0);
			num -= num_items;
		}
	}
	if (num > 0)
	{
		std::cerr << "Error in logic: consumed more than was available" << std::endl;
	}
}

int Unit::GetTactics()
{
	int retval = GetRealSkill(S_TACTICS);

	forlist(&items)
	{
		Item *i = (Item*)elem;
		if (ItemDefs[i->type].type & IT_MONSTER)
		{
			const int temp = MonDefs[(ItemDefs[i->type].index)].tactics;
			if (temp > retval)
				retval = temp;
		}
	}

	return retval;
}

int Unit::GetObservation()
{
	// pull observation skill, with bonuses
	int retval = GetRealSkill(S_OBSERVATION) + GetSkillBonus(S_OBSERVATION);

	// max against any integrated monsters with OBS
	forlist(&items)
	{
		Item *i = (Item*)elem;

		if (ItemDefs[i->type].type & IT_MONSTER)
		{
			const int temp = MonDefs[ItemDefs[i->type].index].obs;
			if (temp > retval)
				retval = temp;
		}
	}

	return retval;
}

int Unit::GetAttackRiding()
{
	if (type == U_WMON)
	{
		int riding = 0;
		forlist(&items)
		{
			Item *i = (Item*)elem;

			if (ItemDefs[i->type].type & IT_MONSTER)
			{
				if (ItemDefs[i->type].fly)
				{
					return 5; // max possible
				}

				if (ItemDefs[i->type].ride)
					riding = 3; // might be a 5
			}
		}

		return riding;
	}

	const int riding = GetSkill(S_RIDING);
	int lowriding = 0;

	//foreach item
	forlist(&items)
	{
		Item *i = (Item*)elem;

		// if item allows a man to be flying
		// XXX -- Fix this -- not all men weigh the same
		// XXX --			 Use the least weight man in the unit
		if (ItemDefs[i->type].fly - ItemDefs[i->type].weight >= 10)
			return riding;

		// if item allows a man to be riding
		// XXX -- Fix this -- Should also be able to carry the man
		if (ItemDefs[i->type].ride - ItemDefs[i->type].weight >= 10)
		{
			if (riding <= 3)
				return riding;

			lowriding = 3;
		}
	}

	return lowriding;
}

int Unit::GetDefenseRiding()
{
	// guards are easy to confront
	if (guard == GUARD_GUARD)
		return 0;

	int riding = 0; // what are mounts capable of
	const int weight = Weight();

	if (CanFly(weight))
	{
		riding = 5;
	}
	else if (CanRide(weight))
	{
		riding = 3;
	}

	// now check for riding skill to make use of capability
	if (GetMen())
	{
		const int manriding = GetSkill(S_RIDING);
		if (manriding < riding)
			return manriding;
	}

	return riding;
}

int Unit::GetStealth()
{
	// units on guard aren't hiding
	if (guard == GUARD_GUARD)
		return 0;

	int monstealth = 100; // lowest monster stealth
	int manstealth = 100; // stealth skill (if any men)

	//foreach item in unit
	forlist(&items)
	{
		Item *i = (Item*)elem;
		if (ItemDefs[i->type].type & IT_MONSTER)
		{
			const int temp = MonDefs[ItemDefs[i->type].index].stealth;
			if (temp < monstealth) monstealth = temp;
		}
		else if (ItemDefs[i->type].type & IT_MAN)
		{
			if (manstealth == 100)
				manstealth = GetRealSkill(S_STEALTH);
		}
	}

	manstealth += GetSkillBonus(S_STEALTH);

	// return min
	if (monstealth < manstealth)
		return monstealth;

	return manstealth;
}

int Unit::GetEntertainment()
{
	const int level  =     GetRealSkill(S_ENTERTAINMENT);
	const int level2 = 5 * GetRealSkill(S_PHANTASMAL_ENTERTAINMENT);

	// return max
	return level > level2 ? level : level2;
}

int Unit::GetSkill(int sk)
{
	if (sk == S_TACTICS)       return GetTactics();
	if (sk == S_STEALTH)       return GetStealth();
	if (sk == S_OBSERVATION)   return GetObservation();
	if (sk == S_ENTERTAINMENT) return GetEntertainment();

	return GetRealSkill(sk);
}

void Unit::SetSkill(int sk, int level)
{
	skills.SetDays(sk, GetDaysByLevel(level) * GetMen());
}

int Unit::GetRealSkill(int sk)
{
	const int numMen = GetMen();
	if (!numMen)
		return 0;

	return GetLevelByDays(skills.GetDays(sk) / numMen);
}

void Unit::ForgetSkill(int sk)
{
	skills.SetDays(sk, 0);

	// check transition from mage to normal
	if (type == U_MAGE)
	{
		//foreach skill, look for a magic skill
		forlist(&skills)
		{
			Skill *s = (Skill*)elem;

			// found one
			if (SkillDefs[s->type].flags & SkillType::MAGIC)
				return; // still a mage
		}

		type = U_NORMAL;
	}

	// check transition from apprentice to normal
	if (type == U_APPRENTICE)
	{
		forlist(&skills)
		{
			Skill *s = (Skill*)elem;

			if (SkillDefs[s->type].flags & SkillType::APPRENTICE)
				return; // still have one
		}

		type = U_NORMAL;
	}
}

int Unit::CheckDepend(int lev, const SkillDepend &dep)
{
	const int temp = GetRealSkill(dep.skill);
	if (temp < dep.level) return 0;

	if (lev >= temp) return 0;

	return 1;
}

int Unit::CanStudy(int sk)
{
	if (SkillDefs[sk].flags & SkillType::DISABLED)
		return 0;

	const int curlev = GetRealSkill(sk);

	// check dependencies
	for (unsigned c = 0; c < sizeof(SkillDefs[sk].depends)/sizeof(SkillDepend); ++c)
	{
		const int skillIdx = SkillDefs[sk].depends[c].skill;

		if (skillIdx == -1)
			return 1; // end of list, success

		if (SkillDefs[skillIdx].flags & SkillType::DISABLED)
			continue; // can't depend on disabled skills

		if (!CheckDepend(curlev, SkillDefs[sk].depends[c]))
			return 0;
	}

	return 1;
}

// Gets maximum skill level for unit based on all man races
int Unit::SkillMax(int sk)
{
	// find max skill level
	int max = 1000;
	forlist (&items)
	{
		Item *i = (Item*)elem;

		if (ItemDefs[i->type].type & IT_MAN)
		{
			const int m = ::SkillMax(sk, i->type);
			if (m < max) max = m;
		}
	}

	return max;
}

int Unit::Study(int sk, int days)
{
	// count non-magic skills and find if this boosting an existing skill
	Skill *existing_skill = NULL;
	int num_nonmagic_skills = 0;
	forlist(&skills)
	{
		Skill *s = (Skill*)elem;
		if (s->type == sk)
			existing_skill = s;

		if ((SkillDefs[s->type].flags & SkillType::MAGIC) == 0)
			++num_nonmagic_skills;
	}

	// if non-leaders are limited, and this is not a leader
	// and we are not improving one of our skills
	if (Globals->SKILL_LIMIT_NONLEADERS && !IsLeader() && !existing_skill)
	{
		// pull man type
		int first_man_type = -1;
		forlist (&items)
		{
			Item *i = (Item*)elem;

			if (ItemDefs[i->type].type & IT_MAN)
			{
				first_man_type = ItemDefs[i->type].index;
				break;
			}
		}

		// count 'magic' as one skill
		const int total_skills = num_nonmagic_skills + ((type == U_MAGE) ? 1 : 0);
		if (total_skills >= ManDefs[first_man_type].max_skills)
		{
			// check for non-leader mages and multiple magic skills
			if (!Globals->MAGE_NONLEADERS)
			{
				Error("STUDY: Cannot learn a new skill.");
				return 0;
			}

			//else, if this is not a magic skill
			if ((SkillDefs[sk].flags & SkillType::MAGIC) == 0)
			{
				Error("STUDY: Cannot learn a new skill.");
				return 0;
			}
		}
	}

	{
		if (GetRealSkill(sk) >= SkillMax(sk))
		{
			Error("STUDY: Maximum level for skill reached.");
			return 0;
		}
	}

	if (!CanStudy(sk))
	{
		Error("STUDY: Doesn't have the pre-requisite skills to study that.");
		return 0;
	}

	skills.SetDays(sk, skills.GetDays(sk) + days);
	AdjustSkills();

	// check to see if we need to show a skill report
	const int lvl = GetRealSkill(sk);
	if (lvl > faction->skills.GetDays(sk))
	{
		faction->skills.SetDays(sk, lvl);
		faction->shows.Add(new ShowSkill(sk, lvl));
	}

	return 1; // success
}

int Unit::Practise(int sk)
{
	int bonus = Globals->SKILL_PRACTISE_AMOUNT;
	if (practised || bonus < 1)
		return 1; // done

	const int days = skills.GetDays(sk);
	const int men = GetMen();

	// if no men or no current skill
	if (men < 1 || days < 1)
		return 0; // no practice

	const int curlev = GetRealSkill(sk);
	if (curlev >= SkillMax(sk))
		return 0; // no practice needed

	for (unsigned i = 0; i < sizeof(SkillDefs[sk].depends)/sizeof(SkillDepend); ++i)
	{
		const int reqsk = SkillDefs[sk].depends[i].skill;
		if (reqsk == -1)
			break; // done

		if (SkillDefs[reqsk].flags & SkillType::DISABLED)
			continue;

		const int reqlev = GetRealSkill(reqsk);
		if (reqlev <= curlev)
		{
			// depth-first recursion
			if (Practise(reqsk))
				return 1;

			// we don't meet the reqs, and can't practise that req, but
			// we still need to check the other reqs
			bonus = 0;
		}
	}

	if (!bonus)
		return 0;

	Study(sk, men * bonus);
	practised = 1;

	Event(AString("Gets practice in ") + SkillDefs[sk].name + " (+" + AString(bonus) + ").");

	return bonus;
}

int Unit::IsLeader()
{
	return GetMen(I_LEADERS) ? 1 : 0;
}

int Unit::IsNormal()
{
	return (GetMen() && !IsLeader()) ? 1 : 0;
}

int Unit::firstManType()
{
	forlist(&items)
	{
		const Item *i = (const Item*)elem;
		if (ItemDefs[i->type].type & IT_MAN)
			return ItemDefs[i->type].index;
	}
	return -1;
}

void Unit::limitSkillNoMagic()
{
	// find highest skills, eliminate others
	Skill *maxskill = NULL; // live-out

	unsigned max = 0; // loop-carry
	forlist(&skills)
	{
		Skill *s = (Skill*)elem;
		if (s->days > max)
		{
			max = s->days;
			maxskill = s;
		}
	}

	// protect macro expansion
	{
		forlist(&skills)
		{
			Skill *s = (Skill*)elem;
			if (s != maxskill)
			{
				skills.Remove(s);
				delete s;
			}
		}
	}
}

void Unit::limitSkillMagic(unsigned max_no_magic)
{
	unsigned num_no_magic = 0;
	int nomagic_skill_max = -1;
	forlist(&skills)
	{
		Skill *s = (Skill*)elem;
		if (SkillDefs[s->type].flags & SkillType::MAGIC)
			continue;

		++num_no_magic;
		if (int(s->days) > nomagic_skill_max)
			nomagic_skill_max = s->days;
	}

	if (num_no_magic <= max_no_magic)
		return; // done

	{
		// start deleting
		forlist(&skills)
		{
			Skill *s = (Skill*)elem;
			if (SkillDefs[s->type].flags & SkillType::MAGIC)
				continue;

			if (int(s->days) <= nomagic_skill_max)
			{
				--num_no_magic;
				skills.Remove(s);
				delete s;
			}

			if (num_no_magic <= max_no_magic)
				return; // done
		}
	}
}

void Unit::AdjustSkills()
{
	// if we are 100% leaders
	if (IsLeader())
	{
		// make sure no skills are > max
		forlist(&skills)
		{
			Skill *s = (Skill*)elem;
			if (GetRealSkill(s->type) >= ::SkillMax(s->type, I_LEADERS))
			{
				s->days = GetDaysByLevel(::SkillMax(s->type, I_LEADERS)) *
					GetMen();
			}
		}

		return;
	}
	//else non-leaders
	int first_man_type = -1;
	{
		forlist(&items)
		{
			Item *i = (Item*)elem;

			if (ItemDefs[i->type].type & IT_MAN)
			{
				first_man_type = ItemDefs[i->type].index;
				break;
			}
		}
	}

	// if we can only know 1 skill, but know more
	if (Globals->SKILL_LIMIT_NONLEADERS && skills.Num() > ManDefs[first_man_type].max_skills)
	{
		if (!Globals->MAGE_NONLEADERS)
		{
			limitSkillNoMagic();
		}
		else
		{
			limitSkillMagic(ManDefs[first_man_type].max_skills - 1);
		}
	}

	// Limit remaining skills to max
	forlist(&skills)
	{
		Skill *theskill = (Skill*)elem;

		const int max = SkillMax(theskill->type);

		if (GetRealSkill(theskill->type) >= max)
		{
			theskill->days = GetDaysByLevel(max) * GetMen();
		}
	}
}

int Unit::MaintCost()
{
	// NPC's don't have maintenance
	if (type == U_WMON || type == U_GUARD || type == U_GUARDMAGE)
		return 0;

	int unit_maint_cost = 0;

	// handle leaders
	int leaders = GetMen(I_LEADERS);
	leaders += GetMen(I_FACTIONLEADER);
	if (leaders < 0)
		leaders = 0;

	// Base cost
	int group_maint_cost = leaders * Globals->LEADER_COST;

	// leaders are counted at maintenance_multiplier * skills in all except
	// the case where it's not being used (mages, leaders, all)
	if (Globals->MULTIPLIER_USE != GameDefs::MULT_NONE)
	{
		// Skill costs are in addtion to base LEADER_COST
		group_maint_cost += leaders * SkillLevels() * Globals->MAINTENANCE_MULTIPLIER;
	}

    // Leader units pay per hit point (can be additive with skill maint)
	if (Globals->MAINT_COST_PER_HIT) {
		const int leader_hits = ManDefs[MAN_LEADER].hits - 1;  // 1 hit paid for in base cost
		if (leader_hits > 0) {
			group_maint_cost += leaders * leader_hits * Globals->LEADER_COST;
		}
	}

	unit_maint_cost = group_maint_cost;

	// handle non-leaders
	int nonleaders = GetMen() - leaders;
	if (nonleaders < 0)
		nonleaders = 0;

	// Base cost
	group_maint_cost = nonleaders * Globals->MAINTENANCE_COST;  // resets group cost

	// non-leaders are counted at maintenance_multiplier * skills only if
	// all characters pay that way
	if (Globals->MULTIPLIER_USE == GameDefs::MULT_ALL)
	{
		// Skill costs are in addtion to base MAINTENANCE_COST
		group_maint_cost += nonleaders * SkillLevels() * Globals->MAINTENANCE_MULTIPLIER;
	}

	if (Globals->MAINT_COST_PER_HIT) {
		// Check each man to determine hit upkeep
		for (int i = 0; i <= NITEMS; ++i)
		{
			if (!(ItemDefs[i].type & IT_MAN)) { continue; }

			if (i == I_LEADERS || i == I_FACTIONLEADER) { continue; }

			const int men_in_unit = GetMen(i);

			if (men_in_unit > 0) {
				const int nonleader_hits = ManDefs[ItemDefs[i].index].hits - 1;  // 1 hit paid for in base cost
				if (nonleader_hits > 0) {
					group_maint_cost += men_in_unit * nonleader_hits * Globals->MAINTENANCE_COST;
				}
			}
		}
	}

	unit_maint_cost += group_maint_cost;

	return unit_maint_cost;
}

void Unit::Short(int needed, int hunger)
{
	if (faction->IsNPC())
		return; // Don't starve monsters and the city guard!

	if (GetMen(I_FACTIONLEADER))
		return; // FACTION LEADER can't starve

	switch (Globals->SKILL_STARVATION)
	{
	case GameDefs::STARVE_MAGES:
		if (type == U_MAGE)
			SkillStarvation();
		return;

	case GameDefs::STARVE_LEADERS:
		if (GetMen(I_LEADERS))
			SkillStarvation();
		return;

	case GameDefs::STARVE_ALL:
		SkillStarvation();
		return;
	}
	//default:

	if (needed < 1 && hunger < 1)
		return;

	int n = 0;

	for (int i = 0; i <= NITEMS; ++i)
	{
		if (!(ItemDefs[ i ].type & IT_MAN))
		{
			// only men need sustenance
			continue;
		}

		if (i == I_LEADERS)
		{
			// don't starve leaders just yet
			continue;
		}

		while (GetMen(i))
		{
			if (getrandom(100) < Globals->STARVE_PERCENT)
			{
				SetMen(i, GetMen(i) - 1);
				++n;
			}

			if (Globals->MULTIPLIER_USE == GameDefs::MULT_ALL)
			{
				const int levels = SkillLevels();
				i = levels * Globals->MAINTENANCE_MULTIPLIER;
				if (i < Globals->MAINTENANCE_COST)
					i = Globals->MAINTENANCE_COST;

				needed -= i;
			}
			else
				needed -= Globals->MAINTENANCE_COST;

			hunger -= Globals->UPKEEP_MINIMUM_FOOD;

			if (needed < 1 && hunger < 1)
			{
				if (n)
					Error(AString(n) + " starve to death.");

				return;
			}
		}
	}

	while (GetMen(I_LEADERS))
	{
		if (getrandom(100) < Globals->STARVE_PERCENT)
		{
			SetMen(I_LEADERS, GetMen(I_LEADERS) - 1);
			++n;
		}

		if (Globals->MULTIPLIER_USE != GameDefs::MULT_NONE)
		{
			const int levels = SkillLevels();
			int i = levels * Globals->MAINTENANCE_MULTIPLIER;
			if (i < Globals->LEADER_COST)
				i = Globals->LEADER_COST;
			needed -= i;
		}
		else
			needed -= Globals->LEADER_COST;

		hunger -= Globals->UPKEEP_MINIMUM_FOOD;
		if (needed < 1 && hunger < 1)
		{
			if (n)
				Error(AString(n) + " starve to death.");
			return;
		}
	}
}

int Unit::Weight()
{
	return items.Weight();
}

int Unit::FlyingCapacity()
{
	int cap = 0;
	forlist(&items)
	{
		Item *i = (Item*)elem;
		cap += ItemDefs[i->type].fly * i->num;
	}

	return cap;
}

int Unit::RidingCapacity()
{
	int cap = 0;
	forlist(&items)
	{
		Item *i = (Item*)elem;
		cap += ItemDefs[i->type].ride * i->num;
	}

	return cap;
}

int Unit::SwimmingCapacity()
{
	int cap = 0;
	forlist(&items)
	{
		Item *i = (Item*)elem;
		cap += ItemDefs[i->type].swim * i->num;
	}

	return cap;
}

int Unit::WalkingCapacity()
{
	int cap = 0;
	std::map<int, int> used_items;  // pulling/pullable itmes (e.g. horses, camels, wagon, etc)

	forlist(&items)
	{
		Item *i = (Item*)elem;

		cap += ItemDefs[i->type].walk * i->num;

		// Add capacity of pullable items (wagons, etc)
		for (unsigned c = 0; c < sizeof(ItemDefs[i->type].hitchItems)/sizeof(HitchItem); ++c) {

			const HitchItem &pulling_item = ItemDefs[i->type].hitchItems[c];

			// No pulling item specified
			if (pulling_item.item == -1)
				continue;

			// Same item cannot pull itself
			if (pulling_item.item == i->type)
				continue;

			const ItemType &pullable_item = ItemDefs[i->type];

			int pulling_items = items.GetNum(pulling_item.item);  // horse, camel, etc
			int pullable_items = i->num;  // wagon, etc

			int used_pulling_items = 0;
			if (used_items.find(pulling_item.item) != used_items.end())
				used_pulling_items = used_items[pulling_item.item];

			int used_pullable_items = 0;
			if (used_items.find(pullable_item.index) != used_items.end())
				used_pullable_items = used_items[pullable_item.index];

			// Subtract already used pulling items
			pulling_items -= used_pulling_items;
			if (pulling_items <= 0)
				continue;

			// More pulling items than needed, leave the rest for other pullables
			if (pulling_items > pullable_items)
				pulling_items = pullable_items;

			// Subtract already used pullable items
			pullable_items -= used_pullable_items;
			if (pullable_items <= 0)
				continue;

			// Not enough pulling items, less pullables will be used
			if (pullable_items > pulling_items)
				pullable_items = pulling_items;

			cap += pullable_items * pulling_item.walk;  // pulling_item.walk is wagon bonus walk capacity

			// Update the used item values
			used_items[pulling_item.item] = pulling_items + used_pulling_items;
			used_items[pullable_item.index] = pullable_items + used_pullable_items;
		}
	}

	return cap;
}

int Unit::CanFly(int weight)
{
	return (FlyingCapacity() >= weight) ? 1 : 0;
}

int Unit::CanReallySwim()
{
	return (SwimmingCapacity() >= items.Weight()) ? 1 : 0;
}

int Unit::CanSwim()
{
	if (CanReallySwim())
		return 1;

	if (Globals->FLIGHT_OVER_WATER != GameDefs::WFLIGHT_NONE && CanFly())
		return 1;

	return 0;
}

int Unit::CanFly()
{
	return CanFly(items.Weight());
}

int Unit::CanRide(int weight)
{
	return (RidingCapacity() >= weight) ? 1 : 0;
}

int Unit::CanWalk(int weight)
{
	return (WalkingCapacity() >= weight) ? 1 : 0;
}

int Unit::MoveType()
{
	const int weight = items.Weight();

	// priority: fly, ride, walk
	if (CanFly (weight)) return M_FLY;
	if (CanRide(weight)) return M_RIDE;

	// Check if we should be able to 'swim'
	// (This should become it's own M_TYPE sometime)
	if (TerrainDefs[object->region->type].similar_type == R_OCEAN && CanSwim())
		return M_WALK;

	return CanWalk(weight) ? M_WALK : M_NONE;
}

int Unit::CalcMovePoints()
{
	switch (MoveType())
	{
		case M_NONE: return 0;

		case M_WALK: return Globals->FOOT_SPEED;

		case M_RIDE: return Globals->HORSE_SPEED;

		case M_FLY:
			if (GetSkill(S_SUMMON_WIND)) return Globals->FLY_SPEED + 2;

			return Globals->FLY_SPEED;
	}

	return 0;
}

int Unit::CanMoveTo(ARegion *r1, ARegion *r2)
{
	if (r1 == r2)
		return 1;

	// check exits
	int exit = 1; // can use an exit
	int dir  = -1; // which exit to use

	for (int i = 0; exit && i < NDIRS; ++i)
	{
		if (r1->neighbors[i] == r2)
		{
			exit = 0;
			dir = i;
		}
	}

	// if exit check failed
	if (exit)
		return 0;

	// check for opposite direction
	exit = 1;
	for (int i = 0; i < NDIRS; ++i)
	{
		if (r2->neighbors[i] == r1)
		{
			exit = 0;
			break;
		}
	}

	if (exit)
		return 0;

	// check for crossing water
	if ((TerrainDefs[r1->type].similar_type == R_OCEAN ||
	     TerrainDefs[r2->type].similar_type == R_OCEAN) &&
	    (!CanSwim() || GetFlag(FLAG_NOCROSS_WATER)))
	{
		return 0;
	}

	// check move cost
	const int mt = MoveType();
	const int mp = CalcMovePoints() - movepoints;
	if (mp < r2->MoveCost(mt, r1, dir, 0))
		return 0;

	return 1;
}

int Unit::CanCatch(ARegion *r, Unit *u)
{
	return faction->CanCatch(r, u);
}

int Unit::CanSee(ARegion *r, Unit *u, int practise)
{
	return faction->CanSee(r, u, practise);
}

int Unit::AmtsPreventCrime(Unit *u)
{
	if (!u)
		return 0;

	const int amulets = items.GetNum(I_AMULETOFTS);
	if (u->items.GetNum(I_RINGOFI) < 1 || amulets < 1)
		return 0;

	const int men = GetMen();
	if (men <= amulets)
		return 1;

	// settings allow a shortage of amulets to sometimes work
	if (!Globals->PROPORTIONAL_AMTS_USAGE)
		return 0;

	return (getrandom(men) < amulets) ? 1 : 0;
}

int Unit::GetAttitude(ARegion *r, Unit *u)
{
	// ally with ourselves
	if (faction == u->faction)
		return A_ALLY;

	const int att = faction->GetAttitude(u->faction->num);
	if (att >= A_FRIENDLY && att >= faction->defaultattitude)
		return att;

	if (CanSee(r, u) == 2)
		return att;

	return faction->defaultattitude;
}

int Unit::Hostile()
{
	if (type != U_WMON)
		return 0;

	int retval = 0;
	forlist(&items)
	{
		Item *i = (Item*)elem;
		if (ItemDefs[i->type].type & IT_MONSTER)
		{
			const int hos = MonDefs[ItemDefs[i->type].index].hostile;
			if (hos > retval)
				retval = hos;
		}
	}

	return retval;
}

int Unit::Forbids(ARegion *r, Unit *u)
{
	// only units on guard will forbid
	if (guard != GUARD_GUARD)
		return 0;

	// isAlive() actually returns fighting ability
	if (!IsAlive())
		return 0;

	// can't forbid what you can't see
	if (!CanSee(r, u, Globals->SKILL_PRACTISE_AMOUNT > 0))
		return 0;

	// or catch
	if (!CanCatch(r, u))
		return 0;

	// default behavior is peace
	if (GetAttitude(r, u) < A_NEUTRAL)
		return 1;

	return 0;
}

int Unit::Taxers()
{
	const int totalMen = GetMen();
	int illusions = 0;
	int creatures = 0;
	int taxers = 0;

	// if men can tax
	if ((Globals->WHO_CAN_TAX & GameDefs::TAX_ANYONE) ||
		((Globals->WHO_CAN_TAX & GameDefs::TAX_COMBAT_SKILL) &&
		 GetSkill(S_COMBAT)) ||
		((Globals->WHO_CAN_TAX & GameDefs::TAX_BOW_SKILL) &&
		 (GetSkill(S_CROSSBOW) || GetSkill(S_LONGBOW))) ||
		((Globals->WHO_CAN_TAX & GameDefs::TAX_RIDING_SKILL) &&
		 GetSkill(S_RIDING)) ||
		((Globals->WHO_CAN_TAX & GameDefs::TAX_STEALTH_SKILL) &&
		 GetSkill(S_STEALTH)))
	{
		taxers = totalMen;
	}
	else
	{
		// check out items
		int numMelee = 0;
		int numUsableMelee = 0;
		int numBows = 0;
		int numUsableBows = 0;
		int numMounted = 0;
		int numUsableMounted = 0;
		int numMounts = 0;
		int numUsableMounts = 0;
		int numBattle = 0;
		int numUsableBattle = 0;

		forlist(&items)
		{
			Item *pItem = (Item*)elem;

			if ((ItemDefs[pItem->type].type & IT_BATTLE) &&
			    (BattleItemDefs[ItemDefs[pItem->type].battleindex].flags & BattleItemType::SPECIAL))
			{
				// Only consider offensive items
				if ((Globals->WHO_CAN_TAX &
				         GameDefs::TAX_USABLE_BATTLE_ITEM) &&
				    (!(BattleItemDefs[ItemDefs[pItem->type].battleindex].flags &
				       BattleItemType::MAGEONLY) ||
				     type == U_MAGE || type == U_APPRENTICE))
				{
					numUsableBattle += pItem->num;
					numBattle       += pItem->num;
					continue; // Don't count this as a weapon as well!
				}

				if (Globals->WHO_CAN_TAX & GameDefs::TAX_BATTLE_ITEM)
				{
					numBattle += pItem->num;
					continue; // Don't count this as a weapon as well!
				}
			}

			if (ItemDefs[pItem->type].type & IT_WEAPON)
			{
				WeaponType *pWep = WeaponDefs + ItemDefs[pItem->type].index;
				if (!(pWep->flags & WeaponType::NEEDSKILL))
				{
					// A melee weapon
					if (GetSkill(S_COMBAT))
						numUsableMelee += pItem->num;

					numMelee += pItem->num;
				}
				else if (pWep->baseSkill == S_RIDING)
				{
					// A mounted weapon
					if (GetSkill(S_RIDING))
						numUsableMounted += pItem->num;

					numMounted += pItem->num;
				}
				else
				{
					// Presume that anything else is a bow!
					if (GetSkill(pWep->baseSkill) ||
					    (pWep->orSkill != -1 && GetSkill(pWep->orSkill)))
					{
						numUsableBows += pItem->num;
					}
					numBows += pItem->num;
				}
			}

			if (ItemDefs[pItem->type].type & IT_MOUNT)
			{
				if (MountDefs[ItemDefs[pItem->type].index].minBonus
						<= GetSkill(S_RIDING))
				{
					numUsableMounts += pItem->num;
				}
				numMounts += pItem->num;
			}

			if (ItemDefs[pItem->type].type & IT_MONSTER)
			{
				if (ItemDefs[pItem->type].index == MONSTER_ILLUSION)
					illusions += pItem->num;
				else
					creatures += pItem->num;
			}
		}

		// Ok, now process the counts!
		if (Globals->WHO_CAN_TAX & GameDefs::TAX_USABLE_WEAPON)
		{
			if (numUsableMounted > numUsableMounts)
			{
				taxers = numUsableMounts;
				numMounts -= numUsableMounts;
				numUsableMounts = 0;
			}
			else
			{
				taxers = numUsableMounted;
				numMounts -= numUsableMounted;
				numUsableMounts -= numUsableMounted;
			}

			taxers += numMelee + numUsableBows;
		}
		else if (Globals->WHO_CAN_TAX & GameDefs::TAX_ANY_WEAPON)
		{
			taxers = numMelee + numBows + numMounted;
		}
		else
		{
			if (Globals->WHO_CAN_TAX & GameDefs::TAX_MELEE_WEAPON_AND_MATCHING_SKILL)
			{
				if (numUsableMounted > numUsableMounts)
				{
					taxers += numUsableMounts;
					numMounts -= numUsableMounts;
					numUsableMounts = 0;
				}
				else
				{
					taxers += numUsableMounted;
					numMounts -= numUsableMounted;
					numUsableMounts -= numUsableMounted;
				}
				taxers += numUsableMelee;
			}

			if (Globals->WHO_CAN_TAX & GameDefs::TAX_BOW_SKILL_AND_MATCHING_WEAPON)
			{
				taxers += numUsableBows;
			}
		}

		if (Globals->WHO_CAN_TAX & GameDefs::TAX_HORSE)
			taxers += numMounts;
		else if (Globals->WHO_CAN_TAX & GameDefs::TAX_HORSE_AND_RIDING_SKILL)
			taxers += numUsableMounts;

		if (Globals->WHO_CAN_TAX & GameDefs::TAX_BATTLE_ITEM)
			taxers += numBattle;
		else if (Globals->WHO_CAN_TAX & GameDefs::TAX_USABLE_BATTLE_ITEM)
			taxers += numUsableBattle;
	}

	// Ok, all the items categories done - check for mages taxing
	if (type == U_MAGE)
	{
		if (Globals->WHO_CAN_TAX & GameDefs::TAX_ANY_MAGE)
			taxers = totalMen;
		else
		{
			if (Globals->WHO_CAN_TAX & GameDefs::TAX_MAGE_COMBAT_SPELL)
			{
				if ((Globals->WHO_CAN_TAX & GameDefs::TAX_MAGE_DAMAGE) &&
						(combat == S_FIRE || combat == S_EARTHQUAKE ||
						 combat == S_SUMMON_TORNADO ||
						 combat == S_CALL_LIGHTNING ||
						 combat == S_SUMMON_BLACK_WIND))
					taxers = totalMen;

				if ((Globals->WHO_CAN_TAX & GameDefs::TAX_MAGE_FEAR) &&
						(combat == S_SUMMON_STORM ||
						 combat == S_CREATE_AURA_OF_FEAR))
					taxers = totalMen;

				if ((Globals->WHO_CAN_TAX & GameDefs::TAX_MAGE_OTHER) &&
						(combat == S_FORCE_SHIELD ||
						 combat == S_ENERGY_SHIELD ||
						 combat == S_SPIRIT_SHIELD ||
						 combat == S_CLEAR_SKIES))
					taxers = totalMen;
			}
			else
			{
				if ((Globals->WHO_CAN_TAX & GameDefs::TAX_MAGE_DAMAGE) &&
						(GetSkill(S_FIRE) || GetSkill(S_EARTHQUAKE) ||
						 GetSkill(S_SUMMON_TORNADO) ||
						 GetSkill(S_CALL_LIGHTNING) ||
						 GetSkill(S_SUMMON_BLACK_WIND)))
					taxers = totalMen;

				if ((Globals->WHO_CAN_TAX & GameDefs::TAX_MAGE_FEAR) &&
						(GetSkill(S_SUMMON_STORM) ||
						 GetSkill(S_CREATE_AURA_OF_FEAR)))
					taxers = totalMen;

				if ((Globals->WHO_CAN_TAX & GameDefs::TAX_MAGE_OTHER) &&
						(GetSkill(S_FORCE_SHIELD) ||
						 GetSkill(S_ENERGY_SHIELD) ||
						 GetSkill(S_SPIRIT_SHIELD) ||
						 GetSkill(S_CLEAR_SKIES)))
					taxers = totalMen;
			}
		}
	}

	// Now check for an overabundance of tax enabling objects
	if (taxers > totalMen)
		taxers = totalMen;

	// And finally for creatures
	if (Globals->WHO_CAN_TAX & GameDefs::TAX_CREATURES)
		taxers += creatures;

	if (Globals->WHO_CAN_TAX & GameDefs::TAX_ILLUSIONS)
		taxers += illusions;

	return taxers;
}

int Unit::GetFlag(int mask) const
{
	return (flags & mask);
}

void Unit::SetFlag(int x, int val)
{
	if (val)
		flags = flags | x;
	else
		if (flags & x) flags -= x;
}

void Unit::CopyFlags(const Unit *x)
{
	flags = x->flags;

	if (x->guard != GUARD_SET && x->guard != GUARD_ADVANCE)
		guard = x->guard;
	else
		guard = GUARD_NONE;

	reveal = x->reveal;
}

int Unit::GetBattleItem(int index)
{
	forlist(&items)
	{
		Item *pItem = (Item*)elem;
		if (!pItem->num)
			continue;

		int item = pItem->type;
		if ((ItemDefs[item].type & IT_BATTLE) && ItemDefs[item].battleindex == index)
		{
			// Exclude weapons.  They will be handled later
			if (ItemDefs[item].type & IT_WEAPON)
				continue;

			items.SetNum(item, pItem->num - 1);
			return item;
		}
	}

	return -1;
}

int Unit::GetArmor(int index, int ass)
{
	if (ass && !(ArmorDefs[index].flags & ArmorType::USEINASSASSINATE))
		return -1;

	forlist(&items)
	{
		Item *pItem = (Item*)elem;
		if (!pItem->num)
			continue;

		const int item = pItem->type;
		if ((ItemDefs[item].type & IT_ARMOR) && ItemDefs[item].index == index)
		{
			// Get the armor
			items.SetNum(item, pItem->num - 1);
			return item;
		}
	}

	return -1;
}

int Unit::GetMount(int index, int canFly, int canRide, int &bonus)
{
	bonus = 0; // side-out

	if (!canFly && !canRide)
		return -1;

	//foreach item
	forlist(&items)
	{
		Item *pItem = (Item*)elem;
		if (!pItem->num)
			continue;

		const int item = pItem->type;
		if ((ItemDefs[item].type & IT_MOUNT) && ItemDefs[item].index == index)
		{
			// Found a possible mount
			if (canFly)
			{
				if (!ItemDefs[item].fly)
				{
					// mount cannot fly, see if the region allows riding mounts
					if (!canRide)
						continue;
				}
			}
			else
			{
				// This region allows riding mounts, so if the mount
				// can not carry at a riding level, continue
				if (!ItemDefs[item].ride)
					continue;
			}

			MountType *pMnt = &MountDefs[index];
			bonus = GetSkill(pMnt->skill);
			if (bonus < pMnt->minBonus)
			{
				// Unit isn't skilled enough for this mount
				bonus = 0;
				continue;
			}

			// Limit to max mount bonus;
			if (bonus > pMnt->maxBonus)
				bonus = pMnt->maxBonus;

			// If the mount can fly and the terrain doesn't allow
			// flying mounts, limit the bonus to the maximum hampered
			// bonus allowed by the mount
			if (ItemDefs[item].fly && !canFly)
			{
				if (bonus > pMnt->maxHamperedBonus)
					bonus = pMnt->maxHamperedBonus;
			}

			// Get the mount
			items.SetNum(item, pItem->num - 1);
			return item;
		}
	} //foreach item

	return -1;
}

int Unit::GetWeapon(int index, int riding, int ridingBonus, int &attackBonus,
      int &defenseBonus, int &attacks)
{
	// initialize output variables
	attackBonus = 0;
	defenseBonus = 0;
	attacks = 1;

	const int combatSkill = GetSkill(S_COMBAT);
	forlist(&items)
	{
		Item *pItem = (Item*)elem;
		if (!pItem->num)
			continue;

		const int item = pItem->type;
		if ((ItemDefs[item].type & IT_WEAPON) && ItemDefs[item].index == index)
		{
			// Found a weapon, check flags and skills
			WeaponType *pWep = &WeaponDefs[index];
			int baseSkillLevel = CanUseWeapon(pWep, riding);
			// returns -1 if weapon cannot be used, else the usable skill level
			if (baseSkillLevel == -1)
				continue;

			const int flags = pWep->flags;
			// Attack and defense skill
			if (!(flags & WeaponType::NEEDSKILL))
				baseSkillLevel = combatSkill;

			attackBonus = baseSkillLevel + pWep->attackBonus;

			if (flags & WeaponType::NOATTACKERSKILL)
				defenseBonus = pWep->defenseBonus;
			else
				defenseBonus = baseSkillLevel + pWep->defenseBonus;

			// Riding bonus
			if (flags & WeaponType::RIDINGBONUS)
				attackBonus += ridingBonus;
			if (flags & (WeaponType::RIDINGBONUSDEFENSE |
			             WeaponType::RIDINGBONUS))
			{
				defenseBonus += ridingBonus;
			}

			// Number of attacks
			attacks = pWep->numAttacks;

			// Note: NUM_ATTACKS_SKILL must be > NUM_ATTACKS_HALF_SKILL
			if (attacks >= WeaponType::NUM_ATTACKS_SKILL)
				attacks += baseSkillLevel - WeaponType::NUM_ATTACKS_SKILL;
			else if (attacks >= WeaponType::NUM_ATTACKS_HALF_SKILL)
			{
				attacks += (baseSkillLevel +1)/2 -
				   WeaponType::NUM_ATTACKS_HALF_SKILL;
			}

			// Sanity check
			if (attacks == 0)
				attacks = 1;

			// get the weapon
			items.SetNum(item, pItem->num - 1);
			return item;
		}
	}

	return -1;
}

void Unit::MoveUnit(Object *toobj)
{
	// notify current object we are leaving
	if (object)
		object->units.Remove(this);

	object = toobj;

	// back pointer from object
	if (toobj)
		toobj->units.Add(this);
}

void Unit::Event(const AString &s)
{
	AString temp = *name + ": " + s;
	faction->Event(temp);
}

void Unit::Error(const AString &s)
{
	AString temp = *name + ": " + s;
	faction->Error(temp);
}

int Unit::GetSkillBonus(int sk)
{
	int bonus = 0;
	const int men = GetMen();

	switch (sk)
	{
		case S_OBSERVATION:
			if(!men)
				break;

			if (Globals->FULL_TRUESEEING_BONUS)
			{
				bonus = GetSkill(S_TRUE_SEEING);
			}
			else // half rounded up
			{
				bonus = (GetSkill(S_TRUE_SEEING)+1) / 2;
			}

			if ((bonus < (2 + Globals->IMPROVED_AMTS)) &&
			    items.GetNum(I_AMULETOFTS))
			{
				bonus = 2 + Globals->IMPROVED_AMTS;
			}
			break;

		case S_STEALTH:
			if (men == 1 && Globals->FULL_INVIS_ON_SELF)
			{
				bonus = GetSkill(S_INVISIBILITY);
			}

			if (bonus < 3 &&
			    (GetFlag(FLAG_INVIS) || men <= items.GetNum(I_RINGOFI)))
			{
				bonus = 3;
			}
			break;

		default:;
	}

	return bonus;
}

int Unit::GetProductionBonus(int item)
{
	const int men = GetMen();

	int bonus = 0;
	if (ItemDefs[item].mult_item != -1)
	{
		bonus = items.GetNum(ItemDefs[item].mult_item);
		if (bonus > men)
			bonus = men;
	}
	else // -1 -> everyone gets a bonus
		bonus = men;

	// scale the bonus (usually -1 mult_item -> 0 mult_val)
	return bonus * ItemDefs[item].mult_val;
}

int Unit::SkillLevels()
{
	int levels = 0;
	forlist(&skills)
	{
		Skill *s = (Skill*)elem;
		levels += GetLevelByDays(s->days / GetMen());
	}
	return levels;
}

Skill* Unit::GetSkillObject(int sk)
{
	forlist(&skills)
	{
		Skill *s = (Skill*)elem;
		if (s->type == sk)
			return s;
	}

	return NULL;
}

void Unit::SkillStarvation()
{
	// bit mask of which skills can be forgotten
	bool can_forget[NSKILLS];
	int count = 0; // number of bits set

	// initialize bit set
	for (int i = 0; i < NSKILLS; ++i)
	{
		if (SkillDefs[i].flags & SkillType::DISABLED)
		{
			can_forget[i] = false;
			continue;
		}

		if (GetSkillObject(i))
		{
			can_forget[i] = true;
			++count;
		}
		else
		{
			can_forget[i] = false;
		}
	}

	for (int i = 0; i < NSKILLS; ++i)
	{
		if (!can_forget[i])
			continue;

		Skill *si = GetSkillObject(i);
		for (int j = 0; j < NSKILLS; ++j)
		{
			if (SkillDefs[j].flags & SkillType::DISABLED)
				continue;

			Skill *sj = GetSkillObject(j);
			int dependancy_level = 0;
			for (unsigned c = 0; c < sizeof(SkillDefs[i].depends)/sizeof(SkillDepend); ++c)
			{
				if (SkillDefs[i].depends[c].skill == j)
				{
					dependancy_level = SkillDefs[i].depends[c].level;
					break;
				}
			}

			if (dependancy_level > 0)
			{
				if (GetLevelByDays(sj->days) == GetLevelByDays(si->days))
				{
					can_forget[j] = false;
					--count;
				}
			}
		}
	}

	if (!count)
	{
		forlist(&items)
		{
			Item *i = (Item*)elem;
			if (ItemDefs[i->type].type & IT_MAN)
			{
				count += items.GetNum(i->type);
				items.SetNum(i->type, 0);
			}
		}

		AString temp = AString(count) + " starve to death.";
		Error(temp);
		return;
	}

	count = getrandom(count) + 1;
	for (int i = 0; i < NSKILLS; ++i)
	{
		if (!can_forget[i])
			continue;

		--count;
		if (count != 0)
			continue;

		AString temp = AString("Starves and forgets one level of ")+
			SkillDefs[i].name + ".";
		Error(temp);

		Skill *s = GetSkillObject(i);
		const int level = GetLevelByDays(s->days);
		s->days -= level * 30;
		if (level == 1)
		{
			if (s->days <= 0)
				ForgetSkill(i);
		}
	}
}

int Unit::CanUseWeapon(const WeaponType *pWep, int riding)
{
	if (riding == -1)
	{
		if (pWep->flags & WeaponType::NOFOOT)
			return -1;
	}
	else
	{
		if (pWep->flags & WeaponType::NOMOUNT)
			return -1;
	}

	return CanUseWeapon(pWep);
}

int Unit::CanUseWeapon(const WeaponType *pWep)
{
	// we don't care in this case, their combat skill will be used
	if (!(pWep->flags & WeaponType::NEEDSKILL))
	{
		Practise(S_COMBAT);
		return 0;
	}

	int baseSkillLevel = 0;
	if (pWep->baseSkill != -1)
		baseSkillLevel = GetSkill(pWep->baseSkill);

	int tempSkillLevel = 0;
	if (pWep->orSkill != -1)
		tempSkillLevel = GetSkill(pWep->orSkill);

	if (tempSkillLevel > baseSkillLevel)
	{
		baseSkillLevel = tempSkillLevel;
		Practise(pWep->orSkill);
	}
	else
		Practise(pWep->baseSkill);

	if ((pWep->flags & WeaponType::NEEDSKILL) && !baseSkillLevel)
		return -1;

	return baseSkillLevel;
}


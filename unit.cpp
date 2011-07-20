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
// MODIFICATIONS
// Date		Person		Comments
// ----		------		--------
// 2000/MAR/14	Larry Stanbery	Corrected logical flaw in creation of mages.
//				Replaced specific skill bonus functions with
//				generic function.
// 2001/Feb/18	Joseph Traub	Added support for Apprentices
// 2001/Feb/25	Joseph Traub	Added a flag preventing units from crossing
//				water.
#include "unit.h"
#include "gamedata.h"

//----------------------------------------------------------------------------
UnitId::UnitId()
: unitnum(-1)
, alias  (-1)
, faction(-1)
{
}

UnitId::~UnitId()
{
}

AString UnitId::Print()
{
	if (unitnum)
		return AString(unitnum);

	if (faction)
	{
		return AString("faction ") + AString(faction) + " new " +
			AString(alias);
	}

	return AString("new ") + AString(alias);
}

//----------------------------------------------------------------------------
Unit::Unit()
{
	name = 0;
	describe = 0;
	num = 0;
	type = U_NORMAL;
	faction = 0;
	formfaction = 0;
	alias = 0;
	guard = GUARD_NONE;
	reveal = REVEAL_NONE;
	flags = 0;
	combat = -1;

	for(int i = 0; i < MAX_READY; i++)
	{
		readyWeapon[i] = -1;
		readyArmor[i] = -1;
	}

	readyItem = -1;
	object = 0;
	attackorders = NULL;
	evictorders = NULL;
	stealorders = NULL;
	monthorders = NULL;
	castorders = NULL;
	teleportorders = NULL;
	inTurnBlock = 0;
	presentTaxing = 0;
	presentMonthOrders = NULL;
	former = NULL;
	free = 0;
	practised = 0;

	ClearOrders();
}

Unit::Unit(int seq, Faction *f, int a)
{
	num = seq;
	type = U_NORMAL;
	name = new AString;
	*name = AString("Unit (") + num + ")";
	describe = 0;
	faction = f;
	formfaction = f;
	alias = a;
	guard = 0;
	reveal = REVEAL_NONE;
	flags = 0;
	combat = -1;
	for(int i = 0; i < MAX_READY; i++)
	{
		readyWeapon[i] = -1;
		readyArmor [i] = -1;
	}
	readyItem = -1;
	object = 0;
	attackorders = NULL;
	evictorders = NULL;
	stealorders = NULL;
	monthorders = NULL;
	castorders = NULL;
	teleportorders = NULL;
	inTurnBlock = 0;
	presentTaxing = 0;
	presentMonthOrders = NULL;
	former = NULL;
	free = 0;

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

/// set flags for a monster
void Unit::SetMonFlags()
{
	guard = GUARD_AVOID;
	SetFlag(FLAG_HOLDING, 1);
}

void Unit::MakeWMon(const char *monname, int mon, int num)
{
	AString *temp = new AString(monname);
	SetName(temp);

	type = U_WMON;
	items.SetNum(mon,num);
	SetMonFlags();
}

void Unit::Writeout(Aoutfile *s)
{
	s->PutStr(*name);
	if (describe)
	{
		s->PutStr(*describe);
	}
	else
	{
		s->PutStr("none");
	}

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
		describe = 0;
	}
	num = s->GetInt();
	type = s->GetInt();
	faction = GetFaction(facs, s->GetInt());

	guard = s->GetInt();
	if(guard == GUARD_ADVANCE) guard = GUARD_NONE;
	if(guard == GUARD_SET) guard = GUARD_GUARD;

	reveal = s->GetInt();

	// Handle the new 'ready item', ready weapons/armor, and free
	free = 0;
	readyItem = -1;
	for(int i = 0; i < MAX_READY; i++)
	{
		readyWeapon[i] = -1;
		readyArmor [i] = -1;
	}

	// Negative indicates extra values inserted vs older games
	int i = s->GetInt();
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
			for(i = 0; i < MAX_READY; i++) {
				readyWeapon[i] = s->GetInt();
				readyArmor[i] = s->GetInt();
			}
			flags = s->GetInt();
			break;

		case -3:
			free = s->GetInt();
			readyItem = s->GetInt();
			for(i = 0; i < MAX_READY; i++) {
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

AString Unit::MageReport()
{
	if (combat != -1)
	{
		return AString(". Combat spell: ") + SkillStrs(combat);
	}

	return AString();
}

AString Unit::ReadyItem()
{
	AString temp, weaponstr, armorstr, battlestr;
	int weapon, armor, i, ready;

	int item = 0;
	for (i = 0; i < MAX_READY; ++i)
	{
		ready = readyWeapon[i];
		if (ready != -1)
		{
			if(item) weaponstr += ", ";
			weaponstr += ItemString(ready, 1);
			++item;
		}
	}

	if (item > 0)
	{
		weaponstr = AString("Ready weapon") + (item == 1?"":"s") + ": " +
			weaponstr;
	}
	weapon = item;

	item = 0;
	for(i = 0; i < MAX_READY; ++i)
	{
		ready = readyArmor[i];
		if (ready != -1)
		{
			if(item) armorstr += ", ";
			armorstr += ItemString(ready, 1);
			++item;
		}
	}

	if (item > 0)
		armorstr = AString("Ready armor: ") + armorstr;
	armor = item;

	if (readyItem != -1)
	{
		battlestr = AString("Ready item: ") + ItemString(readyItem, 1);
		item = 1;
	}
	else
		item = 0;

	if (weapon || armor || item)
	{
		temp += AString(". ");
		if(weapon) temp += weaponstr;
		if(armor) {
			if(weapon) temp += ". ";
			temp += armorstr;
		}
		if(item) {
			if(armor || weapon) temp += ". ";
			temp += battlestr;
		}
	}

	return temp;
}

AString Unit::StudyableSkills()
{
	AString temp;
	int j = 0;

	for (int i = 0; i < NSKILLS; ++i)
	{
		if (SkillDefs[i].depends[0].skill != -1)
		{
			if (CanStudy(i))
			{
				if (j)
				{
					temp += ", ";
				}
				else
				{
					temp += ". Can Study: ";
					j=1;
				}
				temp += SkillStrs(i);
			}
		}
	}
	return temp;
}

AString Unit::GetName(int obs)
{
	AString ret = *name;

	if (reveal == REVEAL_FACTION || obs > GetSkill(S_STEALTH))
	{
		ret += ", ";
		ret += *faction->name;
	}
	return ret;
}

///@return 1 if this unit wants item 'i'
int Unit::CanGetSpoil(Item *i)
{
	if (!i) return 0;

	const int weight = ItemDefs[i->type].weight;
	if (!weight) return 1; // any unit can carry 0 weight spoils

	if (flags & FLAG_NOSPOILS) return 0;

	// check spoils flags
	// (only pick up items with their own carrying capacity)
	int fly  = ItemDefs[i->type].fly;
	int ride = ItemDefs[i->type].ride;
	int walk = ItemDefs[i->type].walk;

	if ((flags & FLAG_FLYSPOILS ) && fly  < weight) return 0; // only flying
	if ((flags & FLAG_WALKSPOILS) && walk < weight) return 0; // only walking
	if ((flags & FLAG_RIDESPOILS) && ride < weight) return 0; // only riding

	return 1; // all spoils
}

AString Unit::SpoilsReport()
{
	AString temp;

	if(GetFlag(FLAG_NOSPOILS)) temp = ", weightless battle spoils";
	else if(GetFlag(FLAG_FLYSPOILS)) temp = ", flying battle spoils";
	else if(GetFlag(FLAG_WALKSPOILS)) temp = ", walking battle spoils";
	else if(GetFlag(FLAG_RIDESPOILS)) temp = ", riding battle spoils";

	return temp;
}

void Unit::WriteReport(Areport *f, int obs, int truesight, int detfac,
			   int autosee)
{
	int stealth = GetSkill(S_STEALTH);
	if (obs == -1)
	{
		// this unit belongs to the Faction writing the report
		obs = 2;
	}
	else
	{
		if (obs < stealth)
		{
			// The unit cannot be seen
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
				// Can see unit, but not Faction
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
				// Can see unit and Faction
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

	if (detfac && obs != 2) obs = 1;

	// Write the report
	AString temp;
	if (obs == 2)
	{
		temp += AString("* ") + *name;
	}
	else
	{
		temp += AString("- ") + *name;
	}

	if (guard == GUARD_GUARD) temp += ", on guard";

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
		temp += SpoilsReport();
	}

	temp += items.Report(obs,truesight,0);

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
	// Write the report
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
	temp += SpoilsReport();

	temp += items.Report(2,1,0);
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
	temp += ". Skills: ";
	temp += skills.Report(GetMen());

	if (type == U_MAGE || type == U_GUARDMAGE)
	{
		temp += MageReport();
	}
	temp += ReadyItem();
	temp += StudyableSkills();

	if (describe)
	{
		temp += AString("; ") + *describe;
	}
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
	canattack = 1;
	nomove = 0;
	enter = 0;
	build = NULL;
	leftShip = 0;
	destroy = 0;
	delete attackorders;
	attackorders = 0;
	delete evictorders;
	evictorders = 0;
	delete stealorders;
	stealorders = 0;
	promote = 0;
	taxing = TAX_NONE;
	advancefrom = 0;
	movepoints = 0;
	delete monthorders;
	monthorders = 0;
	inTurnBlock = 0;
	presentTaxing = 0;
	delete presentMonthOrders;
	presentMonthOrders = 0;
	delete castorders;
	castorders = 0;
	delete teleportorders;
	teleportorders = 0;
}

void Unit::ClearCastOrders()
{
	delete castorders;
	castorders = 0;
	delete teleportorders;
	teleportorders = 0;
}

void Unit::DefaultOrders(Object *obj)
{
	ClearOrders();

	if (type == U_WMON)
	{
		if (ObjectDefs[obj->type].monster == -1)
		{
			MoveOrder *o = new MoveOrder;
			o->advancing = 0;
			int aper = Hostile();
			aper *= Globals->MONSTER_ADVANCE_HOSTILE_PERCENT;
			aper /= 100;

			if (aper < Globals->MONSTER_ADVANCE_MIN_PERCENT)
				aper = Globals->MONSTER_ADVANCE_MIN_PERCENT;

			int n = getrandom(100);
			if (n < aper) o->advancing = 1;

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
		forlist(&items) {
			Item *i = (Item*)elem;
			if (!(ItemDefs[i->type].type & IT_MONSTER))
			{
				items.Remove(i);
				delete i;
			}
		}

		if (free > 0) --free;
	}
}

void Unit::SetName(AString *s)
{
	if (!s) return;

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
	describe = 0;

	if (!s) return;

	AString *newname = s->getlegal();
	delete s;
	describe = newname;
}

///@return 0 if there are no men or monsters, else 1
int Unit::IsAlive()
{
	if (type == U_MAGE || type == U_APPRENTICE)
		return GetMen() ? 1 : 0;

	forlist(&items) {
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

int Unit::GetMen(int t)
{
	return items.GetNum(t);
}

int Unit::GetMons()
{
	int n = 0;

	forlist(&items) {
		const Item *i = (const Item*)elem;

		if (ItemDefs[i->type].type & IT_MONSTER)
		{
			n += i->num;
		}
	}

	return n;
}

///@return number of men in unit
int Unit::GetMen()
{
	int n = 0;
	forlist(&items) {
		const Item *i = (const Item*)elem;

		if (ItemDefs[i->type].type & IT_MAN)
			n += i->num;
	}

	return n;
}

///@return number of men or monsters
int Unit::GetSoldiers()
{
	int n = 0;
	forlist(&items) {
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

int Unit::GetTactics()
{
	int retval = GetRealSkill(S_TACTICS);

	forlist(&items) {
		Item *i = (Item*)elem;
		if (ItemDefs[i->type].type & IT_MONSTER)
		{
			const int temp = MonDefs[(ItemDefs[i->type].index)].tactics;
			if (temp > retval) retval = temp;
		}
	}

	return retval;
}

///@return effective observation skill level of this unit
int Unit::GetObservation()
{
	// pull observation skill, with bonuses
	int retval = GetRealSkill(S_OBSERVATION) + GetSkillBonus(S_OBSERVATION);

	// max against any integrated monsters with OBS
	forlist(&items) {
		Item *i = (Item*)elem;

		if (ItemDefs[i->type].type & IT_MONSTER)
		{
			int temp = MonDefs[ItemDefs[i->type].index].obs;
			if (temp > retval) retval = temp;
		}
	}

	return retval;
}

int Unit::GetAttackRiding()
{
	if (type == U_WMON)
	{
		int riding = 0;
		forlist(&items) {
			Item *i = (Item*)elem;

			if (ItemDefs[i->type].type & IT_MONSTER)
			{
				if (ItemDefs[i->type].fly)
				{
					return 5;
				}

				if (ItemDefs[i->type].ride) riding = 3;
			}
		}

		return riding;
	}

	const int riding = GetSkill(S_RIDING);
	int lowriding = 0;

	forlist(&items) {
		Item *i = (Item*)elem;

		// XXX -- Fix this -- not all men weigh the same
		// XXX --			 Use the least weight man in the unit
		if (ItemDefs[i->type].fly - ItemDefs[i->type].weight >= 10)
		{
			return riding;
		}

		// XXX -- Fix this -- Should also be able to carry the man
		if (ItemDefs[i->type].ride - ItemDefs[i->type].weight >= 10)
		{
			if (riding <= 3) return riding;
			lowriding = 3;
		}
	}

	return lowriding;
}

int Unit::GetDefenseRiding()
{
	if (guard == GUARD_GUARD) return 0;

	int riding = 0;
	const int weight = Weight();

	if (CanFly(weight))
	{
		riding = 5;
	}
	else if (CanRide(weight))
		riding = 3;

	if (GetMen())
	{
		int manriding = GetSkill(S_RIDING);
		if (manriding < riding) return manriding;
	}

	return riding;
}

int Unit::GetStealth()
{
	// units on guard aren't hiding
	if (guard == GUARD_GUARD) return 0;

	int monstealth = 100; // lowest monster stealth
	int manstealth = 100; // stealth skill (if any men)

	//foreach item in unit
	forlist(&items) {
		Item *i = (Item*)elem;
		if (ItemDefs[i->type].type & IT_MONSTER)
		{
			const int temp = MonDefs[ItemDefs[i->type].index].stealth;
			if (temp < monstealth) monstealth = temp;
		}
		else if (ItemDefs[i->type].type & IT_MAN)
		{
			if (manstealth == 100)
			{
				manstealth = GetRealSkill(S_STEALTH);
			}
		}
	}

	manstealth += GetSkillBonus(S_STEALTH);

	// return min
	if (monstealth < manstealth) return monstealth;

	return manstealth;
}

int Unit::GetEntertainment()
{
	int level  =     GetRealSkill(S_ENTERTAINMENT);
	int level2 = 5 * GetRealSkill(S_PHANTASMAL_ENTERTAINMENT);

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

///@return effective skill level
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
		forlist(&skills) {
			Skill *s = (Skill*)elem;

			// found one
			if (SkillDefs[s->type].flags & SkillType::MAGIC)
				return; // done
		}

		type = U_NORMAL;
	}

	// check transition from apprentice to normal
	if (type == U_APPRENTICE)
	{
		forlist(&skills) {
			Skill *s = (Skill*)elem;

			if (SkillDefs[s->type].flags & SkillType::APPRENTICE)
				return; // still have one
		}

		type = U_NORMAL;
	}
}

int Unit::CheckDepend(int lev, SkillDepend &dep)
{
	const int temp = GetRealSkill(dep.skill);
	if (temp < dep.level) return 0;

	if (lev >= temp) return 0;

	return 1;
}

///@return 1 if this unit can study 'sk', else 0
int Unit::CanStudy(int sk)
{
	if (SkillDefs[sk].flags & SkillType::DISABLED) return 0;

	int curlev = GetRealSkill(sk);

	for(unsigned c = 0; c < sizeof(SkillDefs[sk].depends)/sizeof(SkillDepend); ++c)
	{
		const int skillIdx = SkillDefs[sk].depends[c].skill;

		if (skillIdx == -1) return 1; // end of list, success

		if (SkillDefs[skillIdx].flags & SkillType::DISABLED)
			continue; // can't depend on disabled skills

		if (!CheckDepend(curlev, SkillDefs[sk].depends[c]))
			return 0;
	}

	return 1;
}

/// perform the "study" order for this unit
///@return 0 on fail, 1 on success
int Unit::Study(int sk, int days)
{
	// if non-leaders can only have one skill, and this is not a leader
	if (Globals->SKILL_LIMIT_NONLEADERS && !IsLeader())
	{
		// if we have a skill already
		if (skills.Num())
		{
			Skill *s = (Skill*)skills.First();

			// if we are not improving our one skill
			if (s->type != sk)
			{
				// and it is not magic related
				if (!Globals->MAGE_NONLEADERS)
				{
					Error("STUDY: Can know only 1 skill.");
					return 0;
				}

				//else, if this is not a magic skill
				if ((SkillDefs[sk].flags & SkillType::MAGIC) == 0)
				{
					Error("STUDY: Can know only 1 skill.");
					return 0;
				}

				//else, if our "one" skill is not magic
				if ((SkillDefs[s->type].flags & SkillType::MAGIC) == 0)
				{
					Error("STUDY: Can know only 1 skill.");
					return 0;
				}
			}
		}
	}

	forlist(&skills) {
		Skill *s = (Skill*)elem;
		if (s->type != sk) continue;

		// find max skill level
		int max = 1000;
		forlist (&items) {
			Item *i = (Item*)elem;

			if (ItemDefs[i->type].type & IT_MAN)
			{
				const int m = SkillMax(s->type, i->type);
				if (m < max) max = m;
			}
		}

		if (GetRealSkill(s->type) >= max)
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

	// Check to see if we need to show a skill report
	int lvl = GetRealSkill(sk);
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
	if (practised || bonus < 1) return 1; // done

	const int days = skills.GetDays(sk);
	const int men = GetMen();
	// zero men, or zero current skill
	if (men < 1 || days < 1)
		return 0; // no practice

	// calculate maximum skill level (min of all man-types)
	int max = 1000;
	forlist (&items) {
		Item *i = (Item*)elem;

		if (ItemDefs[i->type].type & IT_MAN)
		{
			const int m = SkillMax(sk, i->type);
			if (m < max) max = m;
		}
	}

	const int curlev = GetRealSkill(sk);
	if (curlev >= max) return 0; // no practice needed

	for(unsigned i = 0; i < sizeof(SkillDefs[sk].depends)/sizeof(SkillDepend); ++i)
	{
		const int reqsk = SkillDefs[sk].depends[i].skill;
		if (reqsk == -1) break; // done

		if (SkillDefs[reqsk].flags & SkillType::DISABLED) continue;

		const int reqlev = GetRealSkill(reqsk);
		if (reqlev <= curlev)
		{
			if (Practise(reqsk))
				return 1;

			// We don't meet the reqs, and can't practise that req, but
			// we still need to check the other reqs.
			bonus = 0;
		}
	}

	if (bonus)
	{
		Study(sk, men * bonus);
		practised = 1;
	}

	return bonus;
}

///@return 1 if unit contains any leaders
int Unit::IsLeader()
{
	return GetMen(I_LEADERS) ? 1 : 0;
}

int Unit::IsNormal()
{
	if (GetMen() && !IsLeader()) return 1;
	return 0;
}

/// limit the unit to one skill (for non MU)
void Unit::limitSkillNoMagic()
{
	// Find highest skill, eliminate others
	Skill *maxskill = 0; // live-out

	unsigned max = 0; // loop-carry
	forlist(&skills) {
		Skill *s = (Skill*)elem;
		if (s->days > max) {
			max = s->days;
			maxskill = s;
		}
	}

	{
		forlist(&skills) {
			Skill *s = (Skill*)elem;
			if (s != maxskill) {
				skills.Remove(s);
				delete s;
			}
		}
	}
}

void Unit::limitSkillMagic()
{
	forlist(&skills) {
		Skill *s = (Skill*)elem;
		if(SkillDefs[s->type].flags & SkillType::MAGIC)
			continue;

		skills.Remove(s);
		delete s;
	}
}

void Unit::AdjustSkills()
{
	// if we are 100% leaders
	if (IsLeader())
	{
		// Make sure no skills are > max
		forlist(&skills) {
			Skill *s = (Skill*)elem;
			if (GetRealSkill(s->type) >= SkillMax(s->type, I_LEADERS)) {
				s->days = GetDaysByLevel(SkillMax(s->type, I_LEADERS)) *
					GetMen();
			}
		}

		return;
	}
	//else non-leaders

	// if we can only know 1 skill, but know more
	if (Globals->SKILL_LIMIT_NONLEADERS && skills.Num() > 1) {
		if (!Globals->MAGE_NONLEADERS)
		{
			limitSkillNoMagic();
		}
		else
		{
			limitSkillMagic();
		}
	}

	// Limit remaining skills to max
	forlist(&skills) {
		Skill *theskill = (Skill*)elem;

		int max = 100;
		forlist(&items) {
			Item *i = (Item*)elem;

			if (ItemDefs[i->type].type & IT_MAN)
			{
				if (SkillMax(theskill->type, i->type) < max)
				{
					max = SkillMax(theskill->type,i->type);
				}
			}
		}

		if (GetRealSkill(theskill->type) >= max)
		{
			theskill->days = GetDaysByLevel(max) * GetMen();
		}
	}
}

int Unit::MaintCost()
{
	// NPC's don't have maintenance
	if (type == U_WMON || type == U_GUARD || type == U_GUARDMAGE) return 0;

	// Handle leaders
	int leaders = GetMen(I_LEADERS);
	if (leaders < 0) leaders = 0;

	int i = leaders * Globals->LEADER_COST;
	if (Globals->MULTIPLIER_USE != GameDefs::MULT_NONE)
	{
		// higher leader cost: maintenance_multiplier * total skill points
		int multiI = leaders * SkillLevels() * Globals->MAINTENANCE_MULTIPLIER;

		if (multiI > i)
			i = multiI;
	}

	int retval = i;

	// Handle non-leaders
	int nonleaders = GetMen() - leaders;
	if (nonleaders < 0) nonleaders = 0;

	i = nonleaders * Globals->MAINTENANCE_COST;
	if (Globals->MULTIPLIER_USE == GameDefs::MULT_ALL)
	{
		// Non leaders are counted at maintenance_multiplier * skills only if
		// all characters pay that way.
		int multiI = nonleaders * SkillLevels() * Globals->MAINTENANCE_MULTIPLIER;
		if (multiI > i)
			i = multiI;
	}

	retval += i;

	return retval;
}

void Unit::Short(int needed, int hunger)
{
	if (faction->IsNPC())
		return; // Don't starve monsters and the city guard!

	switch (Globals->SKILL_STARVATION)
	{
		case GameDefs::STARVE_MAGES:
			if (type == U_MAGE) SkillStarvation();
			return;

		case GameDefs::STARVE_LEADERS:
			if (GetMen(I_LEADERS)) SkillStarvation();
			return;

		case GameDefs::STARVE_ALL:
			SkillStarvation();
			return;
	}
	//default:

	if (needed < 1 && hunger < 1) return;

	int n = 0, levels;

	for (int i = 0; i <= NITEMS; ++i)
	{
		if (i == I_LEADERS)
		{
			// Don't starve leaders just yet.
			continue;
		}

		if (!(ItemDefs[ i ].type & IT_MAN))
		{
			// Only men need sustenance.
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
				levels = SkillLevels();
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
				if (n) Error(AString(n) + " starve to death.");
				return;
			}
		}
	}

	int i;
	while (GetMen(I_LEADERS))
	{
		if (getrandom(100) < Globals->STARVE_PERCENT)
		{
			SetMen(I_LEADERS, GetMen(I_LEADERS) - 1);
			++n;
		}

		if (Globals->MULTIPLIER_USE != GameDefs::MULT_NONE)
		{
			levels = SkillLevels();
			i = levels * Globals->MAINTENANCE_MULTIPLIER;
			if (i < Globals->LEADER_COST)
				i = Globals->LEADER_COST;
			needed -= i;
		}
		else
			needed -= Globals->LEADER_COST;

		hunger -= Globals->UPKEEP_MINIMUM_FOOD;
		if (needed < 1 && hunger < 1)
		{
			if (n) Error(AString(n) + " starve to death.");
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
	forlist(&items) {
		Item *i = (Item*) elem;
		cap += ItemDefs[i->type].fly * i->num;
	}
   
	return cap;
}

int Unit::RidingCapacity()
{
	int cap = 0;
	forlist(&items) {
		Item *i = (Item*)elem;
		cap += ItemDefs[i->type].ride * i->num;
	}

	return cap;
}

int Unit::SwimmingCapacity()
{
	int cap = 0;
	forlist(&items) {
		Item *i = (Item*)elem;
		cap += ItemDefs[i->type].swim * i->num;
	}

	return cap;
}

int Unit::WalkingCapacity()
{
	int cap = 0;
	forlist(&items) {
		Item *i = (Item*)elem;

		cap += ItemDefs[i->type].walk * i->num;

		if (ItemDefs[i->type].hitchItem != -1)
		{
			int hitch = ItemDefs[i->type].hitchItem;

			if (!(ItemDefs[hitch].flags & ItemType::DISABLED))
			{
				int hitches = items.GetNum(hitch);
				int hitched = i->num;
				if(hitched > hitches) hitched = hitches;

				cap += hitched * ItemDefs[i->type].hitchwalk;
			}
		}
	}

	return cap;
}

int Unit::CanFly(int weight)
{
	if (FlyingCapacity() >= weight) return 1;
	return 0;
}

///@return 1 if this unit can swim
int Unit::CanReallySwim()
{
	return SwimmingCapacity() >= items.Weight() ? 1 : 0;
}

///@return 1 if this unit can cross water
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
	return RidingCapacity() >= weight ? 1 : 0;
}

int Unit::CanWalk(int weight)
{
	return WalkingCapacity() >= weight ? 1 : 0;
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

	if (CanWalk(weight)) return M_WALK;

	return M_NONE;
}

int Unit::CalcMovePoints()
{
	switch (MoveType())
	{
		case M_NONE:
			return 0;

		case M_WALK:
			return Globals->FOOT_SPEED;

		case M_RIDE:
			return Globals->HORSE_SPEED;

		case M_FLY:
			if (GetSkill(S_SUMMON_WIND)) return Globals->FLY_SPEED + 2;

			return Globals->FLY_SPEED;
	}

	return 0;
}

int Unit::CanMoveTo(ARegion *r1, ARegion *r2)
{
	if (r1 == r2) return 1;

	// check exits
	int exit = 1; // can use an exit
	int dir = -1; // which exit to use

	for (int i = 0; exit && i < NDIRS; ++i)
	{
		if (r1->neighbors[i] == r2)
		{
			exit = 0;
			dir = i;
		}
	}

	// if exit check failed
	if (exit) return 0;

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
	if (exit) return 0;

	// check for crossing water
	if (((TerrainDefs[r1->type].similar_type == R_OCEAN) ||
		 (TerrainDefs[r2->type].similar_type == R_OCEAN)) &&
		(!CanSwim() || GetFlag(FLAG_NOCROSS_WATER)))
	{
		return 0;
	}

	// check move cost
	const int mp = CalcMovePoints() - movepoints;
	if (mp < r2->MoveCost(MoveType(), r1, dir, 0))
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

///@return 1 if amulets are available for the men
int Unit::AmtsPreventCrime(Unit *u)
{
	if (!u) return 0;

	int amulets = items.GetNum(I_AMULETOFTS);
	if((u->items.GetNum(I_RINGOFI) < 1) || (amulets < 1)) return 0;

	const int men = GetMen();
	if (men <= amulets) return 1;

	// settings allow a shortage of amulets to sometimes work
	if (!Globals->PROPORTIONAL_AMTS_USAGE) return 0;

	return getrandom(men) < amulets ? 1 : 0;
}

///@return our attitude towards 'u' (meeting in 'r')
int Unit::GetAttitude(ARegion *r, Unit *u)
{
	// ally with ourselves
	if (faction == u->faction) return A_ALLY;

	const int att = faction->GetAttitude(u->faction->num);
	if (att >= A_FRIENDLY && att >= faction->defaultattitude)
		return att;

	if (CanSee(r, u) == 2)
		return att;

	return faction->defaultattitude;
}

int Unit::Hostile()
{
	if (type != U_WMON) return 0;

	int retval = 0;
	forlist(&items) {
		Item *i = (Item*)elem;
		if (ItemDefs[i->type].type & IT_MONSTER)
		{
			const int hos = MonDefs[ItemDefs[i->type].index].hostile;
			if (hos > retval) retval = hos;
		}
	}

	return retval;
}

///@return 1 if this forbids u from entering r, else 0
int Unit::Forbids(ARegion *r, Unit *u)
{
	if (guard != GUARD_GUARD) return 0; // only guards forbid

	if (!IsAlive()) return 0; // dead units can't forbid

	// can't forbid what you can't see
	if (!CanSee(r, u, Globals->SKILL_PRACTISE_AMOUNT > 0)) return 0;

	// can't forbid what you can't catch
	if (!CanCatch(r,u)) return 0;

	// forbid enemies
	if (GetAttitude(r,u) < A_NEUTRAL) return 1;

	return 0;
}

int Unit::Taxers()
{
	int illusions = 0;
	int creatures = 0;

	const int totalMen = GetMen();
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
		int numMelee= 0;
		int numUsableMelee = 0;
		int numBows = 0;
		int numUsableBows = 0;
		int numMounted= 0;
		int numUsableMounted = 0;
		int numMounts = 0;
		int numUsableMounts = 0;
		int numBattle = 0;
		int numUsableBattle = 0;

		forlist (&items) {
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
					if (GetSkill(S_COMBAT)) numUsableMelee += pItem->num;
					numMelee += pItem->num;
				}
				else if (pWep->baseSkill == S_RIDING)
				{
					// A mounted weapon
					if (GetSkill(S_RIDING)) numUsableMounted += pItem->num;
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
			if (Globals->WHO_CAN_TAX &
					GameDefs::TAX_MELEE_WEAPON_AND_MATCHING_SKILL)
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

			if (Globals->WHO_CAN_TAX &
				GameDefs::TAX_BOW_SKILL_AND_MATCHING_WEAPON)
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
	if (taxers > totalMen) taxers = totalMen;

	// And finally for creatures
	if (Globals->WHO_CAN_TAX & GameDefs::TAX_CREATURES)
		taxers += creatures;

	if (Globals->WHO_CAN_TAX & GameDefs::TAX_ILLUSIONS)
		taxers += illusions;

	return taxers;
}

int Unit::GetFlag(int x)
{
	return (flags & x);
}

void Unit::SetFlag(int x, int val)
{
	if (val)
		flags = flags | x;
	else
		if (flags & x) flags -= x;
}

void Unit::CopyFlags(Unit *x)
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
	forlist(&items) {
		Item *pItem = (Item*)elem;

		if (!pItem->num) continue;

		int item = pItem->type;
		if ((ItemDefs[item].type&IT_BATTLE) && ItemDefs[item].battleindex == index)
		{
			// Exclude weapons.  They will be handled later.
			if (ItemDefs[item].type & IT_WEAPON) continue;

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

	forlist(&items) {
		Item *pItem = (Item*)elem;
		if (!pItem->num) continue;

		int item = pItem->type;
		if ((ItemDefs[item].type&IT_ARMOR) && ItemDefs[item].index == index)
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
	bonus = 0;

	if (!canFly && !canRide) return -1;

	//foreach item
	forlist(&items) {
		Item *pItem = (Item*)elem;

		if (!pItem->num) continue;

		int item = pItem->type;
		if ((ItemDefs[item].type&IT_MOUNT) && (ItemDefs[item].index==index))
		{
			// Found a possible mount
			if (canFly)
			{
				if (!ItemDefs[item].fly)
				{
					// The mount cannot fly, see if the region allows
					// riding mounts
					if (!canRide) continue;
				}
			}
			else
			{
				// This region allows riding mounts, so if the mount
				// can not carry at a riding level, continue
				if(!ItemDefs[item].ride) continue;
			}

			MountType *pMnt = &MountDefs[index];
			bonus = GetSkill(pMnt->skill);
			if(bonus < pMnt->minBonus)
			{
				// Unit isn't skilled enough for this mount
				bonus = 0;
				continue;
			}

			// Limit to max mount bonus;
			if(bonus > pMnt->maxBonus) bonus = pMnt->maxBonus;

			// If the mount can fly and the terrain doesn't allow
			// flying mounts, limit the bonus to the maximum hampered
			// bonus allowed by the mount
			if (ItemDefs[item].fly && !canFly)
			{
				if(bonus > pMnt->maxHamperedBonus)
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
	 attackBonus = 0;
	defenseBonus = 0;
	attacks = 1;

	int combatSkill = GetSkill(S_COMBAT);
	forlist(&items) {
		Item *pItem = (Item*)elem;

		if (!pItem->num) continue;

		int item = pItem->type;
		WeaponType *pWep = &WeaponDefs[index];
		if ((ItemDefs[item].type&IT_WEAPON) && (ItemDefs[item].index==index))
		{
			// Found a weapon, check flags and skills
			// returns -1 if weapon cannot be used, else the usable skill level
			int baseSkillLevel = CanUseWeapon(pWep, riding);
			if (baseSkillLevel == -1) continue;

			int flags = pWep->flags;
			// Attack and defense skill
			if(!(flags & WeaponType::NEEDSKILL)) baseSkillLevel = combatSkill;

			attackBonus = baseSkillLevel + pWep->attackBonus;

			if (flags & WeaponType::NOATTACKERSKILL)
				defenseBonus = pWep->defenseBonus;
			else
				defenseBonus = baseSkillLevel + pWep->defenseBonus;

			// Riding bonus
			if(flags & WeaponType::RIDINGBONUS)
				attackBonus += ridingBonus;
			if(flags & (WeaponType::RIDINGBONUSDEFENSE |
						WeaponType::RIDINGBONUS))
				defenseBonus += ridingBonus;

			// Number of attacks
			attacks = pWep->numAttacks;

			// Note: NUM_ATTACKS_SKILL must be > NUM_ATTACKS_HALF_SKILL
			if(attacks >= WeaponType::NUM_ATTACKS_SKILL)
				attacks += baseSkillLevel - WeaponType::NUM_ATTACKS_SKILL;
			else if(attacks >= WeaponType::NUM_ATTACKS_HALF_SKILL)
				attacks += (baseSkillLevel +1)/2 -
					WeaponType::NUM_ATTACKS_HALF_SKILL;

			// Sanity check
			if (attacks == 0) attacks = 1;

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
	if (object) object->units.Remove(this);

	object = toobj;

	// back pointer from object
	if (object) object->units.Add(this);
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
	int men = GetMen();

	switch(sk)
	{
		case S_OBSERVATION:
			if(!men) break;

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

			if ((bonus < 3) &&
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
	int bonus = 0;
	if (ItemDefs[item].mult_item != -1)
		bonus = items.GetNum(ItemDefs[item].mult_item);
	else
		bonus = GetMen();

	if (bonus > GetMen()) bonus = GetMen();

	return bonus * ItemDefs[item].mult_val;
}

int Unit::SkillLevels()
{
	int levels = 0;
	forlist(&skills) {
		Skill *s = (Skill *)elem;
		levels += GetLevelByDays(s->days/GetMen());
	}
	return levels;
}

Skill* Unit::GetSkillObject(int sk)
{
	forlist(&skills) {
		Skill *s = (Skill*)elem;
		if (s->type == sk)
			return s;
	}

	return NULL;
}

/// starvation via forgetting skills
void Unit::SkillStarvation()
{
	// bit mask of which skills can be forgotten
	int can_forget[NSKILLS];
	int count = 0; // number of bits set

	// initialize bit set
	for(int i = 0; i < NSKILLS; ++i)
	{
		if (SkillDefs[i].flags & SkillType::DISABLED)
		{
			can_forget[i] = 0;
			continue;
		}

		if (GetSkillObject(i))
		{
			can_forget[i] = 1;
			++count;
		}
		else
		{
			can_forget[i] = 0;
		}
	}

	for(int i = 0; i < NSKILLS; ++i)
	{
		if (!can_forget[i]) continue;

		Skill *si = GetSkillObject(i);
		for(int j = 0; j < NSKILLS; ++j)
		{
			if (SkillDefs[j].flags & SkillType::DISABLED) continue;

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
					can_forget[j] = 0;
					--count;
				}
			}
		}
	}

	int i;
	if (!count)
	{
		forlist(&items) {
			Item *i = (Item *)elem;
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

	count = getrandom(count)+1;
	for(i = 0; i < NSKILLS; ++i)
	{
		if (!can_forget[i])
			continue;

		if(--count == 0)
		{
			Skill *s = GetSkillObject(i);
			AString temp = AString("Starves and forgets one level of ")+
				SkillDefs[i].name + ".";
			Error(temp);
			switch (GetLevelByDays(s->days))
			{
				case 1:
					s->days -= 30;
					if(s->days <= 0)
						ForgetSkill(i);
					break;

				case 2:
					s->days -= 60;
					break;

				case 3:
					s->days -= 90;
					break;

				case 4:
					s->days -= 120;
					break;

				case 5:
					s->days -= 150;
					break;
			}
		}
	}
}

int Unit::CanUseWeapon(WeaponType *pWep, int riding)
{
	if (riding == -1)
	{
		if(pWep->flags & WeaponType::NOFOOT) return -1;
	}
	else
	{
		if(pWep->flags & WeaponType::NOMOUNT) return -1;
	}

	return CanUseWeapon(pWep);
}

int Unit::CanUseWeapon(WeaponType *pWep)
{
	// we don't care in this case, their combat skill will be used.
	if (!(pWep->flags & WeaponType::NEEDSKILL))
	{
		Practise(S_COMBAT);
		return 0;
	}

	int baseSkillLevel = 0;
	int tempSkillLevel = 0;
	if (pWep->baseSkill != -1) baseSkillLevel = GetSkill(pWep->baseSkill);

	if (pWep->orSkill != -1) tempSkillLevel = GetSkill(pWep->orSkill);

	if (tempSkillLevel > baseSkillLevel)
	{
		baseSkillLevel = tempSkillLevel;
		Practise(pWep->orSkill);
	}
	else
		Practise(pWep->baseSkill);

	if (pWep->flags & WeaponType::NEEDSKILL && !baseSkillLevel) return -1;

	return baseSkillLevel;
}


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
#include "object.h"
#include "aregion.h"
#include "items.h"
#include "skills.h"
#include "gamedata.h"
#include "faction.h"
#include "unit.h"
#include "gamedefs.h"
#include "fileio.h"
#include "astring.h"
#include "gameio.h"
#include <algorithm>
#include <map>

int ParseObject(AString *token)
{
	for (int i = O_DUMMY+1; i < NOBJECTS; ++i)
	{
		if (*token == ObjectDefs[i].name)
		{
			if (ObjectDefs[i].flags & ObjectType::DISABLED)
				return -1;

			return i;
		}
	}

	return -1;
}

int ObjectIsShip(int ot)
{
	if (ot < 0 || ot >= NOBJECTS)
		return 0;

	return ObjectDefs[ot].capacity ? 1 : 0;
}

//----------------------------------------------------------------------------
Object::Object(ARegion *reg)
{
	name = new AString("Dummy");
	describe = NULL;
	region = reg;
	inner = -1;
	num = 0;
	type = O_DUMMY;
	incomplete = 0;
	capacity = 0;
	runes = 0;
	prevdir = -1;
	mages = 0;
}

Object::~Object()
{
	if (!objects.empty())
		std::cerr << "Warning sub-objects not empty for " << name << std::endl;
	delete name;
	delete describe;
	region = NULL;
}

void Object::Writeout(Aoutfile *f)
{
	f->PutInt(num);
	f->PutInt(type);
	f->PutInt(incomplete);
	f->PutStr(*name);
	if (describe)
	{
		f->PutStr(*describe);
	}
	else
	{
		f->PutStr("none");
	}
	f->PutInt(inner);
	f->PutInt(-1);
	if (Globals->PREVENT_SAIL_THROUGH && !Globals->ALLOW_TRIVIAL_PORTAGE)
		f->PutInt(prevdir);
	else
		f->PutInt(-1);
	f->PutInt(runes);
	f->PutInt(units.size());
	for (auto &u : units)
		u->Writeout(f);

	f->PutInt(objects.size());
	{
		for (auto op : objects)
		{
			op->Writeout(f);
			//delete op;
		}
	}
	objects.clear();
}

void Object::Readin(Ainfile *f, AList *facs, ATL_VER v)
{
	num = f->GetInt();
	type = f->GetInt();
	incomplete = f->GetInt();

	delete name;
	name = f->GetStr();

	delete describe;
	describe = f->GetStr();
	if (*describe == "none")
	{
		delete describe;
		describe = NULL;
	}

	inner = f->GetInt();

	const int dummy = f->GetInt();
	if (dummy == -1)
	{
		prevdir = f->GetInt();
		runes = f->GetInt();
	}
	else
		runes = dummy;

	// Now, fix up a save file if ALLOW_TRIVIAL_PORTAGE is allowed, just
	// in case it wasn't when the save file was made.
	if (Globals->ALLOW_TRIVIAL_PORTAGE)
		prevdir = -1;

	const int i = f->GetInt();
	for (int j = 0; j < i; ++j)
	{
		Unit *temp = new Unit;
		temp->Readin(f, facs, v);
		temp->MoveUnit(this);
	}
	mages = ObjectDefs[type].maxMages;

	if (v >= MAKE_ATL_VER(4, 2, 86))
	{
		const int num_objects = f->GetInt();
		for (int i = 0; i < num_objects; ++i)
		{
			Object *op = new Object(region);
			op->Readin(f, facs, v);
			op->parent = this;
			objects.push_back(op);
		}
	}
}

void Object::SetName(AString *s)
{
	if (!s)
		return;

	if (!CanModify())
	{
		delete s;
		return;
	}

	AString *newname = s->getlegal();
	delete s;
	if (!newname)
		return;

	*newname += AString(" [") + num + "]";

	delete name;
	name = newname;
}

void Object::SetDescribe(AString *s)
{
	if (!CanModify())
	{
		delete s;
		return;
	}

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

int Object::IsBoat()
{
	return ObjectIsShip(type);
}

bool Object::hasBoats() const
{
	if (ObjectIsShip(type))
		return true;

	if (type != O_ARMY)
		return false;

	for (auto op : objects)
	{
		if (ObjectIsShip(op->type))
			return true;
	}
	return false;
}

int Object::IsBuilding()
{
	return ObjectDefs[type].protect ? 1 : 0;
}

int Object::CanModify()
{
	return (ObjectDefs[type].flags & ObjectType::CANMODIFY) ? 1 : 0;
}

Unit* Object::GetUnit(int num)
{
	return Unit::findByNum(units, num);
}

Unit* Object::GetUnitAlias(int alias, int faction)
{
	return Unit::findByFaction(units, alias, faction);
}

Unit* Object::GetUnitId(const UnitId *id, int faction)
{
	if (id == NULL)
		return NULL;

	return id->find(units, faction);
}

int Object::CanEnter(ARegion *reg, Unit *u)
{
	if (!(ObjectDefs[type].flags & ObjectType::CANENTER) &&
	    (u->type == U_MAGE || u->type == U_NORMAL || u->type == U_APPRENTICE))
	{
		return 0;
	}
	return 1;
}

Unit* Object::ForbiddenBy(ARegion *reg, Unit *u)
{
	Unit *owner = GetOwner();
	if (!owner)
		return NULL;

	if (owner->GetAttitude(reg, u) < A_FRIENDLY)
		return owner;

	return NULL;
}

Unit* Object::GetOwner()
{
	if (units.empty())
		return nullptr;

	auto i = units.begin();
	Unit *owner = *i;
	while (i != units.end() && !(**i).GetMen())
	{
		owner = *i;
		++i;
	}

	return owner;
}

void Object::Report(Areport *f, Faction *fac, int obs, int truesight,
  int detfac, int passobs, int passtrue, int passdetfac, int present)
{
	ObjectType *ob = &ObjectDefs[type];

	if (type != O_DUMMY && !present)
	{
		if (IsBuilding() &&
		    !(Globals->TRANSIT_REPORT & GameDefs::REPORT_SHOW_BUILDINGS))
		{
			// This is a building and we don't see buildings in transit
			return;
		}

		if (IsBoat() &&
		    !(Globals->TRANSIT_REPORT & GameDefs::REPORT_SHOW_SHIPS))
		{
			// This is a ship and we don't see ships in transit
			return;
		}

		if (IsRoad() &&
		    !(Globals->TRANSIT_REPORT & GameDefs::REPORT_SHOW_ROADS))
		{
			// This is a road and we don't see roads in transit
			return;
		}
	}

	if (type != O_DUMMY)
	{
		bool is_fleet = false;
		AString temp = AString("+ ") + *name + " : ";
		if (type != O_ARMY || objects.empty())
			temp += ob->name;
		else
		{
			is_fleet = true;
			temp += "Fleet";
			std::map<int, unsigned> obj_ct;
			for (Object *op : objects)
			{
				obj_ct[op->type]++;
			}
			for (const auto &mi : obj_ct)
			{
				temp += ", ";
				temp += AString(mi.second);
				temp += " ";
				temp += ObjectDefs[mi.first].name;
			}
		}

		if (incomplete > 0)
		{
			temp += AString(", needs ") + incomplete;
		}
		else if (Globals->DECAY &&
		         !(ob->flags & ObjectType::NEVERDECAY) && incomplete < 1)
		{
			if (incomplete > (0 - ob->maxMonthlyDecay))
			{
				temp += ", about to decay";
			}
			else if (incomplete > (0 - ob->maxMaintenance/2))
			{
				temp += ", needs maintenance";
			}
		}

		if (inner != -1)
			temp += ", contains an inner location";

		if (runes)
			temp += ", engraved with Runes of Warding";

		if (is_fleet || describe)
			temp += AString("; ");

		if (is_fleet)
		{
			bool first = true;
			for (const auto &op : objects)
			{
				if (!first)
					temp += ", ";
				temp += *op->name;
				temp += " ";
				temp += ObjectDefs[op->type].name;

				first = false;
			}
		}
		if (describe)
		{
			if (is_fleet)
				temp += ". ";
			temp += *describe;
		}

		if (!(ob->flags & ObjectType::CANENTER))
			temp += ", closed to player units";

		temp += ".";
		f->PutStr(temp);
		f->AddTab();
	}

	//foreach unit in this
	for (const auto &u : units)
	{
		if (u->faction == fac)
		{
			// self-report
			u->WriteReport(f, -1, 1, 1, 1);
		}
		else
		{
			if (present)
			{
				// foreign unit present
				u->WriteReport(f, obs, truesight, detfac, type != O_DUMMY);
			}
			else
			{
				// foreign unit passing through
				if ((type == O_DUMMY && (Globals->TRANSIT_REPORT & GameDefs::REPORT_SHOW_OUTDOOR_UNITS)) ||
				    (type != O_DUMMY && (Globals->TRANSIT_REPORT & GameDefs::REPORT_SHOW_INDOOR_UNITS)) ||
				    (u->guard == GUARD_GUARD && (Globals->TRANSIT_REPORT & GameDefs::REPORT_SHOW_GUARDS)))
				{
					u->WriteReport(f, passobs, passtrue, passdetfac, type != O_DUMMY);
				}
			}
		}
	}
	f->EndLine();

	if (type != O_DUMMY)
		f->DropTab();
}

void Object::SetPrevDir(int newdir)
{
	prevdir = newdir;
}

void Object::MoveObject(ARegion *toreg)
{
	region->objects.remove(this);

	region = toreg;
	for (auto o : objects)
	{
		o->region = toreg;
	}

	toreg->objects.push_back(this);
}

int Object::IsRoad()
{
	return (O_ROADN <= type && type <= O_ROADS) ? 1 : 0;
}

void Object::addUnit(Unit *u)
{
	units.push_back(u);
}

void Object::prependUnit(Unit *u)
{
	units.insert(units.begin(), u);
}

void Object::removeUnit(Unit *u, bool remove_from_sub)
{
	units.erase(std::remove(units.begin(), units.end(), u), units.end());

	if (!remove_from_sub)
		return;

	// check sub-objects
	for (auto o : objects)
	{
		o->units.erase(std::remove(o->units.begin(), o->units.end(), u));
	}
}

Object* Object::findUnitSubObject(Unit *u)
{
	Object *ret = nullptr;
	for (auto o : objects)
	{
		for(auto &ou : o->units)
		{
			if (ou == u)
				ret = o;
		}
	}
	return ret;
}

void Object::DoDecayClicks(ARegionList *pRegs)
{
	if (ObjectDefs[type].flags & ObjectType::NEVERDECAY)
		return;

	int clicks = getrandom(region->GetMaxClicks()) + region->PillageCheck();

	if (clicks > ObjectDefs[type].maxMonthlyDecay)
		clicks = ObjectDefs[type].maxMonthlyDecay;

	incomplete += clicks;

	if (incomplete > 0)
	{
		// trigger decay event
		region->RunDecayEvent(this, pRegs);
	}

	// apply to subojects
	for (auto o : objects)
		o->DoDecayClicks(pRegs);
}

bool Object::canFly() const
{
	if (type == O_ARMY && !objects.empty())
	{
		// all ships must fly
		for (Object *op : objects)
		{
			if (op->type != O_BALLOON)
				return false;
		}
		return true;
	}
	return type == O_BALLOON; // TODO: generalize
}

AString* ObjectDescription(int obj)
{
	if (ObjectDefs[obj].flags & ObjectType::DISABLED)
		return NULL;

	ObjectType *o = &ObjectDefs[obj];
	AString *temp = new AString;
	*temp += AString(o->name) + ": ";

	if (o->capacity)
		*temp += AString("This is a ship with capacity ") + o->capacity + ".";
	else
		*temp += "This is a building.";

	if (Globals->LAIR_MONSTERS_EXIST && o->monster != -1)
	{
		*temp += " Monsters can potentially lair in this structure.";
		if (o->flags & ObjectType::NOMONSTERGROWTH)
			*temp += " Monsters in this structures will never regenerate.";
	}

	if (o->flags & ObjectType::CANENTER)
		*temp += " Units may enter this structure.";

	if (o->protect)
	{
		*temp += AString(" This structure provides defense to the first ") +
		   o->protect + " men inside it.";
	}

	// Handle all the specials
	for (int i = 0; i < NUMSPECIALS; ++i)
	{
		SpecialType *spd = &SpecialDefs[i];
		if (!(spd->targflags & SpecialType::HIT_BUILDINGIF) &&
		    !(spd->targflags & SpecialType::HIT_BUILDINGEXCEPT))
		{
			continue;
		}

		bool match = false;
		for (int j = 0; j < 3; ++j)
			if (spd->buildings[j] == obj)
				match = true;

		if (!match)
			continue;

		*temp += " Units in this structure are";

		if (spd->targflags & SpecialType::HIT_BUILDINGEXCEPT)
			*temp += " not";

		*temp += AString(" affected by ") + spd->specialname + ".";
	}

	if (o->sailors)
	{
		*temp += AString(" This ship requires ") + o->sailors +
		   " total levels of sailing skill to sail.";
	}

	if (o->maxMages && Globals->LIMITED_MAGES_PER_BUILDING)
	{
		*temp += AString(" This structure will allow up to ") + o->maxMages +
		   " mages to study above level 2.";
	}

	// can this object be built
	bool buildable = true;

	// if there are no inputs, or no skill to build
	if (o->item == -1 || o->skill == -1)
		buildable = false;

	// if the skill is disabled
	if (SkillDefs[o->skill].flags & SkillType::DISABLED)
		buildable = false;

	// wood or stone requires two checks and'ed together
	if (o->item != I_WOOD_OR_STONE &&
	    (ItemDefs[o->item].flags & ItemType::DISABLED))
		buildable = false;

	// check wood and stone disabled
	if (o->item == I_WOOD_OR_STONE &&
	    (ItemDefs[I_WOOD].flags & ItemType::DISABLED) &&
	    (ItemDefs[I_STONE].flags & ItemType::DISABLED))
		buildable = false;

	if (!buildable)
	{
		*temp += " This structure cannot be built by players.";
	}
	else
	{
		*temp += AString(" This structure is built using ") +
		   SkillStrs(o->skill) + " " + o->level + " and requires " +
		   o->cost + " ";

		if (o->item == I_WOOD_OR_STONE)
			*temp += "wood or stone";
		else
			*temp += ItemDefs[o->item].name;

		*temp += " to build.";
	}

	// check if object is a building that increases production
	if (o->productionAided != -1 &&
	    !(ItemDefs[o->productionAided].flags & ItemType::DISABLED))
	{
		*temp += " This trade structure increases the amount of ";

		if (o->productionAided == I_SILVER)
			*temp += "entertainment";
		else
			*temp += ItemDefs[o->productionAided].names;

		*temp += " available in the region.";
	}

	if (Globals->DECAY)
	{
		if (o->flags & ObjectType::NEVERDECAY)
		{
			*temp += " This structure will never decay.";
		}
		else
		{
			*temp += AString(" This structure can take ") + o->maxMaintenance +
			   " units of damage before it begins to decay.";
			*temp += AString(" Damage can occur at a maximum rate of ") +
			   o->maxMonthlyDecay + " units per month.";

			if (buildable)
			{
				*temp += AString(" Repair of damage is accomplished at ") +
					"a rate of " + o->maintFactor + " damage units per " +
					"unit of ";
				if (o->item == I_WOOD_OR_STONE)
				{
					*temp += "wood or stone.";
				}
				else
				{
					*temp += ItemDefs[o->item].name;
				}
			}
		}
	}

	return temp;
}


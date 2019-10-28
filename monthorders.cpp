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
#include "game.h"
#include "faction.h"
#include "unit.h"
#include "gamedata.h"
#include "object.h"
#include "orders.h"
#include <map>

//----------------------------------------------------------------------------
void Game::RunSailOrders()
{
	int tmpError = 0; // loop-carry?

	//foreach region
	forlist(&regions)
	{
		ARegion *r = (ARegion*)elem;

		AList regs; // live-out

		//foreach object in region
		forlist(&r->objects)
		{
			Object *o = (Object*)elem;

			// find the owner of the object
			Unit *u = o->GetOwner();

			// if owner is not sailing
			if (!u || !u->monthorders || u->monthorders->type != O_SAIL ||
			    !o->IsBoat())
			{
				tmpError = 2;
			}
			else if (o->incomplete >= 1)
			{
				// boat is not done
				tmpError = 1;
			}
			else // process the order
			{
				ARegionPtr *p = new ARegionPtr;
				p->ptr = Do1SailOrder(r, o, u);
				regs.Add(p);
			}

			// error reporting
			if (!tmpError)
				continue;

			forlist(&o->units)
			{
				Unit *u2 = (Unit*)elem;
				if (u2->monthorders && u2->monthorders->type == O_SAIL)
				{
					delete u2->monthorders;
					u2->monthorders = NULL;

					switch (tmpError)
					{
						case 1:
							u2->Error("SAIL: Ship is not finished.");
							break;

						case 2:
							u2->Error("SAIL: Owner must sail ship.");
							break;
					}
				}
			}
		}

		// handle battles caused by sailing
		{
			forlist(&regs)
			{
				ARegion *r2 = ((ARegionPtr*)elem)->ptr;
				DoAutoAttacksRegion(r2);
			}
		}
	}
}

/// helper for RunSailOrders
ARegion* Game::Do1SailOrder(ARegion *reg, Object *ship, Unit *cap)
{
	AList facs; // factions present
	int movepoints = Globals->SHIP_SPEED; // can be altered by mages on board
	int wgt = 0; // total cargo weight
	int slr = 0; // effective sailors (sum of men * skill)

	//foreach unit on the ship
	forlist(&ship->units)
	{
		Unit *unit = (Unit*)elem;

		// sailing clears the guard bit
		if (unit->guard == GUARD_GUARD)
			unit->guard = GUARD_NONE;

		if (!GetFaction2(&facs, unit->faction->num))
		{
			FactionPtr *p = new FactionPtr;
			p->ptr = unit->faction;
			facs.Add(p);
		}

		wgt += unit->Weight();

		if (unit->monthorders && unit->monthorders->type == O_SAIL)
		{
			slr += unit->GetSkill(S_SAILING) * unit->GetMen();
			unit->Practise(S_SAILING);
		}

		// if unit can enhance wind
		// TODO - make it easier to add ship types
		const int wind_level = unit->GetSkill(S_SUMMON_WIND);
		if (!wind_level)
			continue;

		int req_level = 2;
		switch (ship->type)
		{
		case O_LONGBOAT:
			req_level = 0;
			break;

		case O_CLIPPER:
		case O_BALLOON:
			req_level = 1;
			break;
		}

		if (wind_level > req_level)
		{
			movepoints = Globals->SHIP_SPEED + 2;
			unit->Event("Casts Summon Wind to aid the ship's progress.");
			unit->Practise(S_SUMMON_WIND);
		}
	} //foreach unit on ship

	SailOrder *o = (SailOrder*)cap->monthorders;
	int moveok = 0; // move error (0 is none)

	if (wgt > ObjectDefs[ship->type].capacity)
	{
		cap->Error("SAIL: Ship is overloaded.");
		moveok = 1;
	} // weight is ok
	else if (slr < ObjectDefs[ship->type].sailors)
	{
		cap->Error("SAIL: Not enough sailors.");
		moveok = 1;
	}
	else
	{
		// start moving ("SAIL N NW SE SW")
		while (o->dirs.Num())
		{
			// pop the first direction
			MoveDir *x = (MoveDir*)o->dirs.First();
			const int i = x->dir;

			o->dirs.Remove(x);
			delete x;

			// pull the adjacent region
			ARegion *newreg = reg->neighbors[i];
			if (!newreg)
			{
				cap->Error("SAIL: Can't sail that way.");
				break;
			}

			// figure out the cost to enter
			int cost = 1; // winter and monsoon cost 2
			if (Globals->WEATHER_EXISTS && newreg->weather != W_NORMAL)
				cost = 2;

			// check terrain for adjacency to ocean
			if (ship->type != O_BALLOON)
			{
				// can't move into a square not adjacent to water
				if (!newreg->IsCoastal())
				{
					cap->Error("SAIL: Can't sail inland.");
					break;
				}

				// can't slide along coast (must return to ocean)
				if (TerrainDefs[reg->type].similar_type != R_OCEAN &&
				    TerrainDefs[newreg->type].similar_type != R_OCEAN)
				{
					cap->Error("SAIL: Can't sail inland.");
					break;
				}

				// check for sailing THROUGH land! always allow retracing steps
				if (Globals->PREVENT_SAIL_THROUGH &&
				    TerrainDefs[reg->type].similar_type != R_OCEAN &&
				    ship->prevdir != -1 &&
				    ship->prevdir != reg->GetRealDirComp(i))
				{
					int d1 = reg->GetRealDirComp(ship->prevdir);
					int d2 = i;
					if (d1 > d2)
					{
						int tmp = d1;
						d1 = d2;
						d2 = tmp;
					}

					int blocked1 = 0;
					for (int k = d1+1; k < d2; ++k)
					{
						ARegion *land1 = reg->neighbors[k];
						if (!land1 || TerrainDefs[land1->type].similar_type != R_OCEAN)
							blocked1 = 1;
					}

					int blocked2 = 0;
					const int sides = NDIRS - 2 - (d2 - d1 - 1);
					for (int l = d2+1; l <= d2 + sides; ++l)
					{
						int dl = l;
						if (dl >= NDIRS)
							dl -= NDIRS;

						ARegion *land2 = reg->neighbors[dl];
						if (!land2 || TerrainDefs[land2->type].similar_type != R_OCEAN)
							blocked2 = 1;
					}

					if (blocked1 && blocked2)
					{
						cap->Error(AString("SAIL: Could not sail ") + DirectionStrs[i] + " from " +
						   reg->ShortPrint(&regions) + ". Cannot sail through land.");
						break;
					}
				}
			}

			if (movepoints < cost)
			{
				cap->Event("SAIL: Can't sail further; remaining moves queued.");

				TurnOrder *tOrder = new TurnOrder;
				tOrder->repeating = 0;

				AString order = "SAIL ";
				order += DirectionAbrs[i];

				forlist(&o->dirs)
				{
					MoveDir *move = (MoveDir *) elem;
					order += " ";
					order += DirectionAbrs[move->dir];
				}

				tOrder->turnOrders.Add(new AString(order));
				cap->turnorders.Insert(tOrder);
				break;
			}

			movepoints -= cost;

			ship->MoveObject(newreg);
			ship->SetPrevDir(i);

			forlist(&facs)
			{
				Faction *f = ((FactionPtr*)elem)->ptr;

				f->Event(*ship->name + AString(" sails from ") +
				      reg->ShortPrint(&regions) + AString(" to ") +
				      newreg->ShortPrint(&regions) + AString("."));
			}

			if (Globals->TRANSIT_REPORT != GameDefs::REPORT_NOTHING)
			{
				// everyone onboard gets to see the sights
				forlist(&ship->units)
				{
					Unit *unit = (Unit*)elem;
					Farsight *f;
					// note the hex being left
					forlist(&reg->passers)
					{
						f = (Farsight*)elem;
						if (f->unit == unit)
						{
							// we moved into here this turn
							f->exits_used[i] = 1;
						}
					}

					// and mark the hex being entered
					f = new Farsight;
					f->faction = unit->faction;
					f->level = 0;
					f->unit = unit;
					f->exits_used[newreg->GetRealDirComp(i)] = 1;
					newreg->passers.Add(f);
				}
			}

			reg = newreg;
			if (newreg->ForbiddenShip(ship))
			{
				cap->faction->Event(*ship->name + AString(" is stopped by guards in ") +
				      newreg->ShortPrint(&regions) + AString("."));
				break;
			}
		}
	}

	// clear out everyone's orders
	{
		forlist(&ship->units)
		{
			Unit *unit = (Unit*)elem;
			if (!moveok)
			{
				unit->alias = 0;
			}

			if (unit->monthorders)
			{
				if ((!moveok && unit->monthorders->type == O_MOVE) ||
				    unit->monthorders->type == O_SAIL)
				{
					delete unit->monthorders;
					unit->monthorders = NULL;
				}
			}
		}
	}

	return reg;
}

void Game::RunTeachOrders()
{
	//foreach region
	forlist((&regions))
	{
		ARegion *r = (ARegion*)elem;

		//foreach object in the region
		forlist((&r->objects))
		{
			Object *obj = (Object*)elem;

			//foreach unit in the object
			forlist((&obj->units))
			{
				Unit *u = (Unit*)elem;

				if (u->monthorders && u->monthorders->type == O_TEACH)
				{
					Do1TeachOrder(r, u);

					delete u->monthorders;
					u->monthorders = NULL;
				}
			}
		}
	}
}

void Game::Do1TeachOrder(ARegion *reg, Unit *unit)
{
	// if leaders exist, only leaders can teach
	if (Globals->LEADERS_EXIST && !unit->IsLeader())
	{
		// small change to handle Ceran's mercs
		if (!unit->GetMen(I_MERC))
		{
			// mercs can teach even though they are not leaders
			// (however, they cannot improve their skills)
			unit->Error("TEACH: Only leaders can teach.");
			return;
		}
	}

	// first pass, find how many to teach
	int students = 0;
	TeachOrder *order = (TeachOrder*)unit->monthorders;
	forlist(&order->targets)
	{
		UnitId *id = (UnitId*)elem;
		Unit *target = reg->GetUnitId(id, unit->faction->num);
		if (!target)
		{
			order->targets.Remove(id);
			unit->Error("TEACH: No such unit.");
			delete id;
			continue;
		}

		if (target->faction->GetAttitude(unit->faction->num) < A_FRIENDLY)
		{
			unit->Error(AString("TEACH: ") + *(target->name) + " is not a member of a friendly faction.");
			order->targets.Remove(id);
			delete id;
			continue;
		}

		if (!target->monthorders || target->monthorders->type != O_STUDY)
		{
			unit->Error(AString("TEACH: ") + *(target->name) + " is not studying.");
			order->targets.Remove(id);
			delete id;
			continue;
		}

		const int sk = ((StudyOrder*)target->monthorders)->skill;
		if (unit->GetRealSkill(sk) <= target->GetRealSkill(sk))
		{
			unit->Error(AString("TEACH: ") + *(target->name) + " is not studying " "a skill you can teach.");
			order->targets.Remove(id);
			delete id;
			continue;
		}

		students += target->GetMen();
	}

	if (!students)
		return; // nothing to do

	int days = 30 * unit->GetMen() * Globals->STUDENTS_PER_TEACHER;

	// we now have a list of valid targets
	{
		forlist(&order->targets)
		{
			UnitId *id = (UnitId*)elem;

			Unit *u = reg->GetUnitId(id, unit->faction->num);
			const int umen = u->GetMen();

			// limit boost locally to 30 days
			int tempdays = (umen * days) / students;
			if (tempdays > 30 * umen)
				tempdays = 30 * umen;

			days -= tempdays;
			students -= umen;

			StudyOrder *o = (StudyOrder*)u->monthorders;
			o->days += tempdays;

			// limit boost globally to 30 days
			if (o->days > 30 * umen)
			{
				days += o->days - 30 * umen;
				o->days = 30 * umen;
			}

			unit->Event(AString("Teaches ") + SkillDefs[o->skill].name + " to " + *u->name + ".");

			// the TEACHER may learn something in the process!
			unit->Practise(o->skill);
		}
	}
}

void Game::Run1BuildOrder(ARegion *r, Object *obj, Unit *u)
{
	if (!TradeCheck(r, u->faction))
	{
		u->Error("BUILD: Faction can't produce in that many regions.");

		delete u->monthorders;
		u->monthorders = NULL;

		return;
	}

	const int sk = ObjectDefs[obj->type].skill;
	if (sk == -1)
	{
		u->Error("BUILD: Can't build that.");

		delete u->monthorders;
		u->monthorders = NULL;

		return;
	}

	const int usk = u->GetSkill(sk);
	if (usk < ObjectDefs[obj->type].level)
	{
		u->Error("BUILD: Can't build that.");

		delete u->monthorders;
		u->monthorders = NULL;

		return;
	}

	int needed = obj->incomplete;
	int type   = obj->type;

	if (((ObjectDefs[type].flags & ObjectType::NEVERDECAY) || !Globals->DECAY) && needed < 1)
	{
		u->Error("BUILD: Object is finished.");

		delete u->monthorders;
		u->monthorders = NULL;

		return;
	}

	if (needed <= -(ObjectDefs[type].maxMaintenance))
	{
		u->Error("BUILD: Object does not yet require maintenance.");

		delete u->monthorders;
		u->monthorders = NULL;

		return;
	}

	const int it = ObjectDefs[type].item;

	int itn;
	if (it == I_WOOD_OR_STONE)
	{
		itn = u->canConsume(I_WOOD, needed) + u->canConsume(I_STONE, needed);
	}
	else
	{
		itn = u->canConsume(it, needed);
	}

	if (itn == 0)
	{
		u->Error("BUILD: Don't have the required item.");

		delete u->monthorders;
		u->monthorders = NULL;

		return;
	}

	int num = u->GetMen() * usk;

	if (obj->incomplete == ObjectDefs[type].cost)
	{
		if (ObjectIsShip(type))
		{
			obj->num = shipseq++;
			obj->SetName(new AString("Ship"));
		}
		else
		{
			obj->num = u->object->region->buildingseq++;
			obj->SetName(new AString("Building"));
		}
	}

	// fix bogus ship numbers
	if (ObjectIsShip(type) && obj->num < 100)
	{
		obj->num = shipseq++;
		obj->SetName(new AString("Ship"));
	}

	AString job;
	if (needed < 1)
	{
		// (this looks wrong, but isn't)
		// if a building has a maxMaintainence of 75 and is at -70 (ie, 5
		// from max) then the value of maintMax is 5. Next we divide by
		// maintFactor (some things are easier to repair) to get how many items
		// we need to fix it. Then we fix it by that many items * maintFactor
		int maintMax = ObjectDefs[type].maxMaintenance + needed;
		maintMax /= ObjectDefs[type].maintFactor;

		if (num > maintMax)
			num = maintMax;

		if (itn < num)
			num = itn;

		job = " maintenance ";

		obj->incomplete -= (num * ObjectDefs[type].maintFactor);

		if (obj->incomplete < -(ObjectDefs[type].maxMaintenance))
		{
			obj->incomplete = -(ObjectDefs[type].maxMaintenance);
		}
	}
	else if (needed > 0)
	{
		if (num > needed)
			num = needed;

		if (itn < num)
			num = itn;

		job = " construction ";

		obj->incomplete -= num;
		if (obj->incomplete == 0)
		{
			obj->incomplete = -(ObjectDefs[type].maxMaintenance);
		}
	}

	// perform the build
	//--- move unit into object under construction
	u->MoveUnit(obj);

	if (it == I_WOOD_OR_STONE)
	{
		// use stone first
		const int amt_stone = u->canConsume(I_STONE, num);
		if (num > amt_stone)
		{
			// use all the stone we have
			num -= amt_stone;
			u->consume(I_STONE, amt_stone);
			u->consume(I_WOOD, num);
		}
		else
		{
			u->consume(I_STONE, num);
		}
	}
	else
	{
		u->consume(it, num);
	}

	u->Event(AString("Performs") + job + "on " + *(obj->name) + ".");
	u->Practise(sk);

	delete u->monthorders;
	u->monthorders = NULL;
}

void Game::RunBuildHelpers(ARegion *r)
{
	//foreach object in the region
	forlist((&r->objects))
	{
		Object *obj = (Object*)elem;

		//foreach unit in the object
		forlist((&obj->units))
		{
			Unit *u = (Unit*)elem;

			// if unit is not building
			if (!u->monthorders || u->monthorders->type != O_BUILD)
				continue; // done

			BuildOrder *o = (BuildOrder*)u->monthorders;

			Object *tarobj = NULL;

			if (o->target)
			{
				Unit *target = r->GetUnitId(o->target, u->faction->num);
				if (!target)
				{
					u->Error("BUILD: No such unit to help.");

					delete u->monthorders;
					u->monthorders = NULL;

					continue;
				}

				// make sure that unit is building
				if (target->monthorders && target->monthorders->type != O_BUILD)
				{
					u->Error("BUILD: Unit isn't building.");

					delete u->monthorders;
					u->monthorders = NULL;

					continue;
				}

				// make sure that unit considers you friendly!
				if (target->faction->GetAttitude(u->faction->num) < A_FRIENDLY)
				{
					u->Error("BUILD: Unit you are helping rejects your help.");

					delete u->monthorders;
					u->monthorders = NULL;

					continue;
				}

				tarobj = target->build;
				if (!tarobj)
					tarobj = target->object;

				if (u->object != tarobj)
					u->MoveUnit(tarobj);
			}
			else if (u->build && u->build != u->object)
			{
				u->MoveUnit(u->build);
			}

		} //foreach unit in the object
	} //foreach object in the region
}

void Game::RunMonthOrders()
{
	forlist(&regions)
	{
		ARegion *r = (ARegion*)elem;

		RunStudyOrders(r);
		RunBuildHelpers(r);
		RunProduceOrders(r);
	}
}

/// Intermediate results of production processing
struct ProduceIntermediates
{
	int menUsed_;
	int totalItemsUsed_;
	std::map<int, int> manMonthsAvail_; // skill -> man months
	std::map<int, int> bonusItemsUsed_; // item idx -> count

	ProduceIntermediates()
	: menUsed_(0)
	, totalItemsUsed_(0)
	{
	}
};

/// process PRODUCE order for 'u' in 'r'
///@return true if order is finished
bool Game::RunUnitProduce(ARegion *r, Unit *u, ProduceOrder *o, ProduceIntermediates *pi)
{
	if (o->item == I_SILVER)
	{
		return true;
	}

	const ItemType &item = ItemDefs[o->item];
	const int input = ItemDefs[o->item].pInput[0].item;
	if (input == -1)
	{
		u->Error(AString("PRODUCE: ") + item.abr + ": not present in this region.");

		return true;
	}

	const int level = u->GetSkill(o->skill);
	if (level < ItemDefs[o->item].pLevel)
	{
		u->Error(AString("PRODUCE: ") + item.abr + ": Insufficient skill to produce that.");

		return true;
	}

	if (!TradeCheck(r, u->faction))
	{
		u->Error("PRODUCE: Faction can't produce in that many regions.");

		return true;
	}

	// find the max we can possibly produce based on man-months of labor
	const int base_men = u->GetMen();
	const int num_men = base_men - pi->menUsed_;
	const int bonus_item_idx = ItemDefs[o->item].mult_item;
	const int bonus_item_boost = ItemDefs[o->item].mult_val;

	int num_bonus_items = num_men; // assume item -1 (no item for bonus)
	std::map<int, int>::iterator labor_remainder = pi->manMonthsAvail_.find(o->skill);
	std::map<int, int>::iterator bonus_item_iter = pi->bonusItemsUsed_.end();
	if (labor_remainder == pi->manMonthsAvail_.end())
	{
		labor_remainder = pi->manMonthsAvail_.insert(std::make_pair(o->skill, 0)).first;
	}

	int maxproduced;
	// if production is based on inputs * skill
	if (ItemDefs[o->item].flags & ItemType::SKILLOUT)
	{
		maxproduced = num_men; // skill applied later
	}
	else
	{
		// (number of men * skill level + bonus) / man months per item

		// start with bonus items
		if (bonus_item_idx != -1)
		{
			// start with total items
			int num_items_avail = u->items.GetNum(bonus_item_idx);

			// check for items used
			bonus_item_iter = pi->bonusItemsUsed_.find(bonus_item_idx);
			if (bonus_item_iter != pi->bonusItemsUsed_.end())
			{
				// pull them out
				num_items_avail -= bonus_item_iter->second;
			}
			else
				bonus_item_iter = pi->bonusItemsUsed_.insert(std::make_pair(bonus_item_idx, 0)).first;

			// no point using more than 1 per man
			num_bonus_items = std::min(num_items_avail, base_men - pi->totalItemsUsed_);
		}

		const int number = num_men * level +
		    num_bonus_items * bonus_item_boost +
		    labor_remainder->second;

		maxproduced = number / ItemDefs[o->item].pMonths;
	}

	// apply production limit
	if (o->limit != -1 && maxproduced > o->limit)
	{
		maxproduced = o->limit;
	}

	// if item only requires one of its inputs
	if (ItemDefs[o->item].flags & ItemType::ORINPUTS)
	{
		// figure out the max we can produce based on the inputs
		int count = 0;
		for (unsigned c = 0; c < sizeof(ItemDefs->pInput)/sizeof(Materials); ++c)
		{
			const int i = ItemDefs[o->item].pInput[c].item;
			if (i == -1)
				continue;

			const int amt_req = ItemDefs[o->item].pInput[c].amt;
			count += u->canConsume(i, amt_req * (maxproduced - count)) / amt_req;
		}

		if (maxproduced > count)
			maxproduced = count;

		// deduct the items spent
		count = maxproduced;
		for (unsigned c = 0; c < sizeof(ItemDefs->pInput)/sizeof(Materials); ++c)
		{
			const int i = ItemDefs[o->item].pInput[c].item;
			if (i == -1)
				continue;

			// how much is needed per
			const int a = ItemDefs[o->item].pInput[c].amt;

			// how much can we get
			const int amt = u->canConsume(i, count * a);

			const int consumed = amt / a;
			if (count > consumed)
			{
				// not enough
				count -= consumed;
				u->consume(i, consumed * a);
			}
			else // enough
			{
				u->consume(i, count * a);
				count = 0;
			}
		}
	}
	else // AND inputs
	{
		// figure out the max we can produce based on all inputs
		for (unsigned c = 0; c < sizeof(ItemDefs->pInput)/sizeof(Materials); ++c)
		{
			const int i = ItemDefs[o->item].pInput[c].item;
			if (i == -1)
				continue;

			const int a = ItemDefs[o->item].pInput[c].amt;
			const int amt = u->canConsume(i, a * maxproduced);
			if (amt / a < maxproduced)
			{
				maxproduced = amt / a;
			}
		}

		// deduct the items spent
		for (unsigned c = 0; c < sizeof(ItemDefs->pInput)/sizeof(Materials); ++c)
		{
			const int i = ItemDefs[o->item].pInput[c].item;
			if (i == -1)
				continue;

			const int a = ItemDefs[o->item].pInput[c].amt;
			u->consume(i, maxproduced * a);
		}
	}

	// back calculate number of men working
	if (ItemDefs[o->item].flags & ItemType::SKILLOUT)
	{
		pi->menUsed_ = maxproduced;
	}
	else // man months with item boost
	{
		int req_labor = maxproduced * ItemDefs[o->item].pMonths;

		// check for labor remainder
		if (labor_remainder->second >= req_labor)
		{
			labor_remainder->second -= req_labor;
			req_labor = 0;
			// done
		}
		else
		{
			req_labor -= labor_remainder->second;
			labor_remainder->second = 0;

			int men_req = 0;
			int bonus_items_used = 0;

			// use items, and men for items
			while (bonus_items_used < num_bonus_items)
			{
				// use item
				++bonus_items_used;

				// if labor is less than item boost
				if (bonus_item_boost >= req_labor)
				{
					req_labor = 0;
					break;
				}
				req_labor -= bonus_item_boost;

				// man to use the item
				++men_req;
				if (level >= req_labor)
				{
					labor_remainder->second = level - req_labor;
					req_labor = 0;
					break;
				}
				req_labor -= level;
			}

			if (req_labor)
			{
				// calculate men without items
				const int men_no_items = (req_labor + level - 1) / level;
				men_req += men_no_items;
				labor_remainder->second = men_no_items * level - req_labor;
			}

			pi->menUsed_ += men_req;

			if (bonus_item_iter != pi->bonusItemsUsed_.end())
				bonus_item_iter->second += bonus_items_used;
			pi->totalItemsUsed_ += bonus_items_used;
		}
	}

	// now give the items produced
	int output = maxproduced * ItemDefs[o->item].pOut;
	if (ItemDefs[o->item].flags & ItemType::SKILLOUT)
		output *= level;

	u->items.SetNum(o->item, u->items.GetNum(o->item) + output);

	u->Event(AString("Produces ") + ItemString(o->item, output) + " in " + r->ShortPrint(&regions) + ".");
	u->Practise(o->skill);

	if (o->limit == -1)
		return true; // one turn of production

	o->limit -= output;

	return o->limit == 0;
}

void Game::RunProduceOrders(ARegion *r)
{
	// produce raw materials
	{
		forlist((&r->products))
			RunAProduction(r, (Production*)elem);
	}

	// produce finished goods
	{
		//foreach object
		forlist((&r->objects))
		{
			Object *obj = (Object*)elem;

			//foreach unit in the object
			forlist((&obj->units))
			{
				Unit *u = (Unit*)elem;
				if (!u->monthorders)
					continue;

				if (u->monthorders->type == O_PRODUCE)
				{
					ProduceQueue *q = dynamic_cast<ProduceQueue*>(u->monthorders);
					ProduceIntermediates pi;
					if (!q)
					{
						// normal PRODUCE order
						ProduceOrder *o = (ProduceOrder*)u->monthorders;

						const bool ret = RunUnitProduce(r, u, o, &pi);
						if (ret)
						{
							delete u->monthorders;
							u->monthorders = NULL;
						}
					}
					else // produce queue
					{
						bool done = false;
						do
						{
							// peek at first order
							ProduceOrder *o = &q->orders_[0];
							const ItemType &item = ItemDefs[o->item];
							if (item.pInput[0].item == -1)
							{
								u->Error(AString("PRODUCE: ") + item.abr + ": can't use multiple PRODUCE commands with raw materials.");
								q->orders_.pop_front();
								// if all done
								if (q->orders_.empty())
								{
									delete u->monthorders;
									u->monthorders = NULL;
									done = true;
								}
								continue;
							}

							// run it
							const bool ret = RunUnitProduce(r, u, o, &pi);

							// if finished this one
							if (ret)
							{
								q->orders_.pop_front();

								// if all done
								if (q->orders_.empty())
								{
									delete u->monthorders;
									u->monthorders = NULL;
									done = true;
								}
							}
							else // could not finish current
								done = true;

						} while (!done);
					}
				}
				else if (u->monthorders->type == O_BUILD)
				{
					Run1BuildOrder(r, obj, u);
				}
			}
		}
	}
}

int Game::ValidProd(Unit *u, ARegion *r, Production *p)
{
	if (u->monthorders->type != O_PRODUCE)
		return 0;

	// cannot queue raw materials
	if (dynamic_cast<ProduceQueue*>(u->monthorders))
		return 0;

	ProduceOrder *po = (ProduceOrder*)u->monthorders;
	if (p->itemtype != po->item || p->skill != po->skill)
		return 0;

	// check level requirement
	const int level = p->skill != -1 ? u->GetSkill(p->skill) : 1;

	const ItemType &item_def = ItemDefs[p->itemtype];

	if (p->skill != -1 && level < item_def.pLevel)
	{
		u->Error("PRODUCE: Unit isn't skilled enough.");

		delete u->monthorders;
		u->monthorders = NULL;

		return 0;
	}

	// check faction limits on production. If the item is silver, then the
	// unit is entertaining or working, and the limit does not apply
	// (note this function increments number of trade regions)
	if (p->itemtype != I_SILVER && !TradeCheck(r, u->faction))
	{
		u->Error("PRODUCE: Faction can't produce in that many regions.");

		delete u->monthorders;
		u->monthorders = NULL;

		return 0;
	}

	// check for bonus production
	const int bonus = u->GetProductionBonus(p->itemtype);

	const int div = item_def.pMonths == 0 ? 1 : item_def.pMonths;

	po->productivity = ((u->GetMen() * level * p->productivity) + bonus) / div;
	return po->productivity;
}

int Game::FindAttemptedProd(ARegion *r, Production *p)
{
	int attempted = 0;
	forlist((&r->objects))
	{
		Object *obj = (Object*)elem;

		forlist((&obj->units))
		{
			Unit *u = (Unit*)elem;

			if (u->monthorders)
			{
				attempted += ValidProd(u, r, p);
			}
		}
	}
	return attempted;
}

/// produce raw material 'p' in region 'r'
void Game::RunAProduction(ARegion *r, Production *p)
{
	p->activity = 0; // start with 0
	if (p->amount == 0)
		return; // none available

	// first, see how many units are trying to work
	int attempted = FindAttemptedProd(r, p);
	int amt = p->amount; // amount remaining for production

	// pretend we will produce all for distribution purposes
	if (attempted < amt)
		attempted = amt;

	//foreach object in the region
	forlist((&r->objects))
	{
		Object *obj = (Object*)elem;

		//foreach unit in the object
		forlist((&obj->units))
		{
			Unit *u = (Unit*)elem;

			if (!u->monthorders || u->monthorders->type != O_PRODUCE)
				continue;

			// can't queue raw materials
			if (dynamic_cast<ProduceQueue*>(u->monthorders))
				continue;

			ProduceOrder *po = (ProduceOrder*)u->monthorders;
			if (po->skill != p->skill || po->item != p->itemtype)
				continue;

			// we need to implement a hack to avoid overflowing
			int ubucks = 0;

			int uatt = po->productivity;
			if (uatt && amt && attempted)
			{
				double dUbucks = ((double) amt) * ((double) uatt) / ((double) attempted);
				ubucks = (int) dUbucks;
			}

			amt -= ubucks;
			attempted -= uatt;

			u->items.SetNum(po->item, u->items.GetNum(po->item) + ubucks);
			p->activity += ubucks;

			// show in unit's events section
			if (po->item == I_SILVER)
			{
				// WORK
				if (po->skill == -1)
				{
					u->Event(AString("Earns ") + ubucks + " silver working in " + r->ShortPrint(&regions) + ".");
				}
				else
				{
					// ENTERTAIN
					u->Event(AString("Earns ") + ubucks + " silver entertaining in " + r->ShortPrint(&regions) + ".");

					// if they don't have PHEN, then this will fail safely
					u->Practise(S_PHANTASMAL_ENTERTAINMENT);
					u->Practise(S_ENTERTAINMENT);
				}
			}
			else
			{
				// everything else
				u->Event(AString("Produces ") + ItemString(po->item, ubucks) + " in " + r->ShortPrint(&regions) + ".");
				u->Practise(po->skill);
			}

			delete u->monthorders;
			u->monthorders = NULL;
		}
	}
}

void Game::RunStudyOrders(ARegion *r)
{
	forlist((&r->objects))
	{
		Object *obj = (Object*)elem;

		forlist((&obj->units))
		{
			Unit *u = (Unit*)elem;

			if (u->monthorders && u->monthorders->type == O_STUDY)
			{
				Do1StudyOrder(u, obj);

				delete u->monthorders;
				u->monthorders = NULL;
			}
		}
	}
}

void Game::Do1StudyOrder(Unit *u, Object *obj)
{
	// Ceran Mercs
	if (u->GetMen(I_MERC))
	{
		u->Error("STUDY: Mercenaries are not allowed to study.");
		return;
	}

	StudyOrder *o = (StudyOrder*)u->monthorders;
	const int sk = o->skill;

	const int cost = SkillCost(sk) * u->GetMen();
	if (cost > u->canConsume(I_SILVER, cost))
	{
		u->Error("STUDY: Not enough funds.");
		return;
	}

	int reset_man = -1; // old unit type

	// check for limits on magic
	if ((SkillDefs[sk].flags & SkillType::MAGIC) && u->type != U_MAGE)
	{
		if (u->type == U_APPRENTICE)
		{
			u->Error("STUDY: An apprentice cannot be made into an mage.");
			return;
		}

		if (Globals->FACTION_LIMIT_TYPE != GameDefs::FACLIM_UNLIMITED)
		{
			if (CountMages(u->faction) >= AllowedMages(u->faction))
			{
				u->Error("STUDY: Can't have another magician.");
				return;
			}
		}

		if (u->GetMen() != 1)
		{
			u->Error("STUDY: Only 1-man units can be magicians.");
			return;
		}

		if (!Globals->MAGE_NONLEADERS && u->GetMen(I_LEADERS) != 1)
		{
			u->Error("STUDY: Only leaders may study magic.");
			return;
		}

		reset_man = u->type;
		u->type = U_MAGE;
	}

	// check for limits on apprentices
	if ((SkillDefs[sk].flags & SkillType::APPRENTICE) && u->type != U_APPRENTICE)
	{
		if (u->type == U_MAGE)
		{
			u->Error("STUDY: A mage cannot be made into an apprentice.");
			return;
		}

		if (Globals->FACTION_LIMIT_TYPE != GameDefs::FACLIM_UNLIMITED)
		{
			if (CountApprentices(u->faction) >= AllowedApprentices(u->faction))
			{
				u->Error("STUDY: Can't have another apprentice.");
				return;
			}
		}

		if (u->GetMen() != 1)
		{
			u->Error("STUDY: Only 1-man units can be apprentices.");
			return;
		}

		if (!Globals->MAGE_NONLEADERS && u->GetMen(I_LEADERS) != 1)
		{
			u->Error("STUDY: Only leaders may be apprentices.");
			return;
		}

		reset_man = u->type;
		u->type = U_APPRENTICE;
	}

	int days = 30 * u->GetMen() + o->days;

	if ((SkillDefs[sk].flags & SkillType::MAGIC) && u->GetSkill(sk) >= 2)
	{
		if (Globals->LIMITED_MAGES_PER_BUILDING)
		{
			if (obj->incomplete > 0 || obj->type == O_DUMMY)
			{
				u->Error("Warning: Magic study rate outside of a building cut in half above level 2.");
				days /= 2;
			}
			else if (obj->mages == 0)
			{
				u->Error("Warning: Magic rate cut in half above level 2 due to number of mages studying in structure.");
				days /= 2;
			}
			else
			{
				obj->mages--;
			}
		}
		else if (!ObjectDefs[obj->type].protect || obj->incomplete > 0)
		{
			u->Error("Warning: Magic study rate outside of a building cut in half above level 2.");
			days /= 2;
		}
	}

	if (SkillDefs[sk].flags & SkillType::SLOWSTUDY)
	{
		days /= 2;
	}

	if (u->Study(sk, days))
	{
		u->consume(I_SILVER, cost);
		u->Event(AString("Studies ") + SkillDefs[sk].name + ".");
	}
	else
	{
		// if we just tried to become a mage or apprentice, but
		// were unable to study, reset unit to whatever it was before
		if (reset_man != -1)
			u->type = reset_man;
	}
}

void Game::RunMoveOrders()
{
	for (int phase = 0; phase < Globals->MAX_SPEED; ++phase)
	{
		// before processing region to region moves, process leading OUT/ENTER
		{
			//foreach region
			forlist((&regions))
			{
				ARegion *region = (ARegion*)elem;

				//foreach object
				forlist((&region->objects))
				{
					Object *obj = (Object*)elem;

					//foreach unit
					forlist(&obj->units)
					{
						Unit *unit = (Unit*)elem;

						Object *tempobj = obj;
						DoMoveEnter(unit, region, &tempobj);
					}
				}
			}
		}

		// now process actual moves
		AList *locs = new AList;

		//foreach region
		forlist((&regions))
		{
			ARegion *region = (ARegion*)elem;

			//foreach object
			forlist((&region->objects))
			{
				Object *obj = (Object*)elem;

				//foreach unit
				forlist(&obj->units)
				{
					Unit *unit = (Unit*)elem;

					if (phase == unit->movepoints && unit->monthorders &&
					    (unit->monthorders->type == O_MOVE ||
					     unit->monthorders->type == O_ADVANCE) &&
					    !unit->nomove)
					{
						locs->Add(DoAMoveOrder(unit, region, obj));
					}
				}
			}
		}

		DoAdvanceAttacks(locs);
		locs->DeleteAll();
	}
}

// process all the leading OUT and ENTER moves for 'unit'
void Game::DoMoveEnter(Unit *unit, ARegion *region, Object **obj)
{
	if (!unit->monthorders ||
	    (unit->monthorders->type != O_MOVE &&
	     unit->monthorders->type != O_ADVANCE))
	{
		return;
	}

	MoveOrder *o = (MoveOrder*)unit->monthorders;

	// e.g. "MOVE IN N SE NW"
	while (o->dirs.Num())
	{
		// get the first direction
		MoveDir *x = (MoveDir*)o->dirs.First();
		const int i = x->dir;

		// only handle OUT/ENTER
		if (i != MOVE_OUT && i < MOVE_ENTER)
			return;

		o->dirs.Remove(x);
		delete x;

		if (i == MOVE_OUT)
		{
			if (TerrainDefs[region->type].similar_type == R_OCEAN &&
			    (!unit->CanSwim() ||
			     unit->GetFlag(FLAG_NOCROSS_WATER)))
			{
				unit->Error("MOVE: Can't leave ship.");
				continue;
			}

			Object *to = region->GetDummy();
			unit->MoveUnit(to);
			*obj = to;

			continue;
		}

		Object *to = region->GetObject(i - MOVE_ENTER);
		if (!to)
		{
			unit->Error("MOVE: Can't find object.");
			continue;
		}

		if (!to->CanEnter(region, unit))
		{
			unit->Error("ENTER: Can't enter that.");
			continue;
		}

		Unit *forbid = to->ForbiddenBy(region, unit);
		if (forbid && !o->advancing)
		{
			unit->Error("ENTER: Is refused entry.");
			continue;
		}

		if (forbid && region->IsSafeRegion())
		{
			unit->Error("ENTER: No battles allowed in safe regions.");
			continue;
		}

		if (forbid && !(unit->IsAlive() && unit->canattack))
		{
			unit->Error(AString("ENTER: Unable to attack ") + *(forbid->name));
			continue;
		}

		bool done = false;
		while (forbid)
		{
			const int result = RunBattle(region, unit, forbid, 0, 0);
			if (result == BATTLE_IMPOSSIBLE)
			{
				unit->Error(AString("ENTER: Unable to attack ")+ *(forbid->name));
				done = true;
				break;
			}

			if (!unit->IsAlive() || !unit->canattack)
			{
			  done = true;
			  break;
			}

			forbid = to->ForbiddenBy(region, unit);
		}

		if (done)
			continue;

		unit->MoveUnit(to);
		unit->Event(AString("Enters ") + *(to->name) + ".");
		*obj = to;
	}
}

bool oceanIsCoastal(ARegion *reg)
{
	for (unsigned i = 0; i < NDIRS; ++i)
	{
		ARegion *nreg = reg->neighbors[i];
		if (nreg && TerrainDefs[nreg->type].similar_type != R_OCEAN)
			return true;
	}
	return false;
}

// process MOVE order for 'unit'
Location* Game::DoAMoveOrder(Unit *const unit, ARegion *const region, Object *const obj)
{
	MoveOrder *o = (MoveOrder*)unit->monthorders;

	// moving clears guard bit
	if (unit->guard == GUARD_GUARD)
		unit->guard = GUARD_NONE;

	if (o->advancing)
		unit->guard = GUARD_ADVANCE;

	Location *loc = new Location;
	loc->unit = unit;
	loc->region = region;
	loc->obj = obj;

	// TODO can this happen?
	// if move with no direction
	if (!o->dirs.Num())
	{
		delete unit->monthorders;
		unit->monthorders = NULL;
		return loc;
	}

	// pop first move order
	MoveDir *x = (MoveDir*)o->dirs.First();
	const int i = x->dir;
	o->dirs.Remove(x);
	delete x;

	if (i == MOVE_IN && obj->inner == -1)
	{
		unit->Error("MOVE: Can't move IN there.");
		delete unit->monthorders;
		unit->monthorders = NULL;
		return loc;
	}

	// setup region to move to
	ARegion *newreg = (i == MOVE_IN) ? regions.GetRegion(obj->inner) : region->neighbors[i];
	if (!newreg)
	{
		unit->Error(AString("MOVE: Can't move that direction."));
		delete unit->monthorders;
		unit->monthorders = NULL;
		return loc;
	}

	// simple barriers to entry
	if (TerrainDefs[region->type].similar_type == R_OCEAN &&
	    (!unit->CanSwim() || unit->GetFlag(FLAG_NOCROSS_WATER)))
	{
		unit->Error(AString("MOVE: Can't move while in the ocean."));
		delete unit->monthorders;
		unit->monthorders = NULL;
		return loc;
	}

	// ok, now we can move to a new region
	int startmove = 0;
	if (region->type == R_NEXUS && newreg->IsStartingCity())
		startmove = 1;

	// calculate movement cost
	const int movetype = unit->MoveType();
	AString road;
	const int cost = newreg->MoveCost(movetype, region, i, &road);

	if (region->type != R_NEXUS && unit->CalcMovePoints() - unit->movepoints < cost)
	{
		if (unit->MoveType() == M_NONE)
		{
			unit->Error("MOVE: Unit is overloaded and cannot move.");
			delete unit->monthorders;
			unit->monthorders = NULL;
			return loc;
		}

		unit->Event("MOVE: Insufficient movement points; remaining moves queued.");

		// create a new order to enqueue
		AString order = (o->advancing) ? "ADVANCE " : "MOVE ";

		// convert direction to string, and append to 'order'
		if (i < NDIRS) order += DirectionAbrs[i];
		else if (i == MOVE_IN ) order += "IN";
		else if (i == MOVE_OUT) order += "OUT";
		else order += i - MOVE_ENTER;

		// do the same for remaining orders
		forlist(&o->dirs)
		{
			MoveDir *move = (MoveDir*)elem;

			order += " ";

			// convert direction to string, and append to 'order'
			if (move->dir < NDIRS) order += DirectionAbrs[move->dir];
			else if (move->dir == MOVE_IN) order += "IN";
			else if (move->dir == MOVE_OUT) order += "OUT";
			else order += move->dir - MOVE_ENTER;
		}

		// add the new order to unit
		TurnOrder *tOrder = new TurnOrder;
		tOrder->repeating = 0;
		tOrder->turnOrders.Add(new AString(order));

		unit->turnorders.Insert(tOrder);

		delete unit->monthorders;
		unit->monthorders = NULL;
		return loc;
	}
	//else, unit has sufficient movement points

	// check moving into ocean
	if (TerrainDefs[newreg->type].similar_type == R_OCEAN)
	{
		if (!unit->CanSwim() || unit->GetFlag(FLAG_NOCROSS_WATER))
		{
			unit->Event(AString("Discovers that ") + newreg->ShortPrint(&regions) + " is " + TerrainDefs[newreg->type].name + ".");
			delete unit->monthorders;
			unit->monthorders = NULL;
			return loc;
		}

		if (Globals->NO_SWIM_TO_SEA && !oceanIsCoastal(newreg) && !unit->CanFly())
		{
			unit->Event(AString("Cannot swim out to sea: ") + newreg->ShortPrint(&regions) + ".");
			delete unit->monthorders;
			unit->monthorders = NULL;
			return loc;
		}
	}

	if (unit->type == U_WMON && newreg->town && newreg->IsGuarded())
	{
		unit->Event("Monsters don't move into guarded towns.");
		delete unit->monthorders;
		unit->monthorders = NULL;
		return loc;
	}

	// check for guarded regions
	if (unit->guard == GUARD_ADVANCE)
	{
		// normally advance into guard will result in combat
		// but not against allies
		Unit *ally = newreg->ForbiddenByAlly(unit);
		if (ally && !startmove)
		{
			unit->Event(AString("Can't ADVANCE: ") + *(newreg->name) + " is guarded by " + *(ally->name) + ", an ally.");
			delete unit->monthorders;
			unit->monthorders = NULL;
			return loc;
		}
	}

	Unit *forbid = newreg->Forbidden(unit);
	if (forbid && !startmove && unit->guard != GUARD_ADVANCE)
	{
		unit->Event(AString("Is forbidden entry to ") + newreg->ShortPrint(&regions) + " by " + forbid->GetName(unit->GetObservation()) + ".");

		forbid->Event(AString("Forbids entry to ") + unit->GetName(forbid->GetObservation()) + ".");

		delete unit->monthorders;
		unit->monthorders = NULL;
		return loc;
	}

	// clear the unit's alias out, so as not to interfere with TEACH
	unit->alias = 0;

	unit->movepoints += cost;
	unit->MoveUnit(newreg->GetDummy());

	AString temp;
	switch (movetype)
	{
	case M_WALK:
		// swimming is walking on the ocean
		if (TerrainDefs[newreg->type].similar_type == R_OCEAN)
			temp = "Swims ";
		else
			temp = AString("Walks ") + road;
		break;

	case M_RIDE:
		temp = AString("Rides ") + road;
		break;

	case M_FLY:
		temp = "Flies ";
		break;
	}

	unit->Event(temp + AString("from ") + region->ShortPrint(&regions) + AString(" to ") + newreg->ShortPrint(&regions) + AString("."));

	if (forbid)
	{
		unit->advancefrom = region;
	}

	if (Globals->TRANSIT_REPORT != GameDefs::REPORT_NOTHING)
	{
		// update our visit record in the region we are leaving
		forlist(&region->passers)
		{
			Farsight *f = (Farsight*)elem;

			if (f->unit == unit)
			{
				// we moved into here this turn
				if (i < MOVE_IN)
				{
					f->exits_used[i] = 1;
				}
			}
		}

		// and mark the hex being entered
		Farsight *f = new Farsight;
		f->faction = unit->faction;
		f->level = 0;
		f->unit = unit;

		if (i < MOVE_IN)
		{
			f->exits_used[newreg->GetRealDirComp(i)] = 1;
		}
		newreg->passers.Add(f);
	}

	loc->region = newreg;
	return loc;
}


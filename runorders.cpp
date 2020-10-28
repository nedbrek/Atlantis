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
#include "game.h"
#include "faction.h"
#include "unit.h"
#include "gamedata.h"
#include "object.h"
#include "orders.h"
#include "gameio.h"
#include <algorithm>

//----------------------------------------------------------------------------
///@return true if the man type given by 'manIdx' is compatible with faction alignment 'a'
bool isAlignCompat(int manIdx, Faction::Alignments a)
{
	const ManType::Alignment ma = ManDefs[manIdx].align;
	if (ma == ManType::NEUTRAL)
		return true;

	return a == Faction::Alignments(ma);
}

//----------------------------------------------------------------------------
void Game::RunOrders()
{
	//
	// Form and instant orders are handled during parsing
	//
	Awrite("Running FIND Orders...");
	RunFindOrders();
	Awrite("Running ENTER/LEAVE Orders...");
	RunEnterOrders();
	Awrite("Running PROMOTE/EVICT Orders...");
	RunPromoteOrders();
	Awrite("Running Combat...");
	DoAttackOrders();
	DoAutoAttacks();
	Awrite("Running STEAL/ASSASSINATE Orders...");
	RunStealOrders();
	Awrite("Running GIVE/PAY/TRANSFER Orders...");
	DoGiveOrders();
	Awrite("Running EXCHANGE Orders...");
	DoExchangeOrders();
	Awrite("Running DESTROY Orders...");
	RunDestroyOrders();
	Awrite("Running PILLAGE Orders...");
	RunPillageOrders();
	Awrite("Running TAX Orders...");
	RunTaxOrders();
	Awrite("Running GUARD 1 Orders...");
	DoGuard1Orders();
	Awrite("Running Magic Orders...");
	ClearCastEffects();
	RunCastOrders();
	Awrite("Running SELL Orders...");
	RunSellOrders();
	Awrite("Running BUY Orders...");
	RunBuyOrders();
	Awrite("Running FORGET Orders...");
	RunForgetOrders();
	Awrite("Mid-Turn Processing...");
	MidProcessTurn();
	Awrite("Running QUIT Orders...");
	RunQuitOrders();
	if (Globals->ALLOW_WITHDRAW || Globals->WITHDRAW_LEADERS || Globals->WITHDRAW_FLEADERS)
	{
		Awrite("Running WITHDRAW Orders...");
		DoWithdrawOrders();
	}
	Awrite("Removing Empty Units...");
	DeleteEmptyUnits();
	SinkUncrewedShips();
	DrownUnits();
	Awrite("Running Sail Orders...");
	RunSailOrders();
	Awrite("Running Move Orders...");
	RunMoveOrders();
	SinkUncrewedShips();
	DrownUnits();
	FindDeadFactions();
	Awrite("Running Teach Orders...");
	RunTeachOrders();
	Awrite("Running Month-long Orders...");
	RunMonthOrders();
	RunTeleportOrders();
	Awrite("Assessing Maintenance costs...");
	AssessMaintenance();
	Awrite("Post-Turn Processing...");
	PostProcessTurn();
	DeleteEmptyUnits();
	RemoveEmptyObjects();
}

void Game::ClearCastEffects()
{
	forlist(&regions) {
		ARegion *r = (ARegion*)elem;
		r->applyToUnits([](Unit *u) { u->SetFlag(FLAG_INVIS, 0); });
	}
}

void Game::RunCastOrders()
{
	forlist(&regions) {
		ARegion * r = (ARegion *) elem;
		forlist(&r->objects) {
			Object * o = (Object *) elem;
			for(auto &u : o->getUnits())
			{
				if (u->castorders)
				{
					RunACastOrder(r,o,u);
					delete u->castorders;
					u->castorders = 0;
				}
			}
		}
  }
}

int Game::CountMages(Faction *pFac)
{
	int cnt = 0;
	forlist (&regions)
	{
		ARegion *r = (ARegion*)elem;
		forlist (&r->objects)
		{
			Object *o = (Object*)elem;
			for (auto &u : o->getUnits())
			{
				if (u->faction == pFac && u->type == U_MAGE)
					++cnt;
			}
		}
	}
	return cnt;
}

int Game::TaxCheck( ARegion *pReg, Faction *pFac )
{
	if( Globals->FACTION_LIMIT_TYPE == GameDefs::FACLIM_FACTION_TYPES )
	{
		if( AllowedTaxes( pFac ) == -1 )
		{
			//
			// No limit.
			//
			return( 1 );
		}

		forlist( &( pFac->war_regions )) {
			ARegion *x = ((ARegionPtr *) elem)->ptr;
			if( x == pReg )
			{
				//
				// This faction already performed a tax action in this
				// region.
				//
				return 1;
			}
		}
		if( pFac->war_regions.Num() >= AllowedTaxes( pFac ))
		{
			//
			// Can't tax here.
			//
			return 0;
		}
		else
		{
			//
			// Add this region to the faction's tax list.
			//
			ARegionPtr *y = new ARegionPtr;
			y->ptr = pReg;
			pFac->war_regions.Add(y);
			return 1;
		}
	}
	else
	{
		//
		// No limit on taxing regions in this game.
		//
		return( 1 );
	}
}

int Game::TradeCheck( ARegion *pReg, Faction *pFac )
{
	if( Globals->FACTION_LIMIT_TYPE == GameDefs::FACLIM_FACTION_TYPES )
	{
		if( AllowedTrades( pFac ) == -1 )
		{
			//
			// No limit on trading on this faction.
			//
			return( 1 );
		}

		forlist( &( pFac->trade_regions )) {
			ARegion * x = ((ARegionPtr *) elem)->ptr;
			if (x == pReg )
			{
				//
				// This faction has already performed a trade action in this
				// region.
				//
				return 1;
			}
		}
		if ( pFac->trade_regions.Num() >= AllowedTrades( pFac ))
		{
			//
			// This faction is over its trade limit.
			//
			return 0;
		}
		else
		{
			//
			// Add this region to the faction's trade list, and return 1.
			//
			ARegionPtr * y = new ARegionPtr;
			y->ptr = pReg;
			pFac->trade_regions.Add(y);
			return 1;
		}
	}
	else
	{
		//
		// No limit on trade in this game.
		//
		return( 1 );
	}
}

void Game::RunStealOrders()
{
	forlist(&regions) {
		ARegion * r = (ARegion *) elem;
		forlist(&r->objects) {
			Object * o = (Object *) elem;

			const auto units = o->getUnits(); // make copy
			for (auto &u : units)
			{
				if (u->stealorders)
				{
					if (u->stealorders->type == O_STEAL) {
						Do1Steal(r,o,u);
					} else if (u->stealorders->type == O_ASSASSINATE) {
						Do1Assassinate(r,o,u);
					}
					delete u->stealorders;
					u->stealorders = 0;
				}
			}
		}
	}
}

AList * Game::CanSeeSteal(ARegion * r,Unit * u)
{
	AList * retval = new AList;
	forlist(&factions) {
		Faction * f = (Faction *) elem;
		if (r->Present(f)) {
			if (f->CanSee(r,u, Globals->SKILL_PRACTISE_AMOUNT > 0)) {
				FactionPtr * p = new FactionPtr;
				p->ptr = f;
				retval->Add(p);
			}
		}
	}
	return retval;
}

void Game::Do1Assassinate(ARegion * r,Object * o,Unit * u)
{
	AssassinateOrder * so = (AssassinateOrder *) u->stealorders;
	Unit * tar = r->GetUnitId(so->target,u->faction->num);

	if (!tar) {
		u->Error("ASSASSINATE: Invalid unit given.");
		return;
	}
	if (!tar->IsAlive()) {
		u->Error("ASSASSINATE: Invalid unit given.");
		return;
	}

	// New rule -- You can only assassinate someone you can see
	if (!u->CanSee(r, tar)) {
		u->Error("ASSASSINATE: Invalid unit given.");
		return;
	}

	if (tar->type == U_GUARD || tar->type == U_WMON ||
			tar->type == U_GUARDMAGE) {
		u->Error("ASSASSINATE: Can only assassinate other player's "
				"units.");
		return;
	}

	if (u->GetMen() != 1) {
		u->Error("ASSASSINATE: Must be executed by a 1-man unit.");
		return;
	}

	AList * seers = CanSeeSteal(r,u);
	int succ = 1;
	forlist(seers) {
		Faction * f = ((FactionPtr *) elem)->ptr;
		if (f == tar->faction) {
			succ = 0;
			break;
		}
		if (f->GetAttitude(tar->faction->num) == A_ALLY) {
			succ = 0;
			break;
		}
		if (f->num == guardfaction) {
			succ = 0;
			break;
		}
	}
	if (!succ) {
		AString temp = *(u->name) + " is caught attempting to assassinate " +
			*(tar->name) + " in " + *(r->name) + ".";
		forlist(seers) {
			Faction * f = ((FactionPtr *) elem)->ptr;
			f->Event(temp);
		}
		// One learns from one's mistakes.  Surviving them is another matter!
		u->Practise(S_STEALTH);
		return;
	}

	int ass = 1;
	if(u->items.GetNum(I_RINGOFI)) {
		ass = 2; // Check if assassin has a ring.
		// New rule: if a target has an amulet of true seeing they
		// cannot be assassinated by someone with a ring of invisibility
		if(tar->AmtsPreventCrime(u)) {
			tar->Event( "Assassination prevented by amulet of true seeing." );
			u->Event( AString( "Attempts to assassinate " ) + *(tar->name) +
					", but is prevented by amulet of true seeing." );
			return;
		}
	}
	u->Practise(S_STEALTH);
	RunBattle(r,u,tar,ass);
}

void Game::Do1Steal(ARegion * r,Object * o,Unit * u)
{
	StealOrder * so = (StealOrder *) u->stealorders;
	Unit * tar = r->GetUnitId(so->target,u->faction->num);

	if (!tar) {
		u->Error("STEAL: Invalid unit given.");
		return;
	}

	// New RULE!! You can only steal from someone you can see.
	if(!u->CanSee(r, tar)) {
		u->Error("STEAL: Invalid unit given.");
		return;
	}

	if (tar->type == U_GUARD || tar->type == U_WMON ||
			tar->type == U_GUARDMAGE) {
		u->Error("STEAL: Can only steal from other player's "
				"units.");
		return;
	}

	if (u->GetMen() != 1) {
		u->Error("STEAL: Must be executed by a 1-man unit.");
		return;
	}

	AList * seers = CanSeeSteal(r,u);
	int succ = 1;
	{
		forlist(seers) {
			Faction * f = ((FactionPtr *) elem)->ptr;
			if (f == tar->faction) {
				succ = 0;
				break;
			}
			if (f->GetAttitude(tar->faction->num) == A_ALLY) {
				succ = 0;
				break;
			}
			if (f->num == guardfaction) {
				succ = 0;
				break;
			}
		}
	}

	if (!succ) {
		AString temp = *(u->name) + " is caught attempting to steal from " +
			*(tar->name) + " in " + *(r->name) + ".";
		forlist(seers) {
			Faction * f = ((FactionPtr *) elem)->ptr;
			f->Event(temp);
		}
		// One learns from one's mistakes.  Surviving them is another matter!
		u->Practise(S_STEALTH);
		return;
	}

	//
	// New rule; if a target has an amulet of true seeing they can't be
	// stolen from by someone with a ring of invisibility
	//
	if(tar->AmtsPreventCrime(u)) {
		tar->Event( "Theft prevented by amulet of true seeing." );
		u->Event( AString( "Attempts to steal from " ) + *(tar->name) + ", but "
				"is prevented by amulet of true seeing." );
		return;
	}

	int amt = 1;
	if (so->item == I_SILVER) {
		amt = tar->GetMoney();
		if (amt < 400) {
			amt = amt / 2;
		} else {
			amt = 200;
		}
	}

	if (tar->items.GetNum(so->item) < amt) {
		amt = 0;
	}

	u->items.SetNum(so->item,u->items.GetNum(so->item) + amt);
	tar->items.SetNum(so->item,tar->items.GetNum(so->item) - amt);

	{
		AString temp = *(u->name) + " steals " +
			ItemString(so->item,amt) + " from " + *(tar->name) + ".";
		forlist(seers) {
			Faction * f = ((FactionPtr *) elem)->ptr;
			f->Event(temp);
		}
	}

	tar->Event(AString("Has ") + ItemString(so->item,amt) + " stolen.");
	u->Practise(S_STEALTH);
	return;
}

void Game::DrownUnits()
{
	forlist(&regions)
	{
		ARegion *r = (ARegion*)elem;
		if (TerrainDefs[r->type].similar_type != R_OCEAN)
			continue;

		forlist(&r->objects)
		{
			Object *o = (Object*)elem;
			if (o->type != O_DUMMY)
				continue;

			for(auto &u : o->getUnits())
			{
				bool drown = false;
				if (u->type == U_WMON)
				{
					// don't drown monsters
				}
				else
				{
					switch (Globals->FLIGHT_OVER_WATER)
					{
					case GameDefs::WFLIGHT_UNLIMITED:
						drown = !(u->CanSwim());
						break;

					case GameDefs::WFLIGHT_MUST_LAND:
						drown = !(u->CanReallySwim() || u->leftShip);
						u->leftShip = 0;
						break;

					case GameDefs::WFLIGHT_NONE:
						drown = !(u->CanReallySwim());
						break;

					default: // Should never happen
						drown = !(u->CanReallySwim());
						break;
					}
				}

				if (drown)
				{
					r->Kill(u);
					u->Event("Drowns in the ocean.");
				}
			}
		}
	}
}

void Game::SinkUncrewedShips()
{
	forlist(&regions)
	{
		ARegion *r = (ARegion*)elem;

		forlist(&r->objects)
		{
			Object *o = (Object*)elem;
			bool check_empty = TerrainDefs[r->type].similar_type == R_OCEAN && o->IsBoat();
			// also remove empty armies
			if (o->type == O_ARMY)
				check_empty = true;

			if (!check_empty)
				continue;

			int men = 0;
			for (auto &u : o->getUnits())
			{
				men += u->GetMen();
			}

			if (men > 0)
				continue; // safe
			//else no men, move all units out to ocean

			{
				for (auto &u : o->getUnits())
				{
					u->MoveUnit(r->GetDummy());
				}
			}

			if (o->type == O_ARMY)
			{
				// sink all boats
				while (!o->objects.empty())
				{
					Object *sub_obj = o->objects.back();
					delete sub_obj;
					o->objects.pop_back();
				}
			}

			// And sink the boat
			r->objects.remove(o);
			delete o;
		}
	}
}

void Game::RunForgetOrders()
{
	forlist(&regions)
	{
		ARegion *r = (ARegion *) elem;
		forlist(&r->objects)
		{
			Object *o = (Object *) elem;
			for (auto &u : o->getUnits())
			{
				forlist(&u->forgetorders)
				{
					ForgetOrder *fo = (ForgetOrder *) elem;
					u->ForgetSkill(fo->skill);
					u->Event(AString("Forgets ") + SkillStrs(fo->skill) + ".");
				}
				u->forgetorders.DeleteAll();
			}
		}
	}
}

void Game::RunQuitOrders()
{
	forlist(&factions) {
		Faction * f = (Faction *) elem;
		if (f->quit)
			Do1Quit(f);
	}
}

void Game::Do1Quit(Faction *f)
{
	forlist(&regions)
	{
		ARegion *r = (ARegion*)elem;
		forlist(&r->objects)
		{
			Object *o = (Object*)elem;
			for (auto &u : o->getUnits())
			{
				if (u->faction == f)
				{
					o->removeUnit(u, true);
					delete u;
				}
			}
		}
	}
}

void Game::RunDestroyOrders()
{
	forlist(&regions)
	{
		ARegion *r = (ARegion*)elem;
		forlist(&r->objects)
		{
			Object *o = (Object*)elem;
			if (o->type == O_DUMMY || o->type == O_ARMY)
				continue;

			for (auto &u : o->getUnits())
			{
				if (u && u->destroy && o->GetOwner() && u->faction == o->GetOwner()->faction)
					Do1Destroy(r, o, u);
			}
		}
	}
}

void Game::Do1Destroy(ARegion *r, Object *o, Unit *u)
{
	if (TerrainDefs[r->type].similar_type == R_OCEAN)
	{
		u->Error("DESTROY: Can't destroy a ship while at sea.");
		for (auto &u : o->getUnits())
		{
			u->destroy = 0;
		}
		return;
	}

	if (!u->GetMen()) {
		u->Error("DESTROY: Empty units cannot destroy structures.");
		for (auto &u : o->getUnits())
		{
			u->destroy = 0;
		}
		return;
	}

	if (!o->CanModify())
	{
		u->Error("DESTROY: Can't destroy that.");
		for (auto &u : o->getUnits())
		{
			u->destroy = 0;
		}
		return;
	}

	u->Event(AString("Destroys ") + *(o->name) + ".");

	Object *dest = r->GetDummy();
	for (auto &u : o->getUnits())
	{
		u->destroy = 0;
		u->MoveUnit(dest);
	}
	r->objects.Remove(o);
	delete o;
}

void Game::RunFindOrders()
{
	forlist(&regions) {
		ARegion * r = (ARegion *) elem;
		forlist(&r->objects) {
			Object * o = (Object *) elem;
			for (auto &u : o->getUnits())
			{
				RunFindUnit(u);
			}
		}
	}
}

void Game::RunFindUnit(Unit *u)
{
	int all = 0;
	Faction *fac;
	forlist(&u->findorders) {
        if (Globals->DISABLE_FIND_EMAIL_COMMAND) {
            u->Error("FIND: This command has been disabled.");
            break;
        }

		FindOrder * f = (FindOrder *) elem;
		if(f->find == 0) all = 1;
		if(!all) {
			fac = GetFaction(&factions,f->find);
			if (fac) {
				u->faction->Event(AString("The address of ") + *(fac->name) +
						" is " + *(fac->address) + ".");
			} else {
				u->Error(AString("FIND: ") + f->find + " is not a valid "
						"faction number.");
			}
		} else {
			forlist(&factions) {
				fac = (Faction *)elem;
				if(fac) {
					u->faction->Event(AString("The address of ") +
							*(fac->name) + " is " + *(fac->address) + ".");
				}
			}
		}
	}
	u->findorders.DeleteAll();
}

void Game::RunTaxOrders()
{
	forlist(&regions) {
		RunTaxRegion((ARegion *) elem);
	}
}

int Game::CountTaxers(ARegion * reg)
{
	int t = 0;
	forlist(&reg->objects)
	{
		Object * o = (Object *) elem;
		for (auto &u : o->getUnits())
		{
			if (u->GetFlag(FLAG_AUTOTAX) && !Globals->TAX_PILLAGE_MONTH_LONG)
				u->taxing = TAX_TAX;

			if (u->taxing == TAX_AUTO)
				u->taxing = TAX_TAX;

			if (u->taxing != TAX_TAX)
				continue;

			if (!reg->CanTax(u))
			{
				u->Error("TAX: A unit is on guard.");
				u->taxing = TAX_NONE;
				continue;
			}

			int men = u->Taxers();

			if (!men)
			{
				u->Error("TAX: Unit cannot tax.");
				u->taxing = TAX_NONE;
				u->SetFlag(FLAG_AUTOTAX,0);
				continue;
			}

			if (!TaxCheck(reg, u->faction))
			{
				u->Error( "TAX: Faction can't tax that many regions.");
				u->taxing = TAX_NONE;
			}
			else
			{
				t += men;
			}
		}
	}
	return t;
}

void Game::RunTaxRegion(ARegion *reg)
{
	// use larger of money available and taxer income
	int desired = std::max(reg->money, CountTaxers(reg) * Globals->TAX_INCOME);

	forlist(&reg->objects)
	{
		Object *o = (Object*)elem;
		for (auto &u : o->getUnits())
		{
			if (u->taxing != TAX_TAX)
				continue;

			const double t = u->Taxers();
			const double fAmt = t * Globals->TAX_INCOME * reg->money / desired;

			const int amt = (int)fAmt;
			reg->money -= amt;
			desired -= t * Globals->TAX_INCOME;
			u->SetMoney(u->GetMoney() + amt);

			u->Event(AString("Collects $") + amt + " in taxes in " +
			      reg->ShortPrint(&regions) + ".");
			u->taxing = TAX_NONE;
		}
	}
}

void Game::RunPillageOrders()
{
	forlist (&regions) {
		RunPillageRegion((ARegion *) elem);
	}
}

int Game::CountPillagers(ARegion * reg)
{
	int p = 0;
	forlist(&reg->objects) {
		Object * o = (Object *) elem;
		for (auto &u : o->getUnits())
		{
			if (u->taxing == TAX_PILLAGE)
			{
				if (!reg->CanPillage(u)) {
					u->Error("PILLAGE: A unit is on guard.");
					u->taxing = TAX_NONE;
				} else {
					int men = u->Taxers();
					if (men) {
						if(!TaxCheck(reg, u->faction)) {
							u->Error("PILLAGE: Faction can't tax that many "
									"regions.");
							u->taxing = TAX_NONE;
						} else {
							p += men;
						}
					} else {
						u->Error("PILLAGE: Not a combat ready unit.");
						u->taxing = TAX_NONE;
					}
				}
			}
		}
	}
	return p;
}

void Game::ClearPillagers(ARegion * reg)
{
	forlist(&reg->objects) {
		Object * o = (Object *) elem;
		for (auto &u : o->getUnits())
		{
			if (u->taxing == TAX_PILLAGE) {
				u->Error("PILLAGE: Not enough men to pillage.");
				u->taxing = TAX_NONE;
			}
		}
	}
}

void Game::RunPillageRegion(ARegion * reg)
{
	if (TerrainDefs[reg->type].similar_type == R_OCEAN) return;
	if (reg->money < 1) return;
	if (reg->Wages() < 11) return;

	// First, count up pillagers
	int pillagers = CountPillagers(reg);

	if (pillagers * 2 * Globals->TAX_INCOME < reg->money) {
		ClearPillagers(reg);
		return;
	}

	AList *facs = reg->PresentFactions();
	int amt = reg->money * 2;
	forlist(&reg->objects)
	{
		Object *o = (Object*)elem;
		for (auto &u : o->getUnits())
		{
			if (u->taxing == TAX_PILLAGE)
			{
				u->taxing = TAX_NONE;
				int num = u->Taxers();
				int temp = (amt * num)/pillagers;
				amt -= temp;
				pillagers -= num;
				u->SetMoney(u->GetMoney() + temp);
				u->Event(AString("Pillages $") + temp + " from " +
						reg->ShortPrint( &regions ) + ".");
				forlist(facs) {
					Faction * fp = ((FactionPtr *) elem)->ptr;
					if (fp != u->faction) {
						fp->Event(*(u->name) + " pillages " +
								*(reg->name) + ".");
					}
				}
			}
		}
	}
	delete facs;

	// Destroy economy
	reg->money = 0;
	reg->wages -= 6;
	if (reg->wages < 6) reg->wages = 6;
}

void Game::RunPromoteOrders()
{
	// First, do any promote orders
	forlist(&regions)
	{
		ARegion *r = (ARegion*)elem;
		forlist(&r->objects)
		{
			Object *o = (Object*)elem;
			if (o->type == O_DUMMY)
				continue;

			for (auto &u : o->getUnits())
			{
				if (u && u->promote && o->GetOwner() && u->faction == o->GetOwner()->faction)
				{
					Do1PromoteOrder(o, u);
					delete u->promote;
					u->promote = 0;
				}
			}
		}
	}

	// Now do any evict orders
	{
		forlist(&regions)
		{
			ARegion *r = (ARegion*)elem;
			forlist(&r->objects)
			{
				Object *o = (Object*)elem;
				if (o->type == O_DUMMY)
					continue;

				for (auto &u : o->getUnits())
				{
					if (u && u->evictorders && o->GetOwner() && u->faction == o->GetOwner()->faction)
					{
						Do1EvictOrder(o, u);
						delete u->evictorders;
						u->evictorders = 0;
					}
				}
			}
		}
	}

	// Then, clear out other promote/evict orders
	{
		forlist(&regions)
		{
			ARegion *r = (ARegion*)elem;
			forlist(&r->objects)
			{
				Object *o = (Object*)elem;
				for (auto &u : o->getUnits())
				{
					if (u->promote)
					{
						if (o->type != O_DUMMY) {
							u->Error("PROMOTE: Must be owner");
							delete u->promote;
							u->promote = 0;
						}
						else
						{
							u->Error("PROMOTE: Can only promote inside structures.");
							delete u->promote;
							u->promote = 0;
						}
					}

					if (u->evictorders)
					{
						if (o->type != O_DUMMY)
						{
							u->Error("EVICT: Must be owner");
							delete u->evictorders;
							u->evictorders = 0;
						}
						else
						{
							u->Error("EVICT: Can only evict inside structures.");
							delete u->evictorders;
							u->evictorders = 0;
						}
					}
				}
			}
		}
	}
}

void Game::Do1PromoteOrder(Object *obj, Unit *u)
{
	Unit *tar = obj->GetUnitId(u->promote, u->faction->num);
	if (!tar) {
		u->Error("PROMOTE: Can't find target.");
		return;
	}
	obj->removeUnit(tar, true);
	obj->prependUnit(tar);
}

void Game::Do1EvictOrder(Object *obj, Unit *u)
{
	EvictOrder *ord = u->evictorders;
	Object *to = obj->region->GetDummy();

	while (ord && ord->targets.Num())
	{
		UnitId *id = (UnitId*)ord->targets.First();
		ord->targets.Remove(id);

		Unit *tar = obj->GetUnitId(id, u->faction->num);
		delete id;
		if (!tar)
			continue;

		if (obj->IsBoat() &&
		    TerrainDefs[obj->region->type].similar_type == R_OCEAN &&
		    (!tar->CanReallySwim() || tar->GetFlag(FLAG_NOCROSS_WATER)))
		{
			u->Error("EVICT: Cannot forcibly evict units over ocean.");
			continue;
		}

		tar->MoveUnit(to);
		tar->Event(AString("Evicted from ") + *obj->name + " by " + *u->name);
		u->Event(AString("Evicted ") + *tar->name + " from " + *obj->name);
	}
}

void Game::RunEnterOrders()
{
	// run enter first
	forlist(&regions) {
		ARegion *r = (ARegion*)elem;
		forlist(&r->objects)
		{
			Object *o = (Object*)elem;
			for (auto &u : o->getUnits())
			{
				if (u->enter_)
				{
					Do1EnterOrder(r, o, u);
				}
			}
		}
	}

	// then join
	{
	forlist(&regions)
	{
		ARegion *r = (ARegion*)elem;
		forlist(&r->objects)
		{
			Object *o = (Object*)elem;
			for (auto &u : o->getUnits())
			{
				if (u->joinorders)
				{
					Do1JoinOrder(r, o, u);
				}
			}
		}
	}
	}
}

void Game::Do1EnterOrder(ARegion * r,Object * in,Unit * u)
{
	Object *to = NULL;
	if (u->enter_ == -1)
	{
		to = r->GetDummy();
		u->enter_ = 0;

		if ((TerrainDefs[r->type].similar_type == R_OCEAN) &&
		    (!u->CanSwim() || u->GetFlag(FLAG_NOCROSS_WATER)))
		{
			u->Error("LEAVE: Can't leave a ship in the ocean.");
			return;
		}

		if (in->IsBoat() && u->CanSwim())
			u->leftShip = 1;

		if (u->object && u->object->type != O_DUMMY)
			u->Event(AString("Leaves ") + *u->object->name);
	}
	else
	{
		to = r->GetObject(u->enter_);
		u->enter_ = 0;
		if (!to || !to->CanEnter(r, u))
		{
			u->Error("ENTER: Can't enter that.");
			return;
		}

		if (to->ForbiddenBy(r, u))
		{
			u->Error("ENTER: Is refused entry.");
			return;
		}
	}

	u->MoveUnit( to );

	if (u->object && u->object->type != O_DUMMY)
		u->Event(AString("Enters ") + *u->object->name + ".");
}

void Game::Do1JoinOrder(ARegion *r, Object *in, Unit *u)
{
	JoinOrder &jo = *u->joinorders;

	// pull unit to join
	Unit *t = r->GetUnitId(jo.target, u->faction->num);
	if (!t)
	{
		u->Event(AString("JOIN: Nonexistant target (") + jo.target->Print(NULL) + ").");
		return;
	}
	// check relations
	if (t->faction->GetAttitude(u->faction->num) < A_FRIENDLY)
	{
		u->Event(AString("JOIN: Unfriendly faction for target (") + jo.target->Print(NULL) + ").");
		return;
	}

	Object *to_obj = t->object;
	Object *fm_obj = u->object;
	if (to_obj->type != O_DUMMY && to_obj == fm_obj)
	{
		// already together
		return;
	}

	const bool move_to_army = to_obj->type == O_ARMY || to_obj->parent != NULL;
	const bool move_from_army = fm_obj->type == O_ARMY || fm_obj->parent != NULL;

	//TODO? shift fm_obj to ship if captain?

	// check for implicit merge
	//--- not open space (DUMMY object)
	Unit *obj_owner = fm_obj->type != O_DUMMY ? fm_obj->GetOwner() : nullptr;
	// if we are the owner of our object, and the only unit
	if (u == obj_owner && fm_obj->getUnits().size() == 1)
	{
		jo.merge = true; // enforce merge
	}

	// only the onwer can merge
	if (u != obj_owner)
		jo.merge = false;
	//TODO: check captain in fleet

	// make sure we have one army
	if (!move_to_army && !move_from_army)
	{
		Object *army = new Object(r);
		army->type = O_ARMY;
		army->num = shipseq++;
		army->SetName(new AString("Army"));

		r->objects.push_back(army);

		// 'to' is not in an object
		if (to_obj->type == O_DUMMY)
		{
			// move the target unit in
			t->MoveUnit(army);
		}
		else // 'to' _is_ in an object
		{
			// move the object in
			army->SetName(new AString("Fleet"));
			army->SetPrevDir(to_obj->prevdir);
			army->objects.push_back(to_obj);
			to_obj->region->objects.remove(to_obj);

			to_obj->parent = army;

			// copy (not move) units into army
			for (auto &au : to_obj->getUnits())
			{
				au->object = army;
				army->addUnit(au);
			}

			t->object = army;
		}

		to_obj = army;
	}

	// if not merge
	if (!jo.merge || (fm_obj->type == O_DUMMY || fm_obj->incomplete > 0))
	{
		// just this unit
		fm_obj->removeUnit(u, true);
		to_obj->addUnit(u);
		u->object = to_obj;
		return;
	}
	//else merge
	// TODO: handle jo.overload
	//const int unit_wt = u->Weight();

	// if 'u' is in an army
	if (fm_obj->type == O_ARMY)
	{
		// move all ojbects
		for (Object *obj : fm_obj->objects)
		{
			to_obj->objects.push_back(obj);
			obj->parent = t->object;
		}
		fm_obj->objects.clear();

		// copy units into army
		for (auto &au : fm_obj->getUnits())
		{
			to_obj->addUnit(au);
			au->object = to_obj;
		}
		fm_obj->getUnits().clear();

		delete fm_obj;
		return;
	}
	//else
	// move the object (if it is complete)
	if (fm_obj->incomplete <= 0)
		to_obj->objects.push_back(fm_obj);

	// move units from object
	for (auto &au : fm_obj->getUnits())
	{
		to_obj->addUnit(au);
		au->object = to_obj;
	}

	if (fm_obj->parent)
	{
		// remove from other army
		auto &tmp = fm_obj->parent->objects;
		tmp.erase(std::remove(tmp.begin(), tmp.end(), fm_obj), tmp.end());

		fm_obj->parent = to_obj;
	}
	else if (fm_obj->incomplete <= 0)
	{
		// remove from region
		r->objects.remove(fm_obj);
		fm_obj->parent = to_obj;
	}
}

void Game::RemoveEmptyObjects()
{
	forlist(&regions)
	{
		ARegion *r = (ARegion*)elem;
		forlist(&r->objects)
		{
			Object *o = (Object*)elem;
			// if unit cost something
			// and no work was done
			if (ObjectDefs[o->type].cost &&
			    o->incomplete >= ObjectDefs[o->type].cost)
			{
				for (auto &u : o->getUnits())
				{
					u->MoveUnit(r->GetDummy());
				}
				r->objects.remove(o);
				delete o;
			}
		}
	}
}

void Game::EmptyHell()
{
	forlist(&regions)
		((ARegion*)elem)->ClearHell();
}

void Game::MidProcessUnit(ARegion *r, Unit *u)
{
	MidProcessUnitExtra(r, u);
}

void Game::PostProcessUnit(ARegion *r,Unit *u)
{
	PostProcessUnitExtra(r, u);
}

void Game::EndGame(Faction *pVictor)
{
	forlist( &factions ) {
		Faction *pFac = (Faction *) elem;
		pFac->exists = 0;
		if(pFac == pVictor)
			pFac->quit = QUIT_WON_GAME;
		else
			pFac->quit = QUIT_GAME_OVER;

		if(pVictor)
			pFac->Event( *( pVictor->name ) + " has won the game!" );
		else
			pFac->Event( "The game has ended with no winner." );
	}

	gameStatus = GAME_STATUS_FINISHED;
}

void Game::MidProcessTurn()
{
	forlist(&regions)
	{
		ARegion *r = (ARegion*)elem;
		// r->MidTurn(); // Not yet implemented
		forlist(&r->objects)
		{
			Object *o = (Object*)elem;
			for (auto &u : o->getUnits())
			{
				MidProcessUnit(r, u);
			}
		}
	}
}

void Game::PostProcessTurn()
{
	forlist(&regions)
	{
		ARegion *r = (ARegion*)elem;
		r->PostTurn(&regions);

		if (Globals->CITY_MONSTERS_EXIST && (r->town || r->type == R_NEXUS))
			AdjustCityMons(r);

		forlist (&r->objects)
		{
			Object *o = (Object*)elem;
			for (auto &u : o->getUnits())
			{
				PostProcessUnit(r, u);
			}
		}
	}

	if (Globals->WANDERING_MONSTERS_EXIST) GrowWMons(Globals->WMON_FREQUENCY);

	if (Globals->LAIR_MONSTERS_EXIST) GrowLMons(Globals->LAIR_FREQUENCY);

	if (Globals->LAIR_MONSTERS_EXIST) GrowVMons();

	// Check if there are any factions left
	bool livingFacs = false;
	{
		forlist(&factions)
		{
			Faction *pFac = (Faction*)elem;
			if (pFac->exists)
			{
				livingFacs = true;
				break;
			}
		}
	}

	if (!livingFacs)
		EndGame(0);
	else if(!(Globals->OPEN_ENDED))
	{
		Faction *pVictor = CheckVictory();
		if (pVictor)
			EndGame(pVictor);
	}
}

void Game::DoAutoAttacks()
{
	forlist(&regions) {
		ARegion *r = (ARegion*)elem;
		DoAutoAttacksRegion(r);
	}
}

void Game::DoAutoAttacksRegion(ARegion *r)
{
	forlist(&r->objects) {
		Object *o = (Object*)elem;
		for (auto &u : o->getUnits())
		{
			if (u->IsAlive() && u->canattack)
				DoAutoAttack(r, u);
		}
	}
}

void Game::DoAdvanceAttacks(AList *locs)
{
	forlist(locs)
	{
		Location *l = (Location*)elem;
		Unit *u = l->unit;
		ARegion *r = l->region;
		if (u->IsAlive() && u->canattack)
		{
			DoAutoAttack(r, u);
			if (!u->IsAlive() || !u->canattack)
			{
				u->guard = GUARD_NONE;
			}
		}

		if (u->IsAlive() && u->canattack && u->guard == GUARD_ADVANCE)
		{
			DoAdvanceAttack(r, u);
			u->guard = GUARD_NONE;
		}

		if (u->IsAlive())
		{
			DoAutoAttackOn(r, u);
			if (!u->IsAlive() || !u->canattack)
			{
				u->guard = GUARD_NONE;
			}
		}
	}
}

void Game::DoAutoAttackOn(ARegion *r, Unit *t)
{
	forlist(&r->objects)
	{
		Object *o = (Object*)elem;
		for (auto &u : o->getUnits())
		{
			if (u->guard != GUARD_AVOID &&
			    u->GetAttitude(r, t) == A_HOSTILE && u->IsAlive() &&
			    u->canattack)
			{
				AttemptAttack(r, u, t, 1);
			}

			if (!t->IsAlive())
				return;
		}
	}
}

void Game::DoAdvanceAttack(ARegion *r, Unit *u)
{
	Unit *t = r->Forbidden(u);
	while (t && u->IsAlive() && u->canattack)
	{
		AttemptAttack(r, u, t, 1, 1);
		t = r->Forbidden(u);
	}
}

void Game::DoAutoAttack(ARegion *r, Unit *u)
{
	forlist(&r->objects)
	{
		Object *o = (Object*)elem;
		for (auto &t : o->getUnits())
		{
			if (u->guard != GUARD_AVOID && u->GetAttitude(r, t) == A_HOSTILE)
			{
				AttemptAttack(r, u, t, 1);
			}
			if (u->IsAlive() == 0 || u->canattack == 0)
				return;
		}
	}
}

int Game::CountWMonTars(ARegion *r, Unit *mon)
{
	int retval = 0;
	forlist(&r->objects)
	{
		Object *o = (Object*)elem;
		for (auto &u : o->getUnits())
		{
			if (u->type == U_NORMAL || u->type == U_MAGE ||
			    u->type == U_APPRENTICE)
			{
				if (mon->CanSee(r, u) && mon->CanCatch(r, u))
				{
					retval += u->GetMen();
				}
			}
		}
	}
	return retval;
}

Unit* Game::GetWMonTar(ARegion *r, int tarnum, Unit *mon)
{
	forlist(&r->objects)
	{
		Object *o = (Object*)elem;
		for (auto &u : o->getUnits())
		{
			if (u->type == U_NORMAL || u->type == U_MAGE ||
			    u->type == U_APPRENTICE)
			{
				if (mon->CanSee(r, u) && mon->CanCatch(r, u))
				{
					const int num = u->GetMen();
					if (num && tarnum < num)
						return u;
					tarnum -= num;
				}
			}
		}
	}
	return nullptr;
}

void Game::CheckWMonAttack(ARegion *r, Unit *u)
{
	int tars = CountWMonTars(r, u);
	if (!tars) return;

	int rand = 300 - tars;
	if (rand < 100) rand = 100;
	if (getrandom(rand) >= u->Hostile()) return;

	Unit *t = GetWMonTar(r, getrandom(tars), u);
	if (t) AttemptAttack(r, u, t, 1);
}

void Game::DoAttackOrders()
{
	forlist(&regions)
	{
		ARegion *r = (ARegion*)elem;
		forlist(&r->objects)
		{
			Object *o = (Object*)elem;
			for (auto &u : o->getUnits())
			{
				if (u->type == U_WMON)
				{
					if (u->canattack && u->IsAlive())
					{
						CheckWMonAttack(r, u);
					}
					continue;
				}

				if (u->IsAlive() && u->attackorders)
				{
					AttackOrder *ord = u->attackorders;
					while (ord->targets.Num())
					{
						UnitId *id = (UnitId*)ord->targets.First();
						ord->targets.Remove(id);
						Unit *t = r->GetUnitId(id, u->faction->num);
						delete id;

						if (u->canattack && u->IsAlive())
						{
							if (t) {
								AttemptAttack(r, u, t, 0);
							} else {
								u->Error("ATTACK: Non-existent unit.");
							}
						}
					}
					delete ord;
					u->attackorders = nullptr;
				}
			}
		}
	}
}

/*
 * Presume that u is alive, can attack, and wants to attack t.
 * Check that t is alive, u can see t, and u has enough riding
 * skill to catch t.
 *
 * Return 0 if success.
 * 1 if t is already dead.
 * 2 if u can't see t
 * 3 if u lacks the riding to catch t
 */
void Game::AttemptAttack(ARegion *r, Unit *u, Unit *t, int silent, int adv)
{
	if (!t->IsAlive()) return;

	if (!u->CanSee(r, t))
	{
		if (!silent) u->Error("ATTACK: Non-existent unit.");
		return;
	}

	if (!u->CanCatch(r, t))
	{
		if (!silent) u->Error("ATTACK: Can't catch that unit.");
		return;
	}

	RunBattle(r, u, t, 0, adv);
	return;
}

void Game::RunSellOrders()
{
	forlist((&regions))
	{
		ARegion *r = (ARegion*)elem;
		forlist((&r->markets))
		{
			Market *m = (Market*)elem;
			if (m->type == M_SELL)
				DoSell(r, m);
		}

		{
			forlist(&r->objects)
			{
				Object *obj = (Object*)elem;
				for (auto &u : obj->getUnits())
				{
					forlist(&u->sellorders)
					{
						u->Error("SELL: Can't sell that.");
					}
					u->sellorders.deleteAll();
				}
			}
		}
	}
}

int Game::GetSellAmount(ARegion *r, Market *m)
{
	int num = 0;
	forlist(&r->objects)
	{
		Object *obj = (Object*)elem;
		for (auto &u : obj->getUnits())
		{
			forlist (&u->sellorders)
			{
				SellOrder *o = (SellOrder*)elem;
				if (o->item == m->item)
				{
					// sell all
					const int can_sell = u->items.CanSell(o->item);
					if (o->num == -1)
					{
						o->num = can_sell;
					}
					else if (o->num > can_sell)
					{
						o->num = can_sell;
						u->Error("SELL: Unit attempted to sell more than it had.");
					}

					if (o->num < 0)
						o->num = 0;
					else
						u->items.Selling(o->item, o->num);

					num += o->num;
				}
			}
		}
	}
	return num;
}

void Game::DoSell(ARegion *r, Market *m)
{
	// First, find the number of items being sold
	int attempted = std::max(m->amount, GetSellAmount(r, m));

	m->activity = 0;
	int oldamount = m->amount;
	forlist(&r->objects)
	{
		Object *obj = (Object*)elem;
		for (auto &u : obj->getUnits())
		{
			forlist(&u->sellorders)
			{
				SellOrder *o = (SellOrder*)elem;
				// only handle sales of m->item
				if (o->item != m->item)
					continue;

				int temp = 0;
				if (attempted)
				{
					temp = (m->amount * o->num + getrandom(attempted)) / attempted;
					if (temp < 0) temp = 0;
				}

				attempted -= o->num;
				m->amount -= temp;
				m->activity += temp;
				u->items.SetNum(o->item,u->items.GetNum(o->item) - temp);
				u->SetMoney(u->GetMoney() + temp * m->price);
				u->sellorders.Remove(o);
				u->Event(AString("Sells ") + ItemString(o->item, temp)
				   + " at $" + m->price + " each.");
				delete o;
			}
		}
	}
	m->amount = oldamount;
}

void Game::RunBuyOrders()
{
	//foreach region
	forlist(&regions)
	{
		ARegion *r = (ARegion*)elem;

		//foreach market
		forlist((&r->markets))
		{
			Market *m = (Market*)elem;

			// only process BUY orders
			if (m->type == M_BUY)
				DoBuy(r, m);
		}

		// process remainder as errors (BUY orders should be empty now)
		{
			//foreach object
			forlist(&r->objects)
			{
				Object *obj = (Object*)elem;

				//foreach unit
				for (auto &u : obj->getUnits())
				{
					//foreach BUY order
					forlist(&u->buyorders)
					{
						u->Error("BUY: Can't buy that.");
					}
					u->buyorders.deleteAll();
				}
			}
		}
	}
}

int Game::GetBuyAmount(ARegion *r, Market *m)
{
	int num = 0;
	forlist((&r->objects))
	{
		Object *obj = (Object*)elem;
		for (auto &u : obj->getUnits())
		{
			forlist (&u->buyorders)
			{
				BuyOrder *o = (BuyOrder*)elem;
				if (o->item == m->item)
				{
					if (ItemDefs[o->item].type & IT_MAN)
					{
						if (u->type == U_MAGE)
						{
							u->Error("BUY: Mages can't recruit more men.");
							o->num = 0;
						}

						if (u->type == U_APPRENTICE)
						{
							u->Error("BUY: Apprentices can't recruit more men.");
							o->num = 0;
						}

						if ((o->item == I_LEADERS && u->IsNormal()) ||
						    (o->item != I_LEADERS && u->IsLeader()))
						 {
							u->Error("BUY: Can't mix leaders and normal men.");
							o->num = 0;
						}

						// check max skills
						const int man_type = ItemDefs[o->item].index;
						const int first_man_type = u->firstManType();
						if (first_man_type != -1 && ManDefs[man_type].max_skills != ManDefs[first_man_type].max_skills)
						{
							u->Error("BUY: Can't mix men with different maximum number of skills.");
							o->num = 0;
						}

						if (!isAlignCompat(ItemDefs[o->item].index, u->faction->alignments_))
						{
							u->Error("BUY: Can only buy units matching your alignment.");
							o->num = 0;
						}
					}

					if (ItemDefs[o->item].type & IT_TRADE)
					{
						if( !TradeCheck( r, u->faction ))
						{
							u->Error( "BUY: Can't buy trade items in that many regions.");
							o->num = 0;
						}
					}

					// figure out how much money the unit can get
					const int max_money = (o->num == -1) ? -1 : o->num * m->price;
					const int unit_money = u->canConsume(I_SILVER, max_money);

					// if buy max
					if (o->num == -1)
					{
						o->num = unit_money / m->price;
					}

					if (o->num * m->price > unit_money)
					{
						o->num = unit_money / m->price;
						u->Error("BUY: Unit attempted to buy more than it could afford.");
					}
					num += o->num;
				}

				if (o->num < 1 && o->num != -1)
				{
					u->buyorders.Remove(o);
					delete o;
				}
			}
		}
	}

	return num;
}

void Game::DoBuy(ARegion *r, Market *m)
{
	// clear accumulator
	m->activity = 0;

	// First, find the number of items being purchased
	int attempted = GetBuyAmount(r,m);
	if (!attempted)
		return; // nothing to do

	// if not unlimited and not buying all
	if (m->amount != -1 && attempted < m->amount)
		attempted = m->amount; // pretend we're buying all (simplifies apportioning logic)

	// save amount available
	const int oldamount = m->amount;

	forlist(&r->objects)
	{
		Object *obj = (Object*)elem;
		for (auto &u : obj->getUnits())
		{
			forlist(&u->buyorders)
			{
				BuyOrder *o = (BuyOrder*)elem;
				if (o->item != m->item)
					continue;

				const int max_money = (o->num == -1) ? -1 : o->num * m->price;
				const int unit_money = u->canConsume(I_SILVER, max_money);

				int temp = o->num;
				if (temp * m->price > unit_money)
				{
					temp = unit_money / m->price;
				}

				// if not unlimited market
				if (m->amount != -1)
				{
					if (attempted)
					{
						temp = (m->amount * temp +
						      getrandom(attempted)) / attempted;

						if (temp < 0)
							temp = 0;
					}
					attempted -= o->num;
					m->amount -= temp;
					m->activity += temp;
				}

				if (ItemDefs[o->item].type & IT_MAN)
				{
					// recruiting; must dilute skills
					u->AdjustSkills();
				}
				u->items.SetNum(o->item,u->items.GetNum(o->item) + temp);

				u->consume(I_SILVER, temp * m->price);

				u->Event(AString("Buys ") + ItemString(o->item,temp)
				      + " at $" + m->price + " each.");

				// send a message about the item, if it is unknown
				u->faction->DiscoverItem(o->item, 0, 1);

				u->buyorders.Remove(o);
				delete o;
			}
		}
	}

	m->amount = oldamount;
}

void Game::CheckUnitMaintenanceItem(int item, int value, int consume)
{
	forlist(&regions)
	{
		ARegion *r = (ARegion*)elem;
		forlist(&r->objects)
		{
			Object *obj = (Object*)elem;
			for (auto &u : obj->getUnits())
			{
				if (u->needed > 0 &&
				    (!consume ||
				     u->GetFlag(FLAG_CONSUMING_UNIT) ||
				     u->GetFlag(FLAG_CONSUMING_FACTION)
				    )
				)
				{
					int amount = u->items.GetNum(item);
					if (!amount)
						continue;

					int eat = (u->needed + value - 1) / value;
					if (eat > amount)
						eat = amount;

					if (ItemDefs[item].type & IT_FOOD)
					{
						if (Globals->UPKEEP_MAXIMUM_FOOD >= 0 &&
						    eat * value > u->stomach_space)
						{
							eat = (u->stomach_space + value - 1) / value;
							if (eat < 0)
								eat = 0;
						}
						u->hunger -= eat * value;
						u->stomach_space -= eat * value;
						if (Globals->UPKEEP_MAXIMUM_FOOD >= 0 &&
						    u->stomach_space < 0)
						{
							u->needed -= u->stomach_space;
							u->stomach_space = 0;
						}
					}
					u->needed -= eat * value;
					u->items.SetNum(item, amount - eat);
				}
			}
		}
	}
}

void Game::CheckFactionMaintenanceItem(int item, int value, int consume)
{
	forlist(&regions)
	{
		ARegion *r = (ARegion*)elem;
		forlist(&r->objects)
		{
			Object *obj = (Object*)elem;
			for (auto &u : obj->getUnits())
			{
				if (u->needed > 0 &&
				    (!consume ||
				     u->GetFlag(FLAG_CONSUMING_FACTION))
				)
				{
					// Go through all units again
					forlist(&r->objects)
					{
						Object *obj2 = (Object*)elem;
						for (auto &u2 : obj2->getUnits())
						{
							if (u->faction == u2->faction && u != u2)
							{
								int amount = u2->items.GetNum(item);
								if (amount)
								{
									int eat = (u->needed + value - 1) / value;
									if (eat > amount)
										eat = amount;

									if (ItemDefs[item].type & IT_FOOD)
									{
										if (Globals->UPKEEP_MAXIMUM_FOOD >= 0 &&
										    eat * value > u->stomach_space)
										{
											eat = (u->stomach_space + value - 1) / value;
											if (eat < 0)
												eat = 0;
										}
										u->hunger -= eat * value;
										u->stomach_space -= eat * value;
										if (Globals->UPKEEP_MAXIMUM_FOOD >= 0 &&
										    u->stomach_space < 0)
										{
											u->needed -= u->stomach_space;
											u->stomach_space = 0;
										}
									}
									u->needed -= eat * value;
									u2->items.SetNum(item, amount - eat);
								}
							}
						}

						if (u->needed < 1) break;
					}
				}
			}
		}
	}
}

void Game::CheckAllyMaintenanceItem(int item, int value)
{
	forlist(&regions)
	{
		ARegion *r = (ARegion*)elem;
		forlist(&r->objects)
		{
			Object *obj = (Object*)elem;
			for (auto &u : obj->getUnits())
			{
				if (u->needed <= 0)
					continue;

				// Go through all units again
				forlist(&r->objects)
				{
					Object *obj2 = (Object*)elem;
					{
						for (auto &u2 : obj2->getUnits())
						{
							if (u->faction != u2->faction &&
							    u2->GetAttitude(r,u) == A_ALLY)
							{
								int amount = u2->items.GetNum(item);
								if (amount)
								{
									int eat = (u->needed + value - 1) / value;
									if (eat > amount)
										eat = amount;
									if (ItemDefs[item].type & IT_FOOD)
									{
										if (Globals->UPKEEP_MAXIMUM_FOOD >= 0 &&
										    eat * value > u->stomach_space)
										{
											eat = (u->stomach_space + value - 1) / value;
											if (eat < 0)
												eat = 0;
										}
										u->hunger -= eat * value;
										u->stomach_space -= eat * value;
										if (Globals->UPKEEP_MAXIMUM_FOOD >= 0 &&
										    u->stomach_space < 0)
										{
											u->needed -= u->stomach_space;
											u->stomach_space = 0;
										}
									}
									if (eat)
									{
										u->needed -= eat * value;
										u2->items.SetNum(item, amount - eat);
										u2->Event(*(u->name) + " borrows " +
										          ItemString(item, eat) +
										          " for maintenance.");
										u->Event(AString("Borrows ") +
										         ItemString(item, eat) +
										         " from " + *(u2->name) +
										         " for maintenance.");
										u2->items.SetNum(item, amount - eat);
									}
								}
							}
						}

						if (u->needed < 1) break;
					}
				}
			}
		}
	}
}

void Game::CheckUnitHungerItem(int item, int value)
{
	forlist(&regions)
	{
		ARegion *r = (ARegion*)elem;
		forlist(&r->objects)
		{
			Object *obj = (Object*)elem;
			for (auto &u : obj->getUnits())
			{
				if (u->hunger <= 0)
					continue;

				int amount = u->items.GetNum(item);
				if (!amount)
					continue;

				int eat = (u->hunger + value - 1) / value;
				if (eat > amount)
					eat = amount;

				u->hunger -= eat * value;
				u->stomach_space -= eat * value;
				u->needed -= eat * value;

				if (Globals->UPKEEP_MAXIMUM_FOOD >= 0 &&
				    u->stomach_space < 0)
				{
					u->needed -= u->stomach_space;
					u->stomach_space = 0;
				}

				u->items.SetNum(item, amount - eat);
			}
		}
	}
}

void Game::CheckFactionHungerItem(int item, int value)
{
	forlist(&regions)
	{
		ARegion *r = (ARegion*)elem;
		forlist(&r->objects)
		{
			Object *obj = (Object*)elem;
			for (auto &u : obj->getUnits())
			{
				if (u->hunger <= 0)
					continue;

				// Go through all units again
				forlist(&r->objects)
				{
					Object *obj2 = (Object*)elem;
					for (auto &u2 : obj2->getUnits())
					{
						if (u->faction == u2->faction && u != u2)
						{
							const int amount = u2->items.GetNum(item);
							if (amount)
							{
								int eat = (u->hunger + value - 1) / value;
								if (eat > amount)
									eat = amount;

								u->hunger -= eat * value;
								u->stomach_space -= eat * value;
								u->needed -= eat * value;

								if (Globals->UPKEEP_MAXIMUM_FOOD >= 0 &&
								    u->stomach_space < 0)
								{
									u->needed -= u->stomach_space;
									u->stomach_space = 0;
								}

								u2->items.SetNum(item, amount - eat);
							}
						}
					}

					if (u->hunger < 1) break;
				}
			}
		}
	}
}

void Game::CheckAllyHungerItem(int item, int value)
{
	forlist(&regions)
	{
		ARegion * r = (ARegion *) elem;
		forlist((&r->objects))
		{
			Object * obj = (Object *) elem;
			for (auto &u : obj->getUnits())
			{
				if (u->hunger > 0)
				{
					// Go through all units again
					forlist((&r->objects)) {
						Object * obj2 = (Object *) elem;
						for (auto &u2 : obj2->getUnits())
						{
							if (u->faction != u2->faction &&
							   u2->GetAttitude(r,u) == A_ALLY)
							{
								int amount = u2->items.GetNum(item);
								if (amount) {
									int eat = (u->hunger + value - 1) / value;
									if (eat > amount)
										eat = amount;
									u->hunger -= eat * value;
									u->stomach_space -= eat * value;
									u->needed -= eat * value;
									if (Globals->UPKEEP_MAXIMUM_FOOD >= 0 &&
										u->stomach_space < 0)
									{
										u->needed -= u->stomach_space;
										u->stomach_space = 0;
									}
									u2->items.SetNum(item, amount - eat);
									    u2->Event(*(u->name) + " borrows " +
									              ItemString(item, eat) +
									              " to fend off starvation.");
									    u->Event(AString("Borrows ") +
									             ItemString(item, eat) +
									             " from " + *(u2->name) +
									             " to fend off starvation.");
									    u2->items.SetNum(item, amount - eat);
								}
							}
						}

						if (u->hunger < 1) break;
					}
				}
			}
		}
	}
}

void Game::AssessMaintenance()
{
	// First pass: set needed
	{
		forlist(&regions)
		{
			ARegion *r = (ARegion*)elem;
			forlist(&r->objects)
			{
				Object *obj = (Object*)elem;
				for (auto &u : obj->getUnits())
				{
					u->needed = u->MaintCost();
					u->hunger = u->GetMen() * Globals->UPKEEP_MINIMUM_FOOD;
					if (Globals->UPKEEP_MAXIMUM_FOOD < 0)
						u->stomach_space = -1;
					else
						u->stomach_space = u->GetMen() *
						               Globals->UPKEEP_MAXIMUM_FOOD;
				}
			}
		}
	}

	// Assess food requirements first
	if (Globals->UPKEEP_MINIMUM_FOOD > 0)
	{
		CheckUnitHunger();
		CheckFactionHunger();
		if (Globals->ALLOW_WITHDRAW)
		{
			// Can claim food for maintenance, so find the cheapest food
			int i = -1, cost = -1;
			for (unsigned j = 0; j < ItemDefs.size(); ++j)
			{
				if (ItemDefs[j].flags & ItemType::DISABLED) continue;
				if (ItemDefs[j].type & IT_FOOD) {
					if (i == -1 ||
							ItemDefs[i].baseprice > ItemDefs[j].baseprice)
						i = j;
				}
			}

			if (i > 0)
			{
				cost = ItemDefs[i].baseprice * 5 / 2;
				forlist(&regions)
				{
					ARegion *r = (ARegion*)elem;
					forlist(&r->objects)
					{
						Object *obj = (Object*)elem;
						for (auto &u : obj->getUnits())
						{
							if (u->hunger > 0 && u->faction->unclaimed > cost)
							{
								int value = Globals->UPKEEP_FOOD_VALUE;
								int eat = (u->hunger + value - 1) / value;
								// Now see if faction has money
								if (u->faction->unclaimed >= eat * cost)
								{
									u->Event(AString("Withdraws ") +
											ItemString(i, eat) +
											" for maintenance.");
									u->faction->unclaimed -= eat * cost;
									u->hunger -= eat * value;
									u->stomach_space -= eat * value;
									u->needed -= eat * value;
								}
								else
								{
									int amount = u->faction->unclaimed / cost;
									u->Event(AString("Withdraws ") +
											ItemString(i, amount) +
											" for maintenance.");
									u->faction->unclaimed -= amount * cost;
									u->hunger -= amount * value;
									u->stomach_space -= amount * value;
									u->needed -= amount * value;
								}
							}
						}
					}
				}
			}
		}
		CheckAllyHunger();
	}

	// Check for CONSUMEing units.
	if (Globals->FOOD_ITEMS_EXIST)
	{
		CheckUnitMaintenance(1);
		CheckFactionMaintenance(1);
	}

	// Check the unit for money.
	CheckUnitMaintenanceItem(I_SILVER, 1, 0);

	// Check other units in same faction for money
	CheckFactionMaintenanceItem(I_SILVER, 1, 0);

	if (Globals->FOOD_ITEMS_EXIST)
	{
		// Check unit for possible food items.
		CheckUnitMaintenance(0);

		// Fourth pass; check other units in same faction for food items
		CheckFactionMaintenance(0);
	}

	// Check unclaimed money
	{
		forlist(&regions)
		{
			ARegion *r = (ARegion*)elem;
			forlist((&r->objects))
			{
				Object *obj = (Object*)elem;
				for (auto &u : obj->getUnits())
				{
					if (u->needed > 0 && u->faction->unclaimed)
					{
						// Now see if faction has money
						if (u->faction->unclaimed >= u->needed)
						{
							u->Event(AString("Claims ") + u->needed +
									 " silver for maintenance.");
							u->faction->unclaimed -= u->needed;
							u->needed = 0;
						}
						else
						{
							u->Event(AString("Claims ") +
									u->faction->unclaimed +
									" silver for maintenance.");
							u->needed -= u->faction->unclaimed;
							u->faction->unclaimed = 0;
						}
					}
				}
			}
		}
	}

	// Check other allied factions for $$$.
	CheckAllyMaintenanceItem(I_SILVER, 1);

	if (Globals->FOOD_ITEMS_EXIST)
	{
		// Check other factions for food items.
		CheckAllyMaintenance();
	}

	// Last, if the unit still needs money, starve some men.
	{
		forlist(&regions)
		{
			ARegion *r = (ARegion*)elem;
			forlist(&r->objects)
			{
				Object *obj = (Object*)elem;
				for (auto &u : obj->getUnits())
				{
					if (u->needed > 0 || u->hunger > 0)
						u->Short(u->needed, u->hunger);
				}
			}
		}
	}
}

void Game::DoWithdrawOrders()
{
	forlist(&regions)
	{
		ARegion *r = (ARegion*)elem;
		forlist(&r->objects)
		{
			Object *obj = (Object*)elem;
			for (auto &u : obj->getUnits())
			{
				forlist((&u->withdraworders))
				{
					WithdrawOrder *o = (WithdrawOrder*)elem;
					if (DoWithdrawOrder(r, u, o))
						break;
				}
				u->withdraworders.deleteAll();
			}
		}
	}
}

int Game::DoWithdrawOrder(ARegion *r, Unit *u, WithdrawOrder *o)
{
	if (r->type == R_NEXUS)
	{
		u->Error("WITHDRAW: Withdraw does not work in the Nexus.");
		return 1;
	}

	const int itm = o->item;
	const int amt = o->amount;

	if (itm == I_LEADERS)
	{
		const int num_leaders = countItemFaction(I_LEADERS, u->faction->num);
		if (num_leaders + amt > Globals->WITHDRAW_LEADERS)
		{
			u->Error(AString("WITHDRAW LEADERS: ") + amt + " more leaders would put you over the maximum.");
			return 0;
		}

		if (u->GetMen() > 0)
		{
			u->Error("WITHDRAW LEADERS: Unit has men already.");
			return 0;
		}

		if (amt == 1)
			u->Event("A new leader arises!");
		else
			u->Event(AString(amt) + " new leaders arise!");

		u->items.SetNum(I_LEADERS, amt);
		return 0;
	}

	if (itm == I_FACTIONLEADER)
	{
		const int num_leaders = countItemFaction(I_FACTIONLEADER, u->faction->num);
		if (num_leaders + amt > Globals->WITHDRAW_FLEADERS)
		{
			u->Error(AString("WITHDRAW FLEAD: ") + amt + " more faction leaders would put you over the maximum.");
			return 0;
		}

		if (u->GetMen() > 0)
		{
			u->Error("WITHDRAW FLEAD: Unit has men already.");
			return 0;
		}

		if (amt == 1)
			u->Event("A new leader for your faction arises!");
		else
			u->Event(AString(amt) + " new faction leaders arise!");

		u->items.SetNum(I_FACTIONLEADER, amt);
		return 0;
	}

	const int cost = (ItemDefs[itm].baseprice *5/2)*amt;
	if (cost > u->faction->unclaimed)
	{
		u->Error(AString("WITHDRAW: Too little unclaimed silver to withdraw ") +
		      ItemString(itm,amt)+".");
		return 0;
	}
	u->faction->unclaimed -= cost;
	u->Event(AString("Withdraws ") + ItemString(o->item,amt) + ".");
	u->items.SetNum(itm, u->items.GetNum(itm) + amt);
	return 0;
}

void Game::DoGiveOrders()
{
	//foreach region
	forlist(&regions)
	{
		ARegion *r = (ARegion*)elem;

		//foreach object in region
		forlist((&r->objects))
		{
			Object *obj = (Object*)elem;

			//foreach unit in object
			for (auto &u : obj->getUnits())
			{
				//foreach give order
				forlist(&u->giveorders)
				{
					GiveOrder *o = (GiveOrder*)elem;

					// if normal give
					if (o->item >= 0)
					{
						if (DoGiveOrder(r, u, o))
							break; // failed

						continue;
					}

					// should be GIVE ALL CLASS
					if (o->amount != -2)
					{
						u->Error("GIVE: Invalid GIVE ALL CLASS command.");
						continue;
					}

					// class is in negative space
					const int mask = -o->item;

					forlist(&u->items)
					{
						Item *item = (Item*)elem;
						if (mask == (int)ItemDefs.size() || // GIVE ALL ITEMS
						    (ItemDefs[item->type].type & mask)) // GIVE ALL CLASS and class match
						{
							// create new give order
							GiveOrder go;
							go.item = item->type; // current item
							go.amount = item->num; // all
							go.except = 0; // no except
							go.limit = o->limit; // same limit
							go.target = o->target; // same target

							DoGiveOrder(r, u, &go);
							go.target = nullptr; // don't let destructor delete target
						}
					}
				}
				u->giveorders.deleteAll();
			}
		}
	}
}

void Game::DoExchangeOrders()
{
	forlist(&regions)
	{
		ARegion *r = (ARegion*)elem;
		forlist(&r->objects)
		{
			Object *obj = (Object*)elem;
			for (auto &u : obj->getUnits())
			{
				forlist(&u->exchangeorders)
				{
					Order *o = (Order*)elem;
					DoExchangeOrder(r, u, (ExchangeOrder*)o);
				}
			}
		}
	}
}

void Game::DoExchangeOrder(ARegion *r, Unit *u, ExchangeOrder *o)
{
	// Check if the destination unit exists
	Unit *t = r->GetUnitId(o->target, u->faction->num);
	if (!t)
	{
		u->Error(AString("EXCHANGE: Nonexistant target (") +
		      o->target->Print(NULL) + ").");
		u->exchangeorders.Remove(o);
		return;
	}

	// check for give across factions
	if (Globals->ALIGN_RESTRICT_RELATIONS && u->faction != t->faction &&
	    u->faction->alignments_ != Faction::ALL_NEUTRAL &&
	    t->faction->alignments_ != Faction::ALL_NEUTRAL)
	{
		if (u->faction->alignments_ != t->faction->alignments_)
		{
			u->Error("EXCHANGE: Cannot give across alignments.");
			return;
		}
	}

	// Check each Item can be given
	if (ItemDefs[o->giveItem].flags & ItemType::CANTGIVE)
	{
		u->Error(AString("EXCHANGE: Can't trade ") +
		      ItemDefs[o->giveItem].names + ".");
		u->exchangeorders.remove(o);
		return;
	}

	if (ItemDefs[o->expectItem].flags & ItemType::CANTGIVE)
	{
		u->Error(AString("EXCHANGE: Can't trade ") +
		      ItemDefs[o->expectItem].names + ".");
		u->exchangeorders.remove(o);
		return;
	}

	if (ItemDefs[o->giveItem].type & IT_MAN)
	{
		u->Error("EXCHANGE: Exchange aborted.  Men may not be traded.");
		u->exchangeorders.remove(o);
		return;
	}

	if (ItemDefs[o->expectItem].type & IT_MAN)
	{
		u->Error("EXCHANGE: Exchange aborted. Men may not be traded.");
		u->exchangeorders.remove(o);
		return;
	}

	// New RULE -- Must be able to see unit to give something to them!
	if (!u->CanSee(r, t))
	{
		u->Error(AString("EXCHANGE: Nonexistant target (") +
		      o->target->Print(NULL) + ").");
		return;
	}

	// Check other unit has enough to give
	int amtRecieve = o->expectAmount;
	if (amtRecieve > t->items.GetNum(o->expectItem))
	{
		t->Error(AString("EXCHANGE: Not giving enough. Expecting ") +
		      ItemString(o->expectItem, o->expectAmount) + ".");
		u->Error(AString("EXCHANGE: Exchange aborted.  Not enough ") +
		      "recieved. Expecting " +
		   ItemString(o->expectItem,o->expectAmount) + ".");
		o->exchangeStatus = 0;
		return;
	}

	int exchangeOrderFound = 0;
	// Check if other unit has a reciprocal exchange order
	forlist (&t->exchangeorders)
	{
		ExchangeOrder *tOrder = (ExchangeOrder*)elem;
		Unit *ptrUnitTemp = r->GetUnitId(tOrder->target, t->faction->num);
		if (ptrUnitTemp == u)
		{
			if (tOrder->expectItem == o->giveItem)
			{
				if (tOrder->giveItem == o->expectItem)
				{
					exchangeOrderFound = 1;
					if (tOrder->giveAmount < o->expectAmount)
					{
						t->Error(AString("EXCHANGE: Not giving enough. ") +
								"Expecting " +
								ItemString(o->expectItem,o->expectAmount) +
								".");
						u->Error(AString("EXCHANGE: Exchange aborted. ") +
								"Not enough recieved. Expecting " +
								ItemString(o->expectItem,o->expectAmount) +
								".");
						tOrder->exchangeStatus = 0;
						o->exchangeStatus = 0;
						return;
					}
					else if (tOrder->giveAmount > o->expectAmount)
					{
						t->Error(AString("EXCHANGE: Exchange aborted. Too ") +
								"much given. Expecting " +
								ItemString(o->expectItem,o->expectAmount) +
								".");
						u->Error(AString("EXCHANGE: Exchange aborted. Too ") +
								"much offered. Expecting " +
								ItemString(o->expectItem,o->expectAmount) +
								".");
						tOrder->exchangeStatus = 0;
						o->exchangeStatus = 0;
					}
					else if (tOrder->giveAmount == o->expectAmount)
						o->exchangeStatus = 1;

					if (o->exchangeStatus == 1 && tOrder->exchangeStatus == 1)
					{
						u->Event(AString("Exchanges ") +
								ItemString(o->giveItem, o->giveAmount) +
								" with " + *t->name + " for " +
								ItemString(tOrder->giveItem,
									tOrder->giveAmount) +
								".");
						t->Event(AString("Exchanges ") +
								ItemString(tOrder->giveItem,
									tOrder->giveAmount) + " with " +
								*u->name + " for " +
								ItemString(o->giveItem,o->giveAmount) + ".");
						u->items.SetNum(o->giveItem,
								u->items.GetNum(o->giveItem) - o->giveAmount);
						t->items.SetNum(o->giveItem,
								t->items.GetNum(o->giveItem) + o->giveAmount);
						t->items.SetNum(tOrder->giveItem,
								t->items.GetNum(tOrder->giveItem) -
								tOrder->giveAmount);
						u->items.SetNum(tOrder->giveItem,
								u->items.GetNum(tOrder->giveItem) +
								tOrder->giveAmount);
						u->faction->DiscoverItem(tOrder->giveItem, 0, 1);
						t->faction->DiscoverItem(o->giveItem, 0, 1);

						u->exchangeorders.remove(o);
						t->exchangeorders.remove(tOrder);
						return;
					}
					else if (o->exchangeStatus >= 0 && tOrder->exchangeStatus >= 0)
					{
						u->exchangeorders.remove(o);
						t->exchangeorders.remove(tOrder);
					}
				}
			}
		}
	}

	if (!exchangeOrderFound)
	{
		if (!u->CanSee(r, t))
		{
			u->Error(AString("EXCHANGE: Nonexistant target (") +
			      o->target->Print(NULL) + ").");
			u->exchangeorders.remove(o);
			return;
		}
		//else
		u->Error("EXCHANGE: target unit did not issue a matching exchange order.");
		u->exchangeorders.remove(o);
	}
}

int Game::DoGiveOrder(ARegion *r, Unit *u, GiveOrder *o)
{
	if (o->item < 0)
		return 0;

	// Check there is enough to give
	int amt = o->amount;
	if (amt != -2 && amt > u->items.GetNum(o->item))
	{
		u->Error("GIVE: Not enough.");

		// reduce to all
		amt = u->items.GetNum(o->item);
	}
	else if (amt == -2) // all
	{
		amt = u->items.GetNum(o->item);

		if (o->except)
		{
			if (o->except > amt)
			{
				amt = 0;
				u->Error("GIVE: EXCEPT value greater than amount on hand.");
			}
			else
			{
				amt -= o->except;
			}
		}
	}

	// if GIVE 0 (drop/release)
	if (!o->target->valid())
	{
		// TODO? give 0 ship?
		if (amt == -1)
		{
			u->Error("Can't discard a whole unit.");
			return 0;
		}
		ItemType &item = ItemDefs[o->item];

		AString temp = "Discards ";
		if (item.type & IT_MAN)
		{
			u->SetMen(o->item, u->GetMen(o->item) - amt);
			temp = "Disbands ";
		}
		else if (Globals->RELEASE_MONSTERS && (item.type & IT_MONSTER))
		{
			// release monsters
			temp = "Releases ";
			u->items.SetNum(o->item, u->items.GetNum(o->item) - amt);

			if (Globals->WANDERING_MONSTERS_EXIST)
			{
				Faction *mfac = GetFaction(&factions, monfaction);
				Unit *mon = GetNewUnit(mfac, 0);
				const int mondef = item.index;
				mon->MakeWMon(MonDefs[mondef].name, o->item, amt);
				mon->MoveUnit(r->GetDummy());

				// time before spoils recover
				mon->free = Globals->MONSTER_NO_SPOILS + Globals->MONSTER_SPOILS_RECOVERY;
			}
		}
		else
		{
			u->items.SetNum(o->item, u->items.GetNum(o->item) - amt);
		}

		u->Event(temp + ItemString(o->item, amt) + ".");
		return 0;
	}

	// pull target
	Unit *t = r->GetUnitId(o->target, u->faction->num);
	if (!t)
	{
		u->Event(AString("GIVE: Nonexistant target (") + o->target->Print(nullptr) + ").");
		return 0;
	}

	if (u == t)
	{
		const int amt = o->amount < 0 ? 2 : o->amount; // negative means "all", so plural
		u->Error(AString("GIVE: Attempt to give ") + ItemString(o->item, amt) + " to self.");
		return 0;
	}

	// check for give across factions
	if (Globals->ALIGN_RESTRICT_RELATIONS && u->faction != t->faction &&
	    u->faction->alignments_ != Faction::ALL_NEUTRAL &&
	    t->faction->alignments_ != Faction::ALL_NEUTRAL)
	{
		if (u->faction->alignments_ != t->faction->alignments_)
		{
			u->Error("GIVE: Cannot give across alignments.");
			return 0;
		}
	}

	// Must be able to see unit to give something to them!
	if (!u->CanSee(r, t) &&
	    t->faction->GetAttitude(u->faction->num) < A_FRIENDLY)
	{
		u->Error(AString("GIVE: Nonexistant target (") + o->target->Print(NULL) + ").");
		return 0;
	}

	// check for friendly receiver
	if ((o->give_ship || o->item != I_SILVER) &&
	    t->faction->GetAttitude(u->faction->num) < A_FRIENDLY)
	{
		u->Error("GIVE: Target is not a member of a friendly faction.");
		return 0;
	}

	// GIVE SHIP
	if (o->give_ship)
	{
		Object *fm_obj = u->object;
		Unit *captain = fm_obj->GetOwner();
		if (u->faction->num != captain->faction->num)
		{
			u->Error("GIVE SHIP: It doesn't belong to you.");
			return 0;
		}

		// find receiver's object
		Object *to_obj = t->object;

		// TODO transfer from fleet
		if (fm_obj->type == O_ARMY)
		{
			// GIVE implies LEAVE, leave the ship
			Object *sub_obj = fm_obj->findUnitSubObject(u);
			if (sub_obj && sub_obj->num == o->item)
			{
				sub_obj->removeUnit(u, true);
				u->Event(AString("Leaves ") + *sub_obj->name);
			}
			else
			{
				// find it
				for (Object *so : fm_obj->objects)
				{
					if (so->num == o->item)
					{
						sub_obj = so;
						break;
					}
				}
				if (!sub_obj)
				{
					u->Error(AString("GIVE SHIP: Ship ") + o->item + " is not in your fleet.");
					return 0;
				}
			}

			if (fm_obj == to_obj)
			{
				if (sub_obj)
				{
					// PROMOTE new owner
					sub_obj->removeUnit(t, true);
					sub_obj->prependUnit(t);
				}
				u->Error("GIVE SHIP: Ship is already in this fleet.");
				return 0;
			}
			//else check target
			if (ObjectIsShip(to_obj->type))
			{
				// promote ship to fleet
				Object *army = new Object(r);
				army->type = O_ARMY;
				army->num = shipseq++;
				army->SetName(new AString("Fleet"));
				r->objects.remove(to_obj); // remove ship from region
				r->objects.push_back(army); // add fleet to region

				army->SetPrevDir(to_obj->prevdir); // preserve last direction
				army->objects.push_back(to_obj); // put the ship in the fleet
				to_obj->parent = army;

				// copy (not move) units into army
				for (auto &au : to_obj->getUnits())
				{
					au->object = army;
					army->addUnit(au);
				}

				// fall through to army code
				to_obj = army;
				u->object = army;
			}

			if (to_obj->type == O_ARMY)
			{
				// ship to fleet
				// move all the units over
				for (auto &au : sub_obj->getUnits())
				{
					to_obj->addUnit(au);
					au->object = to_obj;
					fm_obj->removeUnit(au, false); // leave unit in ship
				}
				// move the ship
				to_obj->objects.push_back(sub_obj);
				fm_obj->objects.erase(std::remove(fm_obj->objects.begin(), fm_obj->objects.end(), sub_obj));
				return 0;
			}
			// else just a guy in a field

			// make t captain of ship
			to_obj->removeUnit(t, true);
			fm_obj->prependUnit(t);
			t->object = to_obj;

			// remove units on ship from army
			for (auto &au : sub_obj->getUnits())
			{
				fm_obj->removeUnit(au, false); // leave unit in ship
			}

			return 0;
		} // end from fleet
		//else give our only ship
		if (!ObjectIsShip(fm_obj->type))
		{
			u->Error("GIVE SHIP: Not in a ship.");
			return 0;
		}

		// GIVE implies LEAVE
		if ((TerrainDefs[r->type].similar_type == R_OCEAN) &&
		    (!u->CanSwim() || u->GetFlag(FLAG_NOCROSS_WATER)))
		{
			u->Error("GIVE SHIP: Can't leave a ship in the ocean.");
		}
		else
		{
			u->Event(AString("Leaves ") + *fm_obj->name);
			u->MoveUnit(r->GetDummy());
		}

		// GIVE our ship
		if (to_obj == fm_obj)
		{
			// GIVE to someone else in the ship (PROMOTE)
			fm_obj->removeUnit(t, true);
			fm_obj->prependUnit(t);
			return 0;
		}

		if (!ObjectIsShip(to_obj->type))
		{
			// push unit into ship
			to_obj->removeUnit(t, true);
			fm_obj->prependUnit(t);

			// remove units on ship from army
			for (auto &au : fm_obj->getUnits())
			{
				fm_obj->removeUnit(au, false); // leave unit in ship
			}
			return 0;
		}
		//else give only ship to one ship
		if (to_obj->type != O_ARMY)
		{
			// promote to_obj to fleet
			Object *army = new Object(r);
			army->type = O_ARMY;
			army->num = shipseq++;
			army->SetName(new AString("Fleet"));
			r->objects.remove(to_obj); // remove ship from region
			r->objects.push_back(army); // add fleet to region

			army->SetPrevDir(to_obj->prevdir); // preserve last direction
			army->objects.push_back(to_obj); // put the ship in the fleet
			to_obj->parent = army;

			// copy units into army
			for (auto &au : to_obj->getUnits())
			{
				au->object = army;
				army->addUnit(au);
			}

			// fall-through
			to_obj = army;
			u->object = army;
		}
		// transfer into army/fleet
		else if (to_obj->objects.empty())
		{
			// convert army to fleet
			to_obj->SetName(new AString("Fleet"));
		}
		// add ship to fleet
		to_obj->objects.push_back(fm_obj);
		r->objects.remove(fm_obj); // remove from region

		// copy units
		for (auto &au : fm_obj->getUnits())
		{
			to_obj->addUnit(au);
			au->object = to_obj;
		}
		return 0;
	}

	ItemType &item = ItemDefs[o->item];

	// if GIVE UNIT
	if (amt == -1)
	{
		if (u->type == U_MAGE)
		{
			if (Globals->FACTION_LIMIT_TYPE != GameDefs::FACLIM_UNLIMITED)
			{
				if (CountMages(t->faction) >= AllowedMages( t->faction ))
				{
					u->Error("GIVE: Faction has too many mages.");
					return 0;
				}
			}
		}

		if (u->type == U_APPRENTICE)
		{
			if (Globals->FACTION_LIMIT_TYPE != GameDefs::FACLIM_UNLIMITED)
			{
				if (CountApprentices(t->faction) >= AllowedApprentices(t->faction))
				{
					u->Error("GIVE: Faction has too many apprentices.");
					return 0;
				}
			}
		}

		int notallied = 1;
		if (t->faction->GetAttitude(u->faction->num) == A_ALLY)
		{
			notallied = 0;
		}

		u->Event(AString("Gives unit to ") + *(t->faction->name) + ".");
		u->faction = t->faction;
		u->Event("Is given to your faction.");

		if (notallied && u->monthorders && u->monthorders->type == O_MOVE &&
		    ((MoveOrder*)u->monthorders)->advancing)
		{
			u->Error("Unit cannot advance after being given.");
			delete u->monthorders;
			u->monthorders = nullptr;
		}

		// Check if any new skill reports have to be shown
		forlist(&(u->skills))
		{
			Skill *skill = (Skill*)elem;

			const int newlvl = u->GetRealSkill(skill->type);
			const int oldlvl = u->faction->skills.GetDays(skill->type);
			if (newlvl > oldlvl)
			{
				for (int i = oldlvl + 1; i <= newlvl; ++i)
				{
					u->faction->shows.Add(new ShowSkill(skill->type, i));
				}
				u->faction->skills.SetDays(skill->type, newlvl);
			}
		}

		// Check for items shows
		{
			forlist(&u->items)
			{
				Item *i = (Item*)elem;
				u->faction->DiscoverItem(i->type, 0, 1);
			}
		}

		return notallied;
	}

	if (item.flags & ItemType::CANTGIVE)
	{
		u->Error(AString("GIVE: Can't give ") + item.names + ".");
		return 0;
	}

	// check for LIMIT <move>
	if (o->limit != M_NONE && item.weight > 0)
	{
		const int target_weight = t->Weight();
		int max_weight = 0;
		int item_capacity = 0;
		switch (o->limit)
		{
		case M_WALK:
			max_weight = t->WalkingCapacity();
			item_capacity = item.walk;
			break;

		case M_RIDE:
			max_weight = t->RidingCapacity();
			item_capacity = item.ride;
			break;

		case M_FLY:
			max_weight = t->FlyingCapacity();
			item_capacity = item.fly;
			break;

		case M_SWIM:
			max_weight = t->SwimmingCapacity();
			item_capacity = item.swim;
			break;

		case M_SAIL:
		{
			// pull ship
			Object *ship = t->object;
			if (ship)
			{
				max_weight = ObjectDefs[ship->type].capacity;
				// subtract current weight in ship
				for (auto &u2 : ship->getUnits())
				{
					// saturating subtract
					const int u2_wt = u2->Weight();
					if (u2_wt < max_weight)
						max_weight -= u2_wt;
					else
						max_weight = 0;
				}
			}
			else
			{
				// use walk capacity as a sane proxy
				max_weight = t->WalkingCapacity();
				item_capacity = item.walk;
			}
		}
		case M_NONE:;
		case M_MAX:;
		}

		// if item cannot carry itself
		if (item_capacity < item.weight)
		{
			// check current target state
			if (max_weight <= target_weight)
			{
				// it can't carry anything more!
				amt = 0; // GIVE nothing
			}
			else
			{
				const int remaining_weight = max_weight - target_weight;
				const int max_amt = remaining_weight / item.weight;
				if (amt > max_amt)
					amt = max_amt;
			}
		}
		//else item carries itself, so no worries
	} // GIVE LIMIT

	// If the item to be given is a man, combine skills
	if (item.type & IT_MAN)
	{
		if (u->type == U_MAGE || u->type == U_APPRENTICE ||
		    t->type == U_MAGE || t->type == U_APPRENTICE)
		{
			u->Error("GIVE: Magicians can't transfer men.");
			return 0;
		}

		if (o->item == I_LEADERS && t->IsNormal())
		{
			u->Error("GIVE: Can't mix leaders and normal men.");
			return 0;
		}

		if (o->item != I_LEADERS && t->IsLeader())
		{
			u->Error("GIVE: Can't mix leaders and normal men.");
			return 0;
		}

		// hack for Ceran
		if (o->item == I_MERC && t->GetMen())
		{
			u->Error("GIVE: Can't mix mercenaries with other men.");
			return 0;
		}

		if (u->faction != t->faction)
		{
			u->Error("GIVE: Can't give men to another faction.");
			return 0;
		}

		// check for max skills
		const int first_man_type = t->firstManType();

		const int man_type = item.index;
		if (first_man_type != -1 && ManDefs[man_type].max_skills != ManDefs[first_man_type].max_skills)
		{
			u->Error("GIVE: Can't mix men with different maximum number of skills.");
			return 0;
		}

		if (u->nomove) t->nomove = 1;

		SkillList *temp = u->skills.Split(u->GetMen(), amt);
		t->skills.Combine(temp);
		delete temp;
	}

	u->Event(AString("Gives ") + ItemString(o->item, amt) + " to " + *t->name + ".");

	if (u->faction != t->faction)
	{
		t->Event(AString("Receives ") + ItemString(o->item, amt) + " from " + *u->name + ".");
	}

	// update inventories
	u->items.SetNum(o->item, u->items.GetNum(o->item) - amt);
	t->items.SetNum(o->item, t->items.GetNum(o->item) + amt);

	t->faction->DiscoverItem(o->item, 0, 1);

	if (item.type & IT_MAN)
	{
		t->AdjustSkills();
	}
	return 0;
}

void Game::DoGuard1Orders()
{
	forlist(&regions)
	{
		ARegion *r = (ARegion*)elem;
		forlist(&r->objects)
		{
			Object *obj = (Object*)elem;
			for (auto &u : obj->getUnits())
			{
				if (u->guard == GUARD_SET || u->guard == GUARD_GUARD)
				{
					if (!u->Taxers())
					{
						u->guard = GUARD_NONE;
						u->Event("Must be combat ready to be on guard.");
						continue;
					}
					if (u->type != U_GUARD && r->HasCityGuard())
					{
						u->guard = GUARD_NONE;
						u->Error("Is prevented from guarding by the Guardsmen.");
						continue;
					}
					u->guard = GUARD_GUARD;
				}
			}
		}
	}
}

void Game::FindDeadFactions()
{
	forlist(&factions) {
		((Faction*)elem)->CheckExist(&regions);
	}
}

void Game::DeleteEmptyUnits()
{
	forlist(&regions) {
		DeleteEmptyInRegion((ARegion*)elem);
	}
}

void Game::DeleteEmptyInRegion(ARegion *region)
{
	forlist(&region->objects)
	{
		Object *obj = (Object*)elem;
		for (auto &unit : obj->getUnits())
		{
			if (unit->IsAlive() == 0)
			{
				region->Kill(unit);
			}
		}
	}
}


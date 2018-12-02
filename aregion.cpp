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
#include "aregion.h"
#include "game.h"
#include "gamedata.h"
#include <stdio.h>
#include <string.h>

//----------------------------------------------------------------------------
class ARegionFlatArray
{
public:
	ARegionFlatArray(int);
	~ARegionFlatArray();

	void SetRegion(int, ARegion *);
	ARegion* GetRegion(int);

	int size;
	ARegion **regions;
};

//----------------------------------------------------------------------------
const char* weatherString(int w)
{
	static const char *strs[] = {
	    "clear",
	    "winter",
	    "monsoon",
	    "blizzard"
	};
	if (w < 0 || w > W_MAX)
		return "";
	return strs[w];
}

//----------------------------------------------------------------------------
Location* GetUnit(AList *list, int n)
{
	forlist(list)
	{
		Location *l = (Location*)elem;
		if (l->unit->num == n)
			return l;
	}
	return NULL;
}

ARegionPtr* GetRegion(AList *l, int n)
{
	forlist(l)
	{
		ARegionPtr *p = (ARegionPtr*)elem;
		if (p->ptr->num == n)
			return p;
	}
	return NULL;
}

//----------------------------------------------------------------------------
Farsight::Farsight()
{
	faction = NULL;
	unit = NULL;
	level = 0;
	for (int i = 0; i < NDIRS; ++i)
		exits_used[i] = 0;
}

Farsight* GetFarsight(AList *l, Faction *fac)
{
	forlist(l)
	{
		Farsight *f = (Farsight*)elem;
		if (f->faction == fac)
			return f;
	}
	return NULL;
}

//----------------------------------------------------------------------------
static
const char* TownString(int i)
{
	static const char *names[] = {
		"village",
		"town",
		"city"
	};

	if (i < 0 || i >= NTOWNS)
		return "huh?";

	return names[i];
}

TownInfo::TownInfo()
{
	name = NULL;
	pop = 0;
	basepop = 0;
	activity = 0;
}

TownInfo::~TownInfo()
{
	delete name;
}

void TownInfo::Readin(Ainfile *f, ATL_VER &)
{
	name = f->GetStr();
	pop = f->GetInt();
	basepop = f->GetInt();
}

void TownInfo::Writeout(Aoutfile *f) const
{
	f->PutStr(*name);
	f->PutInt(pop);
	f->PutInt(basepop);
}

int TownInfo::TownType() const
{
	if (pop < Globals->CITY_POP/4) return TOWN_VILLAGE;
	if (pop < Globals->CITY_POP/2) return TOWN_TOWN;
	return TOWN_CITY;
}

//----------------------------------------------------------------------------
ARegion::ARegion()
{
	name = new AString("Region");
	xloc = 0;
	yloc = 0;
	buildingseq = 1;
	gate = 0;
	town = NULL;
	clearskies = 0;
	earthlore = 0;

	type = -1;
	race = -1;
	wages = -1;
	maxwages = -1;

	ZeroNeighbors();
}

ARegion::~ARegion()
{
	delete name;
	delete town;
}

void ARegion::ZeroNeighbors()
{
	for (int i = 0; i < NDIRS; ++i)
		neighbors[i] = NULL;
}

void ARegion::SetName(const char *c)
{
	delete name;
	name = new AString(c);
}

int ARegion::Population() const
{
	return population + (town ? town->pop : 0);
}

int ARegion::Wages()
{
	int retval = wages;
	if (town)
	{
		// hack, assumes that TownType + 1 = town wages
		retval += town->TownType() + 1;
	}

	if (earthlore) retval++;
	if (clearskies) retval++;

	if (CountConnectingRoads() > 1) retval++;

	// check Lake Wage Effect
	if (Globals->LAKE_WAGE_EFFECT == GameDefs::NO_EFFECT)
		return retval;

	// count adjacent lakes
	int adjlake = 0;
	for (int d = 0; d < NDIRS; ++d)
	{
		ARegion *check = neighbors[d];
		if (check && check->type == R_LAKE)
			++adjlake;
	}

	if (adjlake <= 0)
		return retval;

	// if lakes affect everything around them
	if (Globals->LAKE_WAGE_EFFECT & GameDefs::ALL)
		return retval + 1;

	// if lakes affect any town, even those in plains
	if ((Globals->LAKE_WAGE_EFFECT & GameDefs::TOWNS) && town)
		return retval + 1;

	// other lake effects are only for non-plains
	if (TerrainDefs[type].similar_type == R_PLAIN)
		return retval;

	// if lakes affect any non-plains terrain
	if (Globals->LAKE_WAGE_EFFECT & GameDefs::NONPLAINS)
		return retval + 1;

	// if lakes affect only desert
	if ((Globals->LAKE_WAGE_EFFECT & GameDefs::DESERT_ONLY) && TerrainDefs[type].similar_type == R_DESERT)
		return retval + 1;

	// if lakes affect towns, but only in non-plains
	if ((Globals->LAKE_WAGE_EFFECT & GameDefs::NONPLAINS_TOWNS_ONLY) && town)
		return retval + 1;

	return retval;
}

AString ARegion::WagesForReport()
{
	Production *p = products.GetProd(I_SILVER, -1);
	if (p)
	{
		return AString("$") + p->productivity + " (Max: $" + p->amount + ")";
	}

	return AString("$") + 0;
}

void ARegion::SetupPop()
{
	const TerrainType *const typer = &TerrainDefs[type];

	const int pop = typer->pop;
	if (pop == 0)
	{
		// wasteland or ocean
		population = 0;
		basepopulation = 0;
		wages = 0;
		maxwages = 0;
		money = 0;
		return;
	}

	const int noncoastalraces = sizeof(typer->races)/sizeof(int);
	const int allraces = noncoastalraces + sizeof(typer->coastal_races)/sizeof(int);

	// pick a race
	race = -1;
	while (race == -1 || (ItemDefs[race].flags & ItemType::DISABLED))
	{
		const int n = getrandom(IsCoastal() ? allraces : noncoastalraces);
		if (n > noncoastalraces-1)
		{
			race = typer->coastal_races[n-noncoastalraces-1];
		}
		else
			race = typer->races[n];
	}

	if (Globals->RANDOM_ECONOMY)
	{
		population = (pop + getrandom(pop)) / 2;
	}
	else
	{
		population = pop;
	}

	basepopulation = population;

	// setup wages
	int mw = typer->wages;
	//--- adjust for variable maintenance cost
	mw += Game::GetAvgMaintPerMan() - 10;
	if (mw < 0) mw = 0;

	if (Globals->RANDOM_ECONOMY)
	{
		mw += getrandom(5);
	}

	wages = mw;
	maxwages = mw;

	if (Globals->TOWNS_EXIST)
	{
		int adjacent = 0;
		int prob = Globals->TOWN_PROBABILITY;
		if (prob < 1) prob = 100;
		int uprob = Globals->UNDERWORLD_TOWN_PROBABILITY;
		if (uprob < 1) uprob = 100;
		int townch = (int) 80000 / prob;
		if (Globals->TOWNS_NOT_ADJACENT)
		{
			for (int d = 0; d < NDIRS; ++d)
			{
				ARegion *newregion = neighbors[d];
				if (newregion && newregion->town)
					++adjacent;
			}
		}

		if (Globals->LESS_ARCTIC_TOWNS && zloc == 1)
		{
			const int arctic_zone_size = parentRegionArray->y / 10;
			const int dnorth = GetPoleDistance(D_NORTH);
			const int dsouth = GetPoleDistance(D_SOUTH);

			if (dnorth < arctic_zone_size)
				townch += 25 * (arctic_zone_size - dnorth) * (arctic_zone_size - dnorth) * Globals->LESS_ARCTIC_TOWNS;

			if (dsouth < arctic_zone_size)
				townch += 25 * (arctic_zone_size - dsouth) * (arctic_zone_size - dsouth) * Globals->LESS_ARCTIC_TOWNS;
		}

		int spread = Globals->TOWN_SPREAD;
		if (spread > 100) spread = 100;

		int townprob = (TerrainDefs[type].economy * 4 * (100 - spread) + 100 * spread) / 100;

		if (zloc > 1)
			townprob = (townprob * uprob) / 100;

		if (adjacent > 0)
			townprob = townprob * (100 - Globals->TOWNS_NOT_ADJACENT) / 100;

		if (getrandom(townch) < townprob)
			AddTown();
	} // if town

	// add wages to products
	Production *p = new Production;
	p->itemtype = I_SILVER;
	money = Population() * (Wages() - Game::GetAvgMaintPerMan());
	p->amount = money / Globals->WORK_FRACTION;
	p->skill = -1;
	p->level = 1;
	p->productivity = Wages();
	products.Add(p);

	// add entertainment to products
	p = new Production;
	p->itemtype = I_SILVER;
	p->amount = money / Globals->ENTERTAIN_FRACTION;
	p->skill = S_ENTERTAINMENT;
	p->level = 1;
	p->productivity = Globals->ENTERTAIN_INCOME;
	products.Add(p);

	// add recruits to market
	float ratio = ItemDefs[race].baseprice / (float)Globals->BASE_MAN_COST;
	Market *m = new Market(M_BUY, race, (int)(Wages()*4*ratio),
	    Population()/5, 0, 10000, 0, 2000);
	markets.Add(m);

	// add leaders to market
	if (Globals->LEADERS_EXIST)
	{
		ratio = ItemDefs[I_LEADERS].baseprice / (float)Globals->BASE_MAN_COST;
		m = new Market(M_BUY, I_LEADERS, (int)(Wages()*4*ratio),
		    Population()/25, 0, 10000, 0, 400);
		markets.Add(m);
	}
}

int ARegion::GetNearestProd(int item)
{
	AList regs, regs2; // double buffer of search windows

	AList *rptr = &regs; // current list being searched
	AList *r2ptr = &regs2; // next generation

	// start with current region
	regs.Add(new ARegionPtr(this));

	// go out to 5 hex radius
	for (int i = 0; i < 5; ++i)
	{
		//foreach region in search list
		forlist(rptr)
		{
			ARegion *r = ((ARegionPtr*)elem)->ptr;

			// if it produces the item of interest
			if (r->products.GetProd(item, ItemDefs[item].pSkill))
			{
				// done
				regs.DeleteAll();
				regs2.DeleteAll();
				return i;
			}

			// add neighbors to next generation
			for (int j = 0; j < NDIRS; ++j)
			{
				if (neighbors[j])
				{
					ARegionPtr *p = new ARegionPtr;
					p->ptr = neighbors[j];
					r2ptr->Add(p);
				}
			}

			// swap windows
			rptr->DeleteAll();

			AList *temp = rptr;
			rptr = r2ptr;
			r2ptr = temp;
		}
	}

	regs.DeleteAll();
	regs2.DeleteAll();
	return 5;
}

void ARegion::SetupCityMarket()
{
	const int citymax = Globals->CITY_POP;

	int numtrade = 0;
	int normalbuy = 0;
	int normalsell = 0;
	int advanced = 0;
	int magic = 0;

	//foreach item
	for (int i = 0; i < NITEMS; ++i)
	{
		if (i == I_SILVER)
			continue; // not in market

		// skip if disabled or not in markets
		if (ItemDefs[i].flags & ItemType::DISABLED) continue;
		if (ItemDefs[i].flags & ItemType::NOMARKET) continue;

		// trade items handled later
		if (ItemDefs[i].type & IT_TRADE)
		{
			++numtrade;
			continue;
		}

		// specific food items
		if (i == I_GRAIN || i == I_LIVESTOCK || i == I_FISH)
		{
			if  (ItemDefs[i].flags & ItemType::NOBUY) continue;

			if (i == I_FISH && !IsCoastal())
				continue;

			int amt = Globals->CITY_MARKET_NORMAL_AMT;
			int price = ItemDefs[i].baseprice;

			if (Globals->RANDOM_ECONOMY)
			{
				amt += getrandom(amt);
				price *= 100 + getrandom(50);
				price /= 100;
			}

			int cap = (citymax * 3/4) - 1000;
			if (cap < 0) cap = citymax / 2;

			markets.Add(new Market(M_SELL, i, price, amt, population, population+cap, amt, amt*2));
			continue;
		}

		// generic food
		if (i == I_FOOD)
		{
			if  (ItemDefs[i].flags & ItemType::NOSELL) continue;

			int amt = Globals->CITY_MARKET_NORMAL_AMT;
			int price = ItemDefs[i].baseprice;

			if (Globals->RANDOM_ECONOMY)
			{
				amt += getrandom(amt);
				price *= 120 + getrandom(80);
				price /= 100;
			}

			int cap = citymax / 8;
			if (cap < citymax) cap = citymax * 3/4;

			markets.Add(new Market (M_BUY, i, price, amt, population+cap, population+6*cap, amt, amt*5));
			continue;
		}

		if (ItemDefs[i].type & IT_NORMAL)
		{
			// raw material
			if (ItemDefs[i].pInput[0].item == -1)
			{
				// check if the product can be produced in the region
				bool canProduce = false;
				for (unsigned c = 0;
				     c < (sizeof(TerrainDefs[type].prods)/sizeof(Product));
				     ++c)
				{
					if (i == TerrainDefs[type].prods[c].product)
					{
						canProduce = true;
						break;
					}
				}

				if (canProduce)
				{
					if  (ItemDefs[i].flags & ItemType::NOSELL) continue;

					// item can be produced here
					if (getrandom(2) == 0)
						continue;

					int amt = Globals->CITY_MARKET_NORMAL_AMT;
					int price = ItemDefs[i].baseprice;

					if (Globals->RANDOM_ECONOMY)
					{
						amt += getrandom(amt);
						price *= 150 + getrandom(50);
						price /= 100;
					}

					const int cap = citymax / 4;

					// raw goods have a basic offset of 0
					const int offset = (normalbuy++ * citymax * 3 / 40);

					markets.Add(new Market (M_BUY, i, price, 0, population+cap+offset, population+citymax, 0, amt));

					continue;
				}
				//else item cannot be produced in this region;
				// perhaps it is in demand here?
				if (!getrandom(6))
				{
					if  (ItemDefs[i].flags & ItemType::NOBUY) continue;

					int amt = Globals->CITY_MARKET_NORMAL_AMT;
					int price = ItemDefs[i].baseprice;

					if (Globals->RANDOM_ECONOMY)
					{
						amt += getrandom(amt);
						price *= 100 + getrandom(50);
						price /= 100;
					}

					const int cap = citymax / 4;
					const int offset = - (citymax/20) + (normalsell++ * citymax * 3/40);
					markets.Add(new Market (M_SELL, i, price, amt/6, population+cap+offset, population+citymax, 0, amt));
				}

				continue;
			}

			// finished goods
			// chance to sell
			if (getrandom(3) == 0)
			{
				if  (ItemDefs[i].flags & ItemType::NOBUY) continue;

				int amt = Globals->CITY_MARKET_NORMAL_AMT;
				int price = ItemDefs[i].baseprice;

				if (Globals->RANDOM_ECONOMY)
				{
					amt += getrandom(amt);
					price *= 100 + getrandom(50);
					price /= 100;
				}

				const int cap = citymax / 4;
				const int offset = - (citymax/20) + (normalsell++ * citymax * 3/40);

				markets.Add(new Market (M_SELL, i, price, amt/6, population+cap+offset, population+citymax, 0, amt));
				continue;
			}

			if  (ItemDefs[i].flags & ItemType::NOSELL) continue;

			// chance to buy
			if (getrandom(6) != 0)
				continue;

			int amt = Globals->CITY_MARKET_NORMAL_AMT;
			int price = ItemDefs[i].baseprice;

			if (Globals->RANDOM_ECONOMY)
			{
				amt += getrandom(amt);
				price *= 150 + getrandom(50);
				price /= 100;
			}

			const int cap = citymax / 4;
			// finished goods have a higher offset than raw goods
			// Offset added to maxamt too to reflect lesser production capacity
			// the more items are on sale.
			const int offset = (citymax/20) + (normalbuy++ * citymax * 3/40);
			markets.Add(new Market(M_BUY, i, price, 0, population+cap+offset, population+citymax+offset, 0, amt));

			continue;
		}

		// Everything else is SELL
		if  (ItemDefs[i].flags & ItemType::NOBUY) continue;

		if (ItemDefs[i].type & IT_ADVANCED)
		{
			if (getrandom(4) != 2)
				continue;

			int amt = Globals->CITY_MARKET_ADVANCED_AMT;
			int price = ItemDefs[i].baseprice;

			if (Globals->RANDOM_ECONOMY)
			{
				amt += getrandom(amt);
				price *= 100 + getrandom(50);
				price /= 100;
			}

			int cap = (citymax * 3/4) - 1000;
			if (cap < citymax/2) cap = citymax / 2;

			const int offset = (citymax / 8) * advanced++;
			if (cap+offset < citymax)
			{
				markets.Add(new Market(M_SELL, i, price, amt/6, population+cap+offset, population+citymax, 0, amt));
			}

			continue;
		}

		if ((ItemDefs[i].type & IT_MAGIC) == 0)
			continue;

		if (getrandom(8) != 2)
			continue;

		int amt = Globals->CITY_MARKET_MAGIC_AMT;
		int price = ItemDefs[i].baseprice;

		if (Globals->RANDOM_ECONOMY)
		{
			amt += getrandom(amt);
			price *= 100 + getrandom(50);
			price /= 100;
		}

		int cap = (citymax * 3/4) - 1000;
		if (cap < citymax/2) cap = citymax / 2;

		// TODO why is offset unused?
		//int offset = (citymax / 20) + ((citymax / 5) * ((magic++) + 1));
		magic++;

		markets.Add(new Market (M_SELL, i, price, amt/6, population+cap, population+citymax, 0, amt));
	}

	// set up the trade items
	int buy1 = getrandom(numtrade);
	int buy2 = getrandom(numtrade);
	int sell1 = getrandom(numtrade);
	int sell2 = getrandom(numtrade);

	buy1 = getrandom(numtrade); // TODO why twice?

	while (buy1 == buy2)
		buy2 = getrandom(numtrade);

	while (sell1 == buy1 || sell1 == buy2)
		sell1 = getrandom(numtrade);

	while (sell2 == sell1 || sell2 == buy2 || sell2 == buy1)
		sell2 = getrandom(numtrade);

	int tradebuy = 0;
	int tradesell = 0;

	for (int i = 0; i < NITEMS; ++i)
	{
		if (ItemDefs[i].flags & ItemType::DISABLED) continue;
		if (ItemDefs[i].flags & ItemType::NOMARKET) continue;

		if ((ItemDefs[i].type & IT_TRADE) == 0)
			continue;

		int addbuy = 0;
		int addsell = 0;

		if (buy1 == 0 || buy2 == 0)
			addbuy = 1;

		buy1--;
		buy2--;

		if (sell1 == 0 || sell2 == 0)
			addsell = 1;

		sell1--;
		sell2--;

		if (ItemDefs[i].flags & ItemType::NOBUY) addbuy = 0;
		if (ItemDefs[i].flags & ItemType::NOSELL) addsell = 0;

		if (addbuy)
		{
			int amt = Globals->CITY_MARKET_TRADE_AMT;
			int price = ItemDefs[i].baseprice;

			if (Globals->RANDOM_ECONOMY)
			{
				amt += getrandom(amt);
				if (Globals->MORE_PROFITABLE_TRADE_GOODS)
				{
					price *= 250 + getrandom(100);
				}
				else
				{
					price *= 150 + getrandom(50);
				}
				price /= 100;
			}

			const int cap = citymax / 2;
			const int offset = - (citymax/20) + tradesell * ((tradesell + 1) * (tradesell + 1) * citymax/40);
			tradesell++;
			if (cap + offset < citymax)
			{
				markets.Add(new Market(M_SELL, i, price, amt/5, cap+population+offset, citymax+population, 0, amt));
			}
		}

		if (addsell)
		{
			int amt = Globals->CITY_MARKET_TRADE_AMT;
			int price = ItemDefs[i].baseprice;

			{
				if (Globals->RANDOM_ECONOMY)
				{
					amt += getrandom(amt);
					if (Globals->MORE_PROFITABLE_TRADE_GOODS)
					{
						price *= 100 + getrandom(90);
					}
					else
					{
						price *= 100 + getrandom(50);
					}
					price /= 100;
				}

				const int cap = citymax / 2;
				const int offset = tradebuy++ * (citymax/6);
				if (cap+offset < citymax)
				{
					markets.Add(new Market (M_BUY, i, price, amt/6, cap+population+offset, citymax+population, 0, amt));
				}
			}
		}
	}
}

void ARegion::SetupProds()
{
	const TerrainType *const typer = &TerrainDefs[type];

	if (Globals->FOOD_ITEMS_EXIST && typer->economy)
	{
		if (getrandom(2) && !(ItemDefs[I_GRAIN].flags & ItemType::DISABLED))
		{
			products.Add(new Production(I_GRAIN, typer->economy));
		}
		else if (!(ItemDefs[I_LIVESTOCK].flags & ItemType::DISABLED))
		{
			products.Add(new Production(I_LIVESTOCK, typer->economy));
		}
	}

	for (unsigned c = 0; c < sizeof(typer->prods)/sizeof(Product); ++c)
	{
		const int item = typer->prods[c].product;
		if (item == -1 || (ItemDefs[item].flags & ItemType::DISABLED))
			continue;

		if (getrandom(100) < typer->prods[c].chance)
		{
			products.Add(new Production(item, typer->prods[c].amount));
		}
	}
}

void ARegion::AddTown()
{
	town = new TownInfo;
	town->name = new AString(AGetNameString(AGetName(1)));
	town->pop = Globals->CITY_POP / 8;
	town->activity = 0;

	if (Globals->RANDOM_ECONOMY)
	{
		int popch = Globals->CITY_POP * 16/10;

		// Underground is not affected by LESS_ARTIC_TOWNS
		if (Globals->LESS_ARCTIC_TOWNS && zloc == 1)
		{
			const int arctic_zone_size = parentRegionArray->y / 10;
			const int dnorth = GetPoleDistance(D_NORTH);
			const int dsouth = GetPoleDistance(D_SOUTH);

			// on small worlds, or the underworld levels, both distances
			// could be less than 9, so choose the smallest
			int dist = dnorth;
			if (dsouth < dist) dist = dsouth;

			if (dist < arctic_zone_size)
				popch = popch - (arctic_zone_size - dist) * ((arctic_zone_size - dist) + 10) * 15;
		}

		town->pop += getrandom(popch);
	}

	town->basepop = town->pop;

	SetupCityMarket();
}

void ARegion::LairCheck()
{
	// no lair if town in region
	if (town)
		return;

	const TerrainType *const tt = &TerrainDefs[type];
	if (!tt->lairChance)
		return;

	// check for lair
	const int check = getrandom(100);
	if (check >= tt->lairChance)
		return;

	// pick a lairtype
	int count = 0; // how many lairs are enabled
	for (unsigned c = 0; c < sizeof(tt->lairs)/sizeof(int); ++c)
	{
		const int lair_type = tt->lairs[c];
		if (lair_type == -1)
			continue;

		if (!(ObjectDefs[lair_type].flags & ObjectType::DISABLED))
		{
			++count;
		}
	}

	// pick one
	count = getrandom(count);

	// find it
	for (unsigned c = 0; c < sizeof(tt->lairs)/sizeof(int); ++c)
	{
		const int lair_type = tt->lairs[c];
		if (lair_type == -1 || (ObjectDefs[lair_type].flags & ObjectType::DISABLED))
			continue;

		if (count)
		{
			--count;
			continue;
		}

		// make it
		MakeLair(lair_type);
		return; // done
	}
}

void ARegion::MakeLair(int t)
{
	Object *o = new Object(this);
	o->num = buildingseq++;
	o->name = new AString(AString(ObjectDefs[t].name) + " [" + o->num + "]");
	o->type = t;
	o->incomplete = 0;
	o->inner = -1;
	objects.Add(o);
}

int ARegion::GetPoleDistance(int dir)
{
	int ct = 1;
	const ARegion *nreg = neighbors[dir];
	while (nreg)
	{
		ct++;
		nreg = nreg->neighbors[dir];
	}
	return ct;
}

void ARegion::Setup(ARegionArray *pArr)
{
	parentRegionArray = pArr;

	// type and location have been setup, do everything else
	SetupProds();

	SetupPop();

	// make the dummy object
	objects.Add(new Object(this));

	if (Globals->LAIR_MONSTERS_EXIST)
		LairCheck();
}

void ARegion::UpdateTown()
{
	if (!Globals->VARIABLE_ECONOMY)
		return; // nothing to do

	// check if we were a starting city and got taken over
	if (!Globals->SAFE_START_CITIES && IsStartingCity() && !HasCityGuard())
	{
		// make sure we haven't already been modified
		bool done = true;
		forlist(&markets)
		{
			Market *m = (Market*)elem;
			if (m->minamt == -1)
			{
				done = false;
				break;
			}
		}

		if (!done)
		{
			markets.DeleteAll();
			SetupCityMarket();
			const float ratio = ItemDefs[race].baseprice / (float)Globals->BASE_MAN_COST;

			// setup recruiting
			Market *m = new Market(M_BUY, race, (int)(Wages()*4*ratio),
			    Population()/5, 0, 10000, 0, 2000);
			markets.Add(m);

			if (Globals->LEADERS_EXIST)
			{
				const float ratio = ItemDefs[I_LEADERS].baseprice / (float)Globals->BASE_MAN_COST;
				m = new Market(M_BUY, I_LEADERS, (int)(Wages()*4*ratio),
				    Population()/25, 0, 10000, 0, 400);
				markets.Add(m);
			}
		}
	}

	// don't do pop stuff for AC exit
	if (town->pop == 5000)
		return;

	// first, get the target population
	int amt = 0;
	int tot = 0;

	forlist(&markets)
	{
		Market *m = (Market*)elem;
		if (Population() <= m->minpop)
			continue;

		if (ItemDefs[m->item].type & IT_TRADE)
		{
			if (m->type == M_BUY)
			{
				amt += 5 * m->activity;
				tot += 5 * m->maxamt;
			}
		}
		else
		{
			if (m->type == M_SELL)
			{
				amt += m->activity;
				tot += m->maxamt;
			}
		}
	}

	int tarpop = tot ? (Globals->CITY_POP * amt) / tot : 0;

	// bump up tarpop
	tarpop = (tarpop * 3) / 2;
	if (tarpop > Globals->CITY_POP) tarpop = Globals->CITY_POP;

	town->pop = town->pop + (tarpop - town->pop) / 5;

	// check base population
	if (town->pop < town->basepop)
	{
		town->pop = town->basepop;
	}

	if ((town->pop * 2) / 3 > town->basepop)
	{
		town->basepop = (town->pop * 2) / 3;
	}
}

void ARegion::PostTurn(ARegionList *pRegs)
{
	// first update population based on production
	if (basepopulation)
	{
		int activity = 0;
		int amount = 0;

		forlist(&products)
		{
			Production *p = (Production*)elem;
			if ((ItemDefs[p->itemtype].type & IT_NORMAL) &&
			    p->itemtype != I_SILVER)
			{
				activity += p->activity;
				amount += p->amount;
			}
		}

		if (Globals->VARIABLE_ECONOMY)
		{
			const int tarpop = basepopulation + (basepopulation * activity) / (2 * amount);
			const int diff = tarpop - population;
			population = population + diff / 5;
		}

		// if there is a town, update it
		if (town)
			UpdateTown();

		if (Globals->DECAY)
			DoDecayCheck(pRegs);

		// now, reset population based stuff
		// recover Wages
		if (wages < maxwages)
			wages++;

		// set money
		money = (Wages() - Game::GetAvgMaintPerMan()) * Population();
		if (money < 0) money = 0;

		// setup working
		Production *p = products.GetProd(I_SILVER, -1);
		if (IsStartingCity())
		{
			// higher wages in the entry cities
			p->amount = Wages() * Population();
		}
		else
		{
			p->amount = (Wages() * Population()) / Globals->WORK_FRACTION;
		}
		p->productivity = Wages();

		// entertainment
		p = products.GetProd(I_SILVER, S_ENTERTAINMENT);
		p->baseamount = money / Globals->ENTERTAIN_FRACTION;

		markets.PostTurn(Population(), Wages());
	}

	UpdateProducts();

	// set these guys to 0
	earthlore  = 0;
	clearskies = 0;

	forlist(&objects)
	{
		Object *o = (Object*)elem;
		forlist(&o->units)
		{
			Unit *u = (Unit*)elem;
			u->PostTurn(this);
		}
	}
}

void ARegion::DoDecayCheck(ARegionList *pRegs)
{
	forlist(&objects)
	{
		Object *o = (Object*)elem;
		if (!(ObjectDefs[o->type].flags & ObjectType::NEVERDECAY))
		{
			DoDecayClicks(o, pRegs);
		}
	}
}

void ARegion::DoDecayClicks(Object *o, ARegionList *pRegs)
{
	if (ObjectDefs[o->type].flags & ObjectType::NEVERDECAY)
		return;

	int clicks = getrandom(GetMaxClicks()) + PillageCheck();

	if (clicks > ObjectDefs[o->type].maxMonthlyDecay)
		clicks = ObjectDefs[o->type].maxMonthlyDecay;

	o->incomplete += clicks;

	if (o->incomplete > 0)
	{
		// trigger decay event
		RunDecayEvent(o, pRegs);
	}
}

void ARegion::RunDecayEvent(Object *o, ARegionList *pRegs)
{
	AList *pFactions = PresentFactions();
	forlist(pFactions)
	{
		Faction *f = ((FactionPtr*)elem)->ptr;
		f->Event(GetDecayFlavor() + *o->name + " " +
		    ObjectDefs[o->type].name + " in " +
		    ShortPrint(pRegs));
	}
}

AString ARegion::GetDecayFlavor() const
{
	const bool badWeather = Globals->WEATHER_EXISTS && weather != W_NORMAL && !clearskies;

	switch (type)
	{
		case R_PLAIN:
		case R_ISLAND_PLAIN:
		case R_CERAN_PLAIN1:
		case R_CERAN_PLAIN2:
		case R_CERAN_PLAIN3:
		case R_CERAN_LAKE:
			return AString("Floods have damaged ");

		case R_DESERT:
		case R_CERAN_DESERT1:
		case R_CERAN_DESERT2:
		case R_CERAN_DESERT3:
			return AString("Flashfloods have damaged ");

		case R_CERAN_WASTELAND:
		case R_CERAN_WASTELAND1:
			return AString("Magical radiation has damaged ");

		case R_TUNDRA:
		case R_CERAN_TUNDRA1:
		case R_CERAN_TUNDRA2:
		case R_CERAN_TUNDRA3:
			if (badWeather)
				return AString("Ground freezing has damaged ");

			return AString("Ground thaw has damaged ");

		case R_MOUNTAIN:
		case R_ISLAND_MOUNTAIN:
		case R_CERAN_MOUNTAIN1:
		case R_CERAN_MOUNTAIN2:
		case R_CERAN_MOUNTAIN3:
			if (badWeather)
				return AString("Avalanches have damaged ");

			return AString("Rockslides have damaged ");

		case R_CERAN_HILL:
		case R_CERAN_HILL1:
		case R_CERAN_HILL2:
			return AString("Quakes have damaged ");

		case R_FOREST:
		case R_SWAMP:
		case R_ISLAND_SWAMP:
		case R_JUNGLE:
		case R_CERAN_FOREST1:
		case R_CERAN_FOREST2:
		case R_CERAN_FOREST3:
		case R_CERAN_MYSTFOREST:
		case R_CERAN_MYSTFOREST1:
		case R_CERAN_MYSTFOREST2:
		case R_CERAN_SWAMP1:
		case R_CERAN_SWAMP2:
		case R_CERAN_SWAMP3:
		case R_CERAN_JUNGLE1:
		case R_CERAN_JUNGLE2:
		case R_CERAN_JUNGLE3:
			return AString("Encroaching vegetation has damaged ");

		case R_CAVERN:
		case R_UFOREST:
		case R_TUNNELS:
		case R_CERAN_CAVERN1:
		case R_CERAN_CAVERN2:
		case R_CERAN_CAVERN3:
		case R_CERAN_UFOREST1:
		case R_CERAN_UFOREST2:
		case R_CERAN_UFOREST3:
		case R_CERAN_TUNNELS1:
		case R_CERAN_TUNNELS2:
		case R_CHASM:
		case R_CERAN_CHASM1:
		case R_GROTTO:
		case R_CERAN_GROTTO1:
		case R_DFOREST:
		case R_CERAN_DFOREST1:
			if (badWeather)
				return AString("Lava flows have damaged ");

			return AString("Quakes have damaged ");

		default:;
	}

	return AString("Unexplained phenomena have damaged ");
}

int ARegion::GetMaxClicks() const
{
	const bool badWeather = Globals->WEATHER_EXISTS && weather != W_NORMAL && !clearskies;

	int terrainAdd = 0;
	int terrainMult = 1;
	int weatherAdd = 0;

	switch (type)
	{
	case R_PLAIN:
	case R_ISLAND_PLAIN:
	case R_TUNDRA:
	case R_CERAN_PLAIN1:
	case R_CERAN_PLAIN2:
	case R_CERAN_PLAIN3:
	case R_CERAN_LAKE:
	case R_CERAN_TUNDRA1:
	case R_CERAN_TUNDRA2:
	case R_CERAN_TUNDRA3:
		terrainAdd = -1;
		if (badWeather) weatherAdd = 4;
		break;

	case R_MOUNTAIN:
	case R_ISLAND_MOUNTAIN:
	case R_CERAN_MOUNTAIN1:
	case R_CERAN_MOUNTAIN2:
	case R_CERAN_MOUNTAIN3:
	case R_CERAN_HILL:
	case R_CERAN_HILL1:
	case R_CERAN_HILL2:
		terrainMult = 2;
		if (badWeather) weatherAdd = 4;
		break;

	case R_FOREST:
	case R_SWAMP:
	case R_ISLAND_SWAMP:
	case R_JUNGLE:
	case R_CERAN_FOREST1:
	case R_CERAN_FOREST2:
	case R_CERAN_FOREST3:
	case R_CERAN_MYSTFOREST:
	case R_CERAN_MYSTFOREST1:
	case R_CERAN_MYSTFOREST2:
	case R_CERAN_SWAMP1:
	case R_CERAN_SWAMP2:
	case R_CERAN_SWAMP3:
	case R_CERAN_JUNGLE1:
	case R_CERAN_JUNGLE2:
	case R_CERAN_JUNGLE3:
		terrainAdd = -1;
		terrainMult = 2;
		if (badWeather) weatherAdd = 1;
		break;

	case R_DESERT:
	case R_CERAN_DESERT1:
	case R_CERAN_DESERT2:
	case R_CERAN_DESERT3:
		terrainAdd = -1;
		if (badWeather) weatherAdd = 5;
		//no break;

	case R_CAVERN:
	case R_UFOREST:
	case R_TUNNELS:
	case R_CERAN_CAVERN1:
	case R_CERAN_CAVERN2:
	case R_CERAN_CAVERN3:
	case R_CERAN_UFOREST1:
	case R_CERAN_UFOREST2:
	case R_CERAN_UFOREST3:
	case R_CERAN_TUNNELS1:
	case R_CERAN_TUNNELS2:
	case R_CHASM:
	case R_CERAN_CHASM1:
	case R_GROTTO:
	case R_CERAN_GROTTO1:
	case R_DFOREST:
	case R_CERAN_DFOREST1:
		terrainAdd = 1;
		terrainMult = 2;
		if (badWeather) weatherAdd = 6;
		break;

	default:
		if (badWeather) weatherAdd = 4;
	}

	return terrainMult * (terrainAdd + 2) + (weatherAdd + 1);
}

int ARegion::PillageCheck() const
{
	const int pillageAdd = maxwages - wages;
	return (pillageAdd > 0) ? pillageAdd : 0;
}

int ARegion::HasRoad()
{
	//foreach object
	forlist(&objects)
	{
		Object *o = (Object*)elem;
		if (o->IsRoad() && o->incomplete < 1)
			return 1;
	}
	return 0;
}

int ARegion::HasExitRoad(int realDirection)
{
	forlist(&objects)
	{
		Object *o = (Object*)elem;
		if (o->IsRoad() && o->incomplete < 1)
		{
			if (o->type == GetRoadDirection(realDirection))
				return 1;
		}
	}
	return 0;
}

int ARegion::CountConnectingRoads()
{
	int connections = 0;
	for (int i = 0; i < NDIRS; ++i)
	{
		if (HasExitRoad(i) && neighbors[i] && neighbors[i]->HasConnectingRoad(i))
			++connections;
	}
	return connections;
}

int ARegion::HasConnectingRoad(int realDirection)
{
	return HasExitRoad(GetRealDirComp(realDirection));
}

int ARegion::GetRoadDirection(int realDirection)
{
	switch (realDirection)
	{
	case D_NORTH    : return O_ROADN;
	case D_NORTHEAST: return O_ROADNE;
	case D_NORTHWEST: return O_ROADNW;
	case D_SOUTH    : return O_ROADS;
	case D_SOUTHEAST: return O_ROADSE;
	case D_SOUTHWEST: return O_ROADSW;
	}
	return 0;
}

int ARegion::GetRealDirComp(int realDirection) const
{
	switch (realDirection)
	{
	case D_NORTH    : return D_SOUTH;
	case D_NORTHEAST: return D_SOUTHWEST;
	case D_NORTHWEST: return D_SOUTHEAST;
	case D_SOUTH    : return D_NORTH;
	case D_SOUTHEAST: return D_NORTHWEST;
	case D_SOUTHWEST: return D_NORTHEAST;
	}
	return 0;
}

void ARegion::UpdateProducts()
{
	forlist(&products)
	{
		Production *prod = (Production*)elem;
		if (prod->itemtype == I_SILVER && prod->skill == -1)
			continue; // skip base wages

		int lastbonus = prod->baseamount / 2;
		int bonus = 0;

		forlist(&objects)
		{
			Object *o = (Object*)elem;
			if (o->incomplete < 1 &&
			    ObjectDefs[o->type].productionAided == prod->itemtype)
			{
				lastbonus /= 2;
				bonus += lastbonus;
			}
		}

		prod->amount = prod->baseamount + bonus;

		if (prod->itemtype == I_GRAIN || prod->itemtype == I_LIVESTOCK)
		{
			prod->amount += ((earthlore + clearskies) * 40) / prod->baseamount;
		}
	}
}

AString ARegion::ShortPrint(ARegionList *pRegs)
{
	AString temp = TerrainDefs[type].name;

	temp += AString(" (") + xloc + "," + yloc;

	ARegionArray *pArr = pRegs->pRegionArrays[ zloc ];
	if (pArr->strName)
	{
		temp += ",";
		if (Globals->EASIER_UNDERWORLD &&
		    (Globals->UNDERWORLD_LEVELS+Globals->UNDERDEEP_LEVELS > 1))
		{
			temp += AString(zloc) + " <";
		}
		else
		{
			// add less explicit multilevel information about the underworld
			if (zloc > 2 && zloc < Globals->UNDERWORLD_LEVELS+2)
			{
				for (int i = zloc; i > 3; --i)
				{
					temp += "very ";
				}
				temp += "deep ";
			}
			else if (zloc > Globals->UNDERWORLD_LEVELS+2 &&
			         zloc < Globals->UNDERWORLD_LEVELS + Globals->UNDERDEEP_LEVELS + 2)
			{
				for (int i = zloc; i > Globals->UNDERWORLD_LEVELS + 3; --i)
				{
					temp += "very ";
				}
				temp += "deep ";
			}
		}

		temp += *pArr->strName;
		if (Globals->EASIER_UNDERWORLD &&
		    (Globals->UNDERWORLD_LEVELS + Globals->UNDERDEEP_LEVELS > 1))
		{
			temp += ">";
		}
	}
	temp += ")";

	temp += AString(" in ") + *name;
	return temp;
}

AString ARegion::Print(ARegionList *pRegs)
{
	AString temp = ShortPrint(pRegs);
	if (town)
	{
		temp += AString(", contains ") + *(town->name) + " [" +
		    TownString(town->TownType()) + "]";
	}
	return temp;
}

void ARegion::SetLoc(int x, int y, int z)
{
	xloc = x;
	yloc = y;
	zloc = z;
}

void ARegion::Kill(Unit *u)
{
	Unit *first = NULL; // first unit from same faction (not 'u')

	//foreach object
	forlist((&objects))
	{
		Object *obj = (Object*)elem;
		if (!obj)
			continue;

		//foreach unit in the object
		forlist((&obj->units))
		{
			if (((Unit*)elem)->faction->num == u->faction->num &&
			    ((Unit*)elem) != u)
			{
				first = (Unit*)elem;
				break;
			}
		}

		if (first)
			break;
	}

	// give u's stuff to first
	if (!first)
	{
		// nobody else here
		u->MoveUnit(0); // exit any building
		hell.Add(u);
		return;
	}

	{ // macro protect
	forlist(&u->items)
	{
		Item *i = (Item*)elem;

		if (IsSoldier(i->type))
			continue; // don't give men

		// If we're in ocean and not in a structure, make sure that the
		// first unit can actually hold the stuff and not drown.
		// If the items would cause them to drown then they will drop them
		first->items.SetNum(i->type, first->items.GetNum(i->type) + i->num);

		if (TerrainDefs[type].similar_type == R_OCEAN)
		{
			if (first->object->type == O_DUMMY)
			{
				if (!first->CanReallySwim())
				{
					// drop items
					first->items.SetNum(i->type, first->items.GetNum(i->type) - i->num);
				}
			}
		}

		u->items.SetNum(i->type, 0);
	}
	}

	u->MoveUnit(0); // exit any building
	hell.Add(u);
}

void ARegion::ClearHell()
{
	hell.DeleteAll();
}

Object* ARegion::GetObject(int num)
{
	forlist(&objects)
	{
		Object *o = (Object*)elem;
		if (o->num == num)
			return o;
	}

	return NULL;
}

Object* ARegion::GetDummy()
{
	forlist(&objects)
	{
		Object *o = (Object*)elem;
		if (o->type == O_DUMMY)
			return o;
	}

	return NULL;
}

Unit* ARegion::GetUnit(int num)
{
	forlist((&objects))
	{
		Object *obj = (Object*)elem;
		Unit *u = obj->GetUnit(num);
		if (u)
			return u;
	}

	return NULL;
}

Location* ARegion::GetLocation(UnitId *id, int faction)
{
	forlist(&objects)
	{
		Object *o = (Object*)elem;

		Unit *retval = o->GetUnitId(id, faction);
		if (retval)
		{
			Location *l = new Location;
			l->region = this;
			l->obj = o;
			l->unit = retval;
			return l;
		}
	}

	return NULL;
}

Unit* ARegion::GetUnitAlias(int alias, int faction)
{
	forlist((&objects))
	{
		Object *obj = (Object*)elem;
		Unit *u = obj->GetUnitAlias(alias, faction);
		if (u)
			return u;
	}
	return NULL;
}

Unit* ARegion::GetUnitId(UnitId *id, int faction)
{
	forlist(&objects)
	{
		Object *o = (Object*)elem;

		Unit *retval = o->GetUnitId(id, faction);
		if (retval)
			return retval;
	}
	return NULL;
}

int ARegion::Present(Faction *f)
{
	//foreach object
	forlist((&objects))
	{
		Object *obj = (Object*)elem;

		//foreach unit in object
		forlist((&obj->units))
			if (((Unit*)elem)->faction == f)
				return 1;
	}
	return 0;
}

AList* ARegion::PresentFactions()
{
	AList *facs = new AList;

	//foreach object
	forlist((&objects))
	{
		Object *obj = (Object*)elem;

		//foreach unit
		forlist((&obj->units))
		{
			Unit *u = (Unit*)elem;

			if (!GetFaction2(facs, u->faction->num))
			{
				FactionPtr *p = new FactionPtr;
				p->ptr = u->faction;
				facs->Add(p);
			}
		}
	}

	return facs;
}

void ARegion::Writeout(Aoutfile *f)
{
	f->PutStr(*name);
	f->PutInt(num);
	f->PutInt(type);
	f->PutInt(buildingseq);
	f->PutInt(gate);
	f->PutInt(race);
	f->PutInt(population);
	f->PutInt(basepopulation);
	f->PutInt(wages);
	f->PutInt(maxwages);
	f->PutInt(money);

	if (town)
	{
		f->PutInt(1);
		town->Writeout(f);
	}
	else
	{
		f->PutInt(0);
	}

	f->PutInt(xloc);
	f->PutInt(yloc);
	f->PutInt(zloc);

	products.Writeout(f);
	markets.Writeout(f);

	f->PutInt(objects.Num());
	forlist((&objects))
		((Object*)elem)->Writeout(f);
}

void ARegion::Readin(Ainfile *f, AList *facs, ATL_VER v)
{
	name = f->GetStr();

	num = f->GetInt();
	type = f->GetInt();
	buildingseq = f->GetInt();
	gate = f->GetInt();

	race = f->GetInt();
	population = f->GetInt();
	basepopulation = f->GetInt();
	wages = f->GetInt();
	maxwages = f->GetInt();
	money = f->GetInt();

	if (f->GetInt())
	{
		town = new TownInfo;
		town->Readin(f, v);
	}
	else
	{
		town = 0;
	}

	xloc = f->GetInt();
	yloc = f->GetInt();
	zloc = f->GetInt();

	products.Readin(f);
	markets.Readin(f);

	const int i = f->GetInt();
	for (int j = 0; j < i; ++j)
	{
		Object *temp = new Object(this);
		temp->Readin(f, facs, v);
		objects.Add(temp);
	}
}

int ARegion::CanMakeAdv(Faction *fac, int item)
{
	// 2 means "reveal to everyone always"
	if (Globals->FACTION_SKILLS_REVEAL_RESOURCES == 2)
		return 1;

	const int skill_id = ItemDefs[item].pSkill;
	const int skill_level = ItemDefs[item].pLevel;

	if (Globals->FACTION_SKILLS_REVEAL_RESOURCES &&
	    fac->skills.GetDays(skill_id) >= skill_level)
		return 1;

	if (Globals->IMPROVED_FARSIGHT)
	{
		forlist(&farsees)
		{
			Farsight *f = (Farsight*)elem;
			if (f && f->faction == fac && f->unit)
			{
				if (f->unit->GetSkill(skill_id) >= skill_level)
					return 1;
			}
		}
	}

	if ((Globals->TRANSIT_REPORT & GameDefs::REPORT_USE_UNIT_SKILLS) &&
	    (Globals->TRANSIT_REPORT & GameDefs::REPORT_SHOW_RESOURCES))
	{
		forlist(&passers)
		{
			Farsight *f = (Farsight*)elem;
			if (f && f->faction == fac && f->unit)
			{
				if (f->unit->GetSkill(skill_id) >= skill_level)
					return 1;
			}
		}
	}

	forlist(&objects)
	{
		Object *o = (Object*)elem;
		forlist(&o->units)
		{
			Unit *u = (Unit*)elem;
			if (u->faction == fac)
			{
				if (u->GetSkill(skill_id) >= skill_level)
				{
					return 1;
				}
			}
		}
	}

	return 0;
}

void ARegion::WriteProducts(Areport *f, Faction *fac, int present)
{
	AString temp = "Products: ";

	bool has = false;
	forlist((&products))
	{
		Production *p = (Production*)elem;

		if (ItemDefs[p->itemtype].type & IT_ADVANCED)
		{
			if (CanMakeAdv(fac, p->itemtype) || fac->IsNPC())
			{
				if (has)
				{
					temp += AString(", ") + p->WriteReport();
				}
				else
				{
					has = true;
					temp += p->WriteReport();
				}
			}
		}
		else
		{
			if (p->itemtype == I_SILVER)
			{
				if (p->skill == S_ENTERTAINMENT)
				{
					if ((Globals->TRANSIT_REPORT & GameDefs::REPORT_SHOW_ENTERTAINMENT) || present)
					{
						f->PutStr(AString("Entertainment available: $") + p->amount + ".");
					}
					else
					{
						f->PutStr(AString("Entertainment available: $0."));
					}
				}
			}
			else
			{
				if (!present && !(Globals->TRANSIT_REPORT & GameDefs::REPORT_SHOW_RESOURCES))
					continue;

				if (has)
				{
					temp += AString(", ") + p->WriteReport();
				}
				else
				{
					has = true;
					temp += p->WriteReport();
				}
			}
		}
	}

	if (!has)
		temp += "none";

	temp += ".";
	f->PutStr(temp);
}

int ARegion::HasItem(Faction *fac, int item)
{
	//foreach object in this region
	forlist(&objects)
	{
		Object *o = (Object*)elem;

		//foreach unit in the object
		forlist(&o->units)
		{
			Unit *u = (Unit*)elem;

			if (u->faction == fac)
			{
				if (u->items.GetNum(item))
					return 1;
			}
		}
	}
	return 0;
}

void ARegion::WriteMarkets(Areport *f, Faction *fac, int present)
{
	AString temp = "Wanted: ";

	bool has = false;
	forlist(&markets)
	{
		Market *m = (Market*)elem;
		if (!m->amount)
			continue;

		if (!present && !(Globals->TRANSIT_REPORT & GameDefs::REPORT_SHOW_MARKETS))
			continue;

		if (m->type == M_SELL)
		{
			if (ItemDefs[m->item].type & IT_ADVANCED)
			{
				if (!Globals->MARKETS_SHOW_ADVANCED_ITEMS)
				{
					if (!HasItem(fac, m->item))
					{
						continue;
					}
				}
			}

			if (has)
			{
				temp += ", ";
			}
			else
			{
				has = true;
			}
			temp += m->Report();
		}
	}

	if (!has)
		temp += "none";
	temp += ".";
	f->PutStr(temp);

	temp = "For Sale: ";
	has = false;

	{
		forlist(&markets)
		{
			Market *m = (Market*)elem;
			if (!m->amount)
				continue;

			if (!present && !(Globals->TRANSIT_REPORT & GameDefs::REPORT_SHOW_MARKETS))
				continue;

			if (m->type == M_BUY)
			{
				if (has)
				{
					temp += ", ";
				}
				else
				{
					has = true;
				}
				temp += m->Report();
			}
		}
	}

	if (!has)
		temp += "none";
	temp += ".";
	f->PutStr(temp);
}

void ARegion::WriteEconomy(Areport *f, Faction *fac, int present)
{
	f->AddTab();

	if ((Globals->TRANSIT_REPORT & GameDefs::REPORT_SHOW_WAGES) || present)
	{
		f->PutStr(AString("Wages: ") + WagesForReport() + ".");
	}
	else
	{
		f->PutStr(AString("Wages: $0."));
	}

	WriteMarkets(f, fac, present);

	WriteProducts(f, fac, present);

	f->EndLine();
	f->DropTab();
}

void ARegion::WriteExits(Areport *f, ARegionList *pRegs, bool *exits_seen) const
{
	f->PutStr("Exits:");
	f->AddTab();

	bool y = false;
	for (int i = 0; i < NDIRS; ++i)
	{
		ARegion *r = neighbors[i];
		if (r && exits_seen[i])
		{
			f->PutStr(AString(DirectionStrs[i]) + " : " +
			    r->Print(pRegs) + ".");
			y = true;
		}
	}

	if (!y) f->PutStr("none");
	f->DropTab();
	f->EndLine();
}

#define AC_STRING "%s Nexus is a magical place; the entryway " \
"to the world of %s. Enjoy your stay, the city guards should " \
"keep you safe as long as you should choose to stay. However, rumor " \
"has it that once you have left the Nexus, you can never return."

void ARegion::WriteReport(Areport *f, Faction *fac, int month, ARegionList *pRegions)
{
	Farsight *farsight = GetFarsight(&farsees, fac);
	Farsight *passer = GetFarsight(&passers, fac);
	const int present = Present(fac) || fac->IsNPC();

	if (!farsight && !passer && !present)
		return; // can't see anything

	AString temp = Print(pRegions);

	if (Population() &&
		 (present || farsight ||
		  (Globals->TRANSIT_REPORT & GameDefs::REPORT_SHOW_PEASANTS)))
	{
		temp += AString(", ") + Population() + " peasants";
		if (Globals->RACES_EXIST)
		{
			temp += AString(" (") + ItemDefs[race].names + ")";
		}

		if (present || farsight ||
			 Globals->TRANSIT_REPORT & GameDefs::REPORT_SHOW_REGION_MONEY)
		{
			temp += AString(", $") + money;
		}
		else
		{
			temp += AString(", $0");
		}
	}
	temp += ".";
	f->PutStr(temp);

	f->PutStr("------------------------------------------------------------");
	f->AddTab();

	if (Globals->WEATHER_EXISTS)
	{
		temp = "It was ";
		if (weather == W_BLIZZARD) temp = "There was an unnatural ";
		else if (weather == W_NORMAL) temp = "The weather was ";
		temp += SeasonNames[weather];
		temp += " last month; ";
		const int nxtweather = pRegions->GetWeather(this, (month + 1) % 12);
		temp += "it will be ";
		temp += SeasonNames[nxtweather];
		temp += " next month.";
		f->PutStr(temp);
	}

	if (type == R_NEXUS)
	{
		f->PutStr("");

		const int len = strlen(AC_STRING)+2*strlen(Globals->WORLD_NAME);
		char *nexus_desc = new char[len];
		sprintf(nexus_desc, AC_STRING, Globals->WORLD_NAME, Globals->WORLD_NAME);
		AString tmp(nexus_desc);
		if (Globals->NEXUS_NO_EXITS)
			tmp += " You will need to 'CAST Gate_Lore RANDOM' to leave the Nexus.";
		f->PutStr(tmp);
		delete[] nexus_desc;

		f->PutStr("");
	}

	f->DropTab();

	WriteEconomy(f, fac, present || farsight);

	// exits----------------
	// show none by default
	bool exits_seen[NDIRS] = {false, };

	if (present || farsight ||
		 (Globals->TRANSIT_REPORT & GameDefs::REPORT_SHOW_ALL_EXITS))
	{
		// full report
		for (int i = 0; i < NDIRS; ++i)
			exits_seen[i] = true;
	}
	else // this is just a transit report
	{
		// see if we are showing used exits
		if (Globals->TRANSIT_REPORT & GameDefs::REPORT_SHOW_USED_EXITS)
		{
			forlist(&passers)
			{
				Farsight *p = (Farsight*)elem;
				if (p->faction == fac)
				{
					for (int i = 0; i < NDIRS; ++i)
					{
						exits_seen[i] |= p->exits_used[i];
					}
				}
			}
		}
	}

	WriteExits(f, pRegions, exits_seen);

	if (Globals->GATES_EXIST && gate && gate != -1)
	{
		bool sawgate = false;
		if (fac->IsNPC())
			sawgate = true; // NPCs see everything

		if (Globals->IMPROVED_FARSIGHT && farsight)
		{
			forlist(&farsees)
			{
				Farsight *watcher = (Farsight*)elem;
				if (watcher && watcher->faction == fac && watcher->unit)
				{
					if (watcher->unit->GetSkill(S_GATE_LORE))
					{
						sawgate = true;
					}
				}
			}
		}

		if (Globals->TRANSIT_REPORT & GameDefs::REPORT_USE_UNIT_SKILLS)
		{
			forlist(&passers)
			{
				Farsight *watcher = (Farsight*)elem;
				if (watcher && watcher->faction == fac && watcher->unit)
				{
					if (watcher->unit->GetSkill(S_GATE_LORE))
					{
						sawgate = true;
					}
				}
			}
		}

		forlist(&objects)
		{
			Object *o = (Object*)elem;
			forlist(&o->units)
			{
				Unit *u = (Unit*)elem;
				if (!sawgate && (u->faction == fac && u->GetSkill(S_GATE_LORE)))
				{
					sawgate = true;
				}
			}
		}

		if (sawgate)
		{
			f->PutStr(AString("There is a Gate here (Gate ") + gate +
				 " of " + (pRegions->numberofgates) + ").");
			f->PutStr("");
		}
	}

	int detfac = 0; // detect faction

	forlist(&objects)
	{
		Object *o = (Object*)elem;
		forlist(&o->units)
		{
			Unit *u = (Unit*)elem;
			if (u->faction == fac && u->GetSkill(S_MIND_READING) > 2)
			{
				detfac = 1;
			}
		}
	}

	if (Globals->IMPROVED_FARSIGHT && farsight)
	{
		forlist(&farsees)
		{
			Farsight *watcher = (Farsight*)elem;
			if (watcher && watcher->faction == fac && watcher->unit)
			{
				if (watcher->unit->GetSkill(S_MIND_READING) > 2)
				{
					detfac = 1;
				}
			}
		}
	}

	int passdetfac = 0; // detect faction when passing through
	if ((Globals->TRANSIT_REPORT & GameDefs::REPORT_USE_UNIT_SKILLS) &&
		 (Globals->TRANSIT_REPORT & GameDefs::REPORT_SHOW_UNITS))
	{
		forlist(&passers)
		{
			Farsight *watcher = (Farsight*)elem;
			if (watcher && watcher->faction == fac && watcher->unit)
			{
				if (watcher->unit->GetSkill(S_MIND_READING) > 2)
				{
					passdetfac = 1;
				}
			}
		}
	}

	{ // protect macro
		const int obs     = fac->IsNPC() ? 10 : GetObservation(fac, 0);
		const int passobs = fac->IsNPC() ? 10 : GetObservation(fac, 1);
		const int truesight = GetTrueSight(fac, 0);
		const int passtrue  = GetTrueSight(fac, 1);
		forlist(&objects)
		{
			((Object*)elem)->Report(f, fac, obs, truesight, detfac,
				 passobs, passtrue, passdetfac, present || farsight);
		}
	}

	f->EndLine();
}

void ARegion::WriteTemplate(Areport *f, Faction *fac, ARegionList *pRegs, int month)
{
	bool header = false;

	//foreach object
	forlist(&objects)
	{
		Object *o = (Object*)elem;
		//foreach unit
		forlist(&o->units)
		{
			Unit *u = (Unit*)elem;
			if (u->faction != fac)
				continue;

			if (!header)
			{
				if (fac->temformat == TEMPLATE_MAP)
				{
					WriteTemplateHeader(f, fac, pRegs, month);
				}
				else
				{
					f->PutStr("");
					f->PutStr(AString("*** ") + Print(pRegs) + " ***", 1);
				}
				header = true;
			}

			f->PutStr("");
			f->PutStr(AString("unit ") + u->num);

			if (fac->temformat == TEMPLATE_LONG ||
			    fac->temformat == TEMPLATE_MAP)
			{
				f->PutStr(u->TemplateReport(), 1);
			}

			forlist(&(u->oldorders))
			{
				f->PutStr(*((AString*)elem));
			}
			u->oldorders.DeleteAll();

			if (!u->turnorders.First())
				continue;

			bool first = true;
			{ // macro protect
			forlist(&u->turnorders)
			{
				TurnOrder *tOrder = (TurnOrder*)elem;
				if (first)
				{
					forlist(&tOrder->turnOrders)
					{
						f->PutStr(*((AString*)elem));
					}
					first = false;
				}
				else
				{
					if (tOrder->repeating)
						f->PutStr(AString("@TURN"));
					else
						f->PutStr(AString("TURN"));

					forlist(&tOrder->turnOrders)
					{
						f->PutStr(*((AString*)elem));
					}
					f->PutStr(AString("ENDTURN"));
				}
			}
			}

			TurnOrder *tOrder = (TurnOrder*)u->turnorders.First();

			if (tOrder->repeating)
			{
				f->PutStr(AString("@TURN"));
				forlist(&tOrder->turnOrders)
				{
					f->PutStr(*((AString*)elem));
				}
				f->PutStr(AString("ENDTURN"));
			}

			u->turnorders.DeleteAll();
		}
	}
}

int ARegion::GetTrueSight(Faction *f, int usepassers)
{
	int truesight = 0;

	if (Globals->IMPROVED_FARSIGHT)
	{
		forlist(&farsees)
		{
			Farsight *farsight = (Farsight*)elem;
			if (farsight && farsight->faction == f && farsight->unit)
			{
				const int t = farsight->unit->GetSkill(S_TRUE_SEEING);
				if (t > truesight) truesight = t;
			}
		}
	}

	if (usepassers &&
	    (Globals->TRANSIT_REPORT & GameDefs::REPORT_USE_UNIT_SKILLS) &&
	    (Globals->TRANSIT_REPORT & GameDefs::REPORT_SHOW_UNITS))
	{
		forlist(&passers)
		{
			Farsight *farsight = (Farsight*)elem;
			if (farsight && farsight->faction == f && farsight->unit)
			{
				const int t = farsight->unit->GetSkill(S_TRUE_SEEING);
				if (t > truesight) truesight = t;
			}
		}
	}

	forlist((&objects))
	{
		Object *obj = (Object*)elem;
		forlist((&obj->units))
		{
			Unit *u = (Unit*)elem;
			if (u->faction == f)
			{
				const int temp = u->GetSkill(S_TRUE_SEEING);
				if (temp > truesight) truesight = temp;
			}
		}
	}

	return truesight;
}

int ARegion::GetObservation(Faction *f, int usepassers)
{
	int obs = 0;

	if (Globals->IMPROVED_FARSIGHT)
	{
		forlist(&farsees)
		{
			Farsight *farsight = (Farsight*)elem;
			if (farsight && farsight->faction == f && farsight->unit)
			{
				const int o = farsight->unit->GetSkill(S_OBSERVATION);
				if (o > obs) obs = o;
			}
		}
	}

	if (usepassers &&
	    (Globals->TRANSIT_REPORT & GameDefs::REPORT_USE_UNIT_SKILLS) &&
	    (Globals->TRANSIT_REPORT & GameDefs::REPORT_SHOW_UNITS))
	{
		forlist(&passers)
		{
			Farsight *farsight = (Farsight*)elem;
			if (farsight && farsight->faction == f && farsight->unit)
			{
				const int o = farsight->unit->GetSkill(S_OBSERVATION);
				if (o > obs) obs = o;
			}
		}
	}

	forlist((&objects))
	{
		Object *obj = (Object*)elem;
		forlist((&obj->units))
		{
			Unit *u = (Unit*)elem;
			if (u->faction == f)
			{
				const int temp = u->GetSkill(S_OBSERVATION);
				if (temp > obs) obs = temp;
			}
		}
	}

	return obs;
}

void ARegion::SetWeather(int newWeather)
{
	weather = newWeather;
}

int ARegion::IsCoastal()
{
	if (type != R_LAKE && TerrainDefs[type].similar_type == R_OCEAN)
		return 1;

	int seacount = 0;
	for (int i = 0; i < NDIRS; ++i)
	{
		// if neighbor is ocean-like
		if (neighbors[i] && TerrainDefs[neighbors[i]->type].similar_type == R_OCEAN)
		{
			// if counting lakes or not lake
			if (Globals->LAKESIDE_IS_COASTAL || neighbors[i]->type != R_LAKE)
				++seacount;
		}
	}

	return seacount;
}

int ARegion::IsCoastalOrLakeside()
{
	if (type != R_LAKE && TerrainDefs[type].similar_type == R_OCEAN)
		return 1;

	int seacount = 0;
	for (int i = 0; i < NDIRS; ++i)
	{
		if (neighbors[i] && TerrainDefs[neighbors[i]->type].similar_type == R_OCEAN)
		{
			++seacount;
		}
	}
	return seacount;
}

int ARegion::MoveCost(int movetype, ARegion *fromRegion, int dir, AString *road)
{
	int cost = 1; // base move cost

	if (Globals->WEATHER_EXISTS)
	{
		if (weather == W_BLIZZARD)
			return 10; // shut down movement

		// double cost in bad weather ('clearskies' overrides)
		if (weather != W_NORMAL && !clearskies)
			cost = 2;
	}

	if (movetype == M_WALK || movetype == M_RIDE)
	{
		// ground based units pay terrain costs
		cost *= TerrainDefs[type].movepoints;

		// and can benefit from roads
		if (fromRegion->HasExitRoad(dir) && HasConnectingRoad(dir))
		{
			cost -= cost/2;

			// update move message
			if (road)
				*road = " on a road";
		}
	}

	// prevent underflow (just in case)
	return (cost < 1) ? 1 : cost;
}

Unit* ARegion::Forbidden(Unit *u)
{
	forlist((&objects))
	{
		Object *obj = (Object*)elem;
		forlist((&obj->units))
		{
			Unit *u2 = (Unit*)elem;
			if (u2->Forbids(this, u))
				return u2;
		}
	}

	return NULL;
}

Unit* ARegion::ForbiddenByAlly(Unit *u)
{
	forlist((&objects))
	{
		Object *obj = (Object*)elem;
		forlist((&obj->units))
		{
			Unit *u2 = (Unit*)elem;
			if (u->faction->GetAttitude(u2->faction->num) == A_ALLY &&
			    u2->Forbids(this, u))
				return u2;
		}
	}
	return NULL;
}

int ARegion::HasCityGuard()
{
	forlist((&objects))
	{
		Object *obj = (Object*)elem;
		forlist((&obj->units))
		{
			Unit *u = (Unit*)elem;
			if (u->type == U_GUARD && u->GetSoldiers() &&
			    u->guard == GUARD_GUARD)
			{
				return 1;
			}
		}
	}
	return 0;
}

void ARegion::NotifySpell(Unit *caster, int spell, ARegionList *pRegs)
{
	AList flist;

	forlist((&objects))
	{
		Object *o = (Object*)elem;
		forlist((&o->units))
		{
			Unit *u = (Unit*)elem;
			if (u->faction == caster->faction)
				continue;

			if (u->GetSkill(spell))
			{
				if (!GetFaction2(&flist, u->faction->num))
				{
					FactionPtr *fp = new FactionPtr;
					fp->ptr = u->faction;
					flist.Add(fp);
				}
			}
		}
	}

	{
		forlist(&flist)
		{
			FactionPtr *fp = (FactionPtr*)elem;
			fp->ptr->Event(AString(*(caster->name)) + " uses " +
			    SkillStrs(spell) + " in " + Print(pRegs) + ".");
		}
	}
}

void ARegion::NotifyCity(Unit *caster, const AString &oldname, const AString &newname)
{
	AList flist;
	forlist((&objects))
	{
		Object *o = (Object*)elem;
		forlist((&o->units))
		{
			Unit *u = (Unit*)elem;
			if (u->faction == caster->faction)
				continue;

			if (!GetFaction2(&flist, u->faction->num))
			{
				FactionPtr *fp = new FactionPtr;
				fp->ptr = u->faction;
				flist.Add(fp);
			}
		}
	}

	{
		forlist(&flist)
		{
			FactionPtr *fp = (FactionPtr*)elem;
			fp->ptr->Event(AString(*(caster->name)) + " renames " +
			    oldname + " to " + newname + ".");
		}
	}
}

int ARegion::CanTax(Unit *u)
{
	forlist((&objects))
	{
		Object *obj = (Object*)elem;
		forlist((&obj->units))
		{
			Unit *u2 = (Unit*)elem;
			if (u2->guard == GUARD_GUARD && u2->IsAlive())
				if (u2->GetAttitude(this, u) <= A_NEUTRAL)
					return 0;
		}
	}
	return 1;
}

int ARegion::CanPillage(Unit *u)
{
	forlist(&objects)
	{
		Object *obj = (Object*)elem;
		forlist(&obj->units)
		{
			Unit *u2 = (Unit*)elem;
			if (u2->guard == GUARD_GUARD && u2->IsAlive() &&
			    u2->faction != u->faction)
				return 0;
		}
	}
	return 1;
}

int ARegion::ForbiddenShip(Object *ship)
{
	forlist(&ship->units)
	{
		Unit *u = (Unit*)elem;
		if (Forbidden(u))
			return 1;
	}
	return 0;
}

void ARegion::DefaultOrders()
{
	forlist((&objects))
	{
		Object *obj = (Object*)elem;
		forlist((&obj->units))
			((Unit*)elem)->DefaultOrders(obj);
	}
}

// just used for mapping; just check if there is an inner region
int ARegion::HasShaft()
{
	forlist(&objects)
	{
		Object *o = (Object*)elem;
		if (o->inner != -1)
			return 1;
	}
	return 0;
}

int ARegion::IsGuarded()
{
	forlist(&objects)
	{
		Object *o = (Object*)elem;
		forlist(&o->units)
		{
			Unit *u = (Unit*)elem;
			if (u->guard == GUARD_GUARD)
				return 1;
		}
	}
	return 0;
}

int ARegion::CountWMons()
{
	int count = 0;
	forlist(&objects)
	{
		Object *o = (Object*)elem;
		forlist(&o->units)
		{
			Unit *u = (Unit*)elem;
			if (u->type == U_WMON)
			{
				++count;
			}
		}
	}
	return count;
}

//----------------------------------------------------------------------------
ARegionList::ARegionList()
{
	pRegionArrays = NULL;
	numLevels = 0;
	numberofgates = 0;
}

ARegionList::~ARegionList()
{
	if (pRegionArrays)
	{
		for (int i = 0; i < numLevels; ++i)
		{
			delete pRegionArrays[i];
		}

		delete[] pRegionArrays;
	}
}

void ARegionList::WriteRegions(Aoutfile *f)
{
	f->PutInt(Num());
	f->PutInt(numLevels);

	for (int i = 0; i < numLevels; ++i)
	{
		ARegionArray *pRegs = pRegionArrays[i];
		f->PutInt(pRegs->x);
		f->PutInt(pRegs->y);

		if (pRegs->strName)
		{
			f->PutStr(*pRegs->strName);
		}
		else
		{
			f->PutStr("none");
		}

		f->PutInt(pRegs->levelType);
	}

	f->PutInt(numberofgates);

	{
		forlist(this)
		{
			((ARegion*)elem)->Writeout(f);
		}
	}

	{
		f->PutStr("Neighbors");
		forlist(this)
		{
			ARegion *reg = (ARegion*)elem;
			for (int i = 0; i < NDIRS; ++i)
			{
				if (reg->neighbors[i])
				{
					f->PutInt(reg->neighbors[i]->num);
				}
				else
				{
					f->PutInt(-1);
				}
			}
		}
	}
}

int ARegionList::ReadRegions(Ainfile *f, AList *factions, ATL_VER v)
{
	const int num = f->GetInt();

	numLevels = f->GetInt();
	CreateLevels(numLevels);

	for (int i = 0; i < numLevels; ++i)
	{
		const int curX = f->GetInt();
		const int curY = f->GetInt();
		AString *name = f->GetStr();
		ARegionArray *pRegs = new ARegionArray(curX, curY);
		if (*name == "none")
		{
			pRegs->strName = NULL;
			delete name;
		}
		else
		{
			pRegs->strName = name;
		}
		pRegs->levelType = f->GetInt();
		pRegionArrays[i] = pRegs;
	}

	numberofgates = f->GetInt();

	ARegionFlatArray fa(num);

	Awrite("Reading the regions...");
	for (int i = 0; i < num; ++i)
	{
		ARegion *temp = new ARegion;
		temp->Readin(f, factions, v);
		fa.SetRegion(temp->num, temp);
		Add(temp);

		pRegionArrays[ temp->zloc ]->SetRegion(temp->xloc, temp->yloc, temp);
	}

	Awrite("Setting up the neighbors...");

	{
		delete f->GetStr();
		forlist(this)
		{
			ARegion *reg = (ARegion*)elem;
			for (int i = 0; i < NDIRS; ++i)
			{
				const int j = f->GetInt();
				if (j != -1)
				{
					reg->neighbors[i] = fa.GetRegion(j);
				}
				else
				{
					reg->neighbors[i] = 0;
				}
			}
		}
	}
	return 1;
}

ARegion* ARegionList::GetRegion(int n)
{
	forlist(this)
	{
		if (((ARegion*)elem)->num == n)
			return (ARegion*)elem;
	}
	return NULL;
}

ARegion* ARegionList::GetRegion(int x, int y, int z)
{
	if (z >= numLevels)
		return NULL;

	ARegionArray *arr = pRegionArrays[z];

	x = (x + arr->x) % arr->x;
	y = (y + arr->y) % arr->y;

	return arr->GetRegion(x, y);
}

Location* ARegionList::FindUnit(int i)
{
	forlist(this)
	{
		ARegion *reg = (ARegion*)elem;
		forlist((&reg->objects))
		{
			Object *obj = (Object*)elem;
			forlist((&obj->units))
			{
				Unit *u = (Unit*)elem;
				if (u->num == i)
				{
					Location *retval = new Location;
					retval->unit = u;
					retval->region = reg;
					retval->obj = obj;
					return retval;
				}
			}
		}
	}
	return NULL;
}

void ARegionList::NeighSetup(ARegion *r, ARegionArray *ar)
{
	r->ZeroNeighbors();

	if (r->yloc != 0 && r->yloc != 1)
	{
		r->neighbors[D_NORTH] = ar->GetRegion(r->xloc, r->yloc - 2);
	}

	if (r->yloc != 0)
	{
		r->neighbors[D_NORTHEAST] = ar->GetRegion(r->xloc + 1, r->yloc - 1);
		r->neighbors[D_NORTHWEST] = ar->GetRegion(r->xloc - 1, r->yloc - 1);
	}

	if (r->yloc != ar->y - 1)
	{
		r->neighbors[D_SOUTHEAST] = ar->GetRegion(r->xloc + 1, r->yloc + 1);
		r->neighbors[D_SOUTHWEST] = ar->GetRegion(r->xloc - 1, r->yloc + 1);
	}

	if (r->yloc != ar->y - 1 && r->yloc != ar->y - 2)
	{
		r->neighbors[D_SOUTH] = ar->GetRegion(r->xloc, r->yloc + 2);
	}
}

void ARegionList::CreateAbyssLevel(int level, const char *name)
{
	MakeRegions(level, 4, 4);
	pRegionArrays[level]->SetName(name);
	pRegionArrays[level]->levelType = ARegionArray::LEVEL_NEXUS;

	ARegion *reg = NULL;
	for (int x = 0; x < 4; ++x)
	{
		for (int y = 0; y < 4; ++y)
		{
			reg = pRegionArrays[level]->GetRegion(x, y);
			if (!reg)
				continue;

			reg->SetName("Abyssal Plains");
			reg->type = R_DESERT;
			reg->wages = -2;
		}
	}

	if (Globals->GATES_EXIST)
	{
		bool gateset = false;
		do
		{
			const int tempx = getrandom(4);
			const int tempy = getrandom(4);
			reg = pRegionArrays[level]->GetRegion(tempx, tempy);
			if (reg)
			{
				gateset = true;
				numberofgates++;
				reg->gate = -1;
			}
		} while(!gateset);
	}

	FinalSetup(pRegionArrays[level]);

	ARegion *lair = NULL;
	do
	{
		const int tempx = getrandom(4);
		const int tempy = getrandom(4);
		lair = pRegionArrays[level]->GetRegion(tempx, tempy);

	} while(!lair || lair == reg);

	Object *o = new Object(lair);
	o->num = lair->buildingseq++;
	o->name = new AString(AString(ObjectDefs[O_BKEEP].name)+" ["+o->num+"]");
	o->type = O_BKEEP;
	o->incomplete = 0;
	o->inner = -1;
	lair->objects.Add(o);
}

void ARegionList::CreateNexusLevel(int level, int xSize, int ySize, const char *name)
{
	MakeRegions(level, xSize, ySize);

	pRegionArrays[level]->SetName(name);
	pRegionArrays[level]->levelType = ARegionArray::LEVEL_NEXUS;

	AString nex_name = Globals->WORLD_NAME;
	nex_name += " Nexus";

	for (int y = 0; y < ySize; ++y)
	{
		for (int x = 0; x < xSize; ++x)
		{
			ARegion *reg = pRegionArrays[level]->GetRegion(x, y);
			if (reg)
			{
				reg->SetName(nex_name.Str());
				reg->type = R_NEXUS;
			}
		}
	}

	FinalSetup(pRegionArrays[level]);

	for (int y = 0; y < ySize; ++y)
	{
		for (int x = 0; x < xSize; ++x)
		{
			ARegion *reg = pRegionArrays[level]->GetRegion(x, y);
			if (reg && Globals->NEXUS_IS_CITY && Globals->TOWNS_EXIST)
			{
				reg->MakeStartingCity();
				if (Globals->GATES_EXIST)
				{
					numberofgates++;
				}
			}
		}
	}
}

void ARegionList::CreateSurfaceLevel(int level, int xSize, int ySize,
    int percentOcean, int continentSize, const char *name)
{
	MakeRegions(level, xSize, ySize);

	pRegionArrays[level]->SetName(name);
	pRegionArrays[level]->levelType = ARegionArray::LEVEL_SURFACE;
	MakeLand(pRegionArrays[level], percentOcean, continentSize);

	if (Globals->LAKES_EXIST)
		CleanUpWater(pRegionArrays[level]);

	SetupAnchors(pRegionArrays[level]);

	GrowTerrain(pRegionArrays[level], 0);

	AssignTypes(pRegionArrays[level]);

	if (Globals->ARCHIPELAGO || Globals->LAKES_EXIST)
		SeverLandBridges(pRegionArrays[level]);

	if (Globals->LAKES_EXIST)
		RemoveCoastalLakes(pRegionArrays[level]);

	FinalSetup(pRegionArrays[level]);
}

void ARegionList::CreateIslandLevel(int level, int nPlayers, const char *name)
{
	const int xSize = 20 + (nPlayers + 3) / 4 * 6 - 2;
	const int ySize = xSize;

	MakeRegions(level, xSize, ySize);

	pRegionArrays[level]->SetName(name);
	pRegionArrays[level]->levelType = ARegionArray::LEVEL_SURFACE;

	MakeCentralLand(pRegionArrays[level]);
	MakeIslands(pRegionArrays[level], nPlayers);
	RandomTerrain(pRegionArrays[level]);

	FinalSetup(pRegionArrays[level]);
}

void ARegionList::CreateUnderworldLevel(int level, int xSize, int ySize,
    const char *name)
{
	MakeRegions(level, xSize, ySize);

	pRegionArrays[level]->SetName(name);
	pRegionArrays[level]->levelType = ARegionArray::LEVEL_UNDERWORLD;

	SetRegTypes(pRegionArrays[level], R_NUM);

	SetupAnchors(pRegionArrays[level]);

	GrowTerrain(pRegionArrays[level], 1);

	AssignTypes(pRegionArrays[level]);

	MakeUWMaze(pRegionArrays[level]);

	FinalSetup(pRegionArrays[level]);
}

void ARegionList::CreateUnderdeepLevel(int level, int xSize, int ySize,
    const char *name)
{
	MakeRegions(level, xSize, ySize);

	pRegionArrays[level]->SetName(name);
	pRegionArrays[level]->levelType = ARegionArray::LEVEL_UNDERDEEP;

	SetRegTypes(pRegionArrays[level], R_NUM);

	SetupAnchors(pRegionArrays[level]);

	GrowTerrain(pRegionArrays[level], 1);

	AssignTypes(pRegionArrays[level]);

	MakeUWMaze(pRegionArrays[level]);

	FinalSetup(pRegionArrays[level]);
}

void ARegionList::MakeRegions(int level, int xSize, int ySize)
{
	Awrite("Making a level...");

	ARegionArray *arr = new ARegionArray(xSize, ySize);
	pRegionArrays[level] = arr;

	// make the regions themselves
	for (int y = 0; y < ySize; ++y)
	{
		for (int x = 0; x < xSize; ++x)
		{
			if (((x + y) % 2) != 0)
				continue;

			ARegion *reg = new ARegion;
			reg->SetLoc(x, y, level);
			reg->num = Num();

			Add(reg);
			arr->SetRegion(x, y, reg);
		}
	}

	SetupNeighbors(arr);

	Awrite("");
}

void ARegionList::SetupNeighbors(ARegionArray *pRegs)
{
	for (int x = 0; x < pRegs->x; ++x)
	{
		for (int y = 0; y < pRegs->y; ++y)
		{
			ARegion *reg = pRegs->GetRegion(x, y);
			if (!reg)
				continue;

			NeighSetup(reg, pRegs);
		}
	}
}

void ARegionList::MakeLand(ARegionArray *pRegs, int percentOcean,
    int continentSize)
{
	Awrite("Making land");

	const int total = pRegs->x * pRegs->y / 2;
	int ocean = total;

	while (ocean > (total * percentOcean) / 100)
	{
		int sz = getrandom(continentSize);
		sz = sz * sz;

		const int tempx = getrandom(pRegs->x);
		const int tempy = getrandom(pRegs->y / 2) * 2 + tempx % 2;

		ARegion *reg = pRegs->GetRegion(tempx, tempy);
		ARegion *newreg = reg;
		ARegion *seareg = reg;

		// archipelago or continent?
		if (getrandom(100) < Globals->ARCHIPELAGO)
		{
			// make an archipelago
			sz = sz / 5 + 1;
			bool first = true;
			int tries = 0;
			for (int i = 0; i < sz; ++i)
			{
				int direc = getrandom(NDIRS);
				newreg = reg->neighbors[direc];

				while (!newreg)
				{
					direc = getrandom(6);
					newreg = reg->neighbors[direc];
				}

				++tries;

				for (int m = 0; m < 2; ++m)
				{
					seareg = newreg;
					newreg = seareg->neighbors[direc];
					if (!newreg)
						break;
				}

				if (!newreg)
					continue;

				seareg = newreg;

				newreg = seareg->neighbors[getrandom(NDIRS)];
				while (!newreg)
					newreg = seareg->neighbors[getrandom(NDIRS)];

				// island start point (~3 regions away from last island)
				seareg = newreg;

				if (first)
				{
					seareg = reg;
					first = false;
				}

				// don't change existing terrain
				if (seareg->type != -1)
				{
					if (tries > 5)
						break;

					continue;
				}

				reg = seareg;
				tries = 0;
				reg->type = R_NUM;
				ocean--;

				int growit = getrandom(20);
				int growth = 0;
				int growch = 2;

				// grow this island
				while (growit > growch)
				{
					growit = getrandom(20);
					tries = 0;

					int newdir = getrandom(NDIRS);
					while (newdir == reg->GetRealDirComp(direc))
						newdir = getrandom(NDIRS);

					newreg = reg->neighbors[newdir];

					while (!newreg && tries < 36)
					{
						while (newdir == reg->GetRealDirComp(direc))
							newdir = getrandom(NDIRS);

						newreg = reg->neighbors[newdir];
						tries++;
					}

					if (!newreg)
						continue;

					reg = newreg;
					tries = 0;

					// don't change existing terrain
					if (reg->type != -1)
						continue;

					reg->type = R_NUM;
					growth++;
					if (growth > growch) growch = growth;
					ocean--;

				} // while growing island

			} // for size of archipelago

			continue; // while too much ocean
		} // if archipelago
		//else

		// make a continent
		if (reg->type == -1)
		{
			reg->type = R_NUM;
			ocean--;
		}

		for (int i = 0; i < sz; ++i)
		{
			ARegion *newreg = reg->neighbors[getrandom(NDIRS)];
			while (!newreg)
				newreg = reg->neighbors[getrandom(NDIRS)];

			reg = newreg;
			if (reg->type == -1)
			{
				reg->type = R_NUM;
				ocean--;
			}
		}
	} // while too much ocean

	// at this point, go back through and set all the rest to ocean
	SetRegTypes(pRegs, R_OCEAN);
	Awrite("");
}

void ARegionList::MakeCentralLand(ARegionArray *pRegs)
{
	for (int i = 0; i < pRegs->x; ++i)
	{
		for (int j = 0; j < pRegs->y; ++j)
		{
			ARegion *reg = pRegs->GetRegion(i, j);
			if (!reg)
				continue;

			// initialize region to ocean
			reg->type = R_OCEAN;

			// if the region is close to the edges, it stays ocean
			if (i < 8 || i >= pRegs->x - 8 || j < 8 || j >= pRegs->y - 8)
				continue;

			// if the region is within 10 of the edges, it has a 50%
			// chance of staying ocean
			if (i < 10 || i >= pRegs->x - 10 || j < 10 || j >= pRegs->y - 10)
			{
				if (getrandom(100) > 50)
					continue;
			}

			// otherwise, set the region to land
			reg->type = R_NUM;
		}
	}
}

void ARegionList::MakeIslands(ARegionArray *pArr, int nPlayers)
{
	// first, make the islands along the top
	int nRow = (nPlayers + 3) / 4;

	for (int i = 0; i < nRow; ++i)
		MakeOneIsland(pArr, 10 + i * 6, 2);

	// next, along the left
	nRow = (nPlayers + 2) / 4;
	for (int i = 0; i < nRow; ++i)
		MakeOneIsland(pArr, 2, 10 + i * 6);

	// islands along the bottom
	nRow = (nPlayers + 1) / 4;
	for (int i = 0; i < nRow; ++i)
		MakeOneIsland(pArr, 10 + i * 6, pArr->y - 6);

	// and the islands on the right
	nRow = nPlayers / 4;
	for (int i = 0; i < nRow; ++i)
		MakeOneIsland(pArr, pArr->x - 6, 10 + i * 6);
}

void ARegionList::MakeOneIsland(ARegionArray *pRegs, int xx, int yy)
{
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			ARegion *pReg = pRegs->GetRegion(i + xx, j + yy);
			if (pReg)
				pReg->type = R_NUM;
		}
	}
}

void ARegionList::CleanUpWater(ARegionArray *pRegs)
{
	// check all ocean regions for inland sea status... repeat six times!
	// it's an ugly set of loops, but it works!
	Awrite("Converting Scattered Water");

	for (int ctr = 0; ctr < 6; ++ctr)
	{
		for (int i = 0; i < pRegs->x; ++i)
		{
			for (int j = 0; j < pRegs->y; ++j)
			{
				ARegion *reg = pRegs->GetRegion(i, j);
				if (!reg || reg->type != R_OCEAN)
					continue;

				bool remainocean = false;
				for (int d = 0; d < NDIRS; ++d)
				{
					ARegion *newregion1 = reg->neighbors[d];
					if (!newregion1 || newregion1->type != R_OCEAN)
						continue;

					for (int d1 = -1; d1 < 2; ++d1)
					{
						int direc = d + d1;
						if (direc < 0)
							direc = NDIRS - direc;
						if (direc >= NDIRS)
							direc = direc - NDIRS;

						ARegion *newregion2 = newregion1->neighbors[direc];
						if (!newregion2 || newregion2->type != R_OCEAN)
							continue;

						for (int d2 = -1; d2 < 2; ++d2)
						{
							direc = d + d2;
							if (direc < 0)
								direc = NDIRS - direc;
							if (direc >= NDIRS)
								direc = direc - NDIRS;

							ARegion *newregion3 = newregion2->neighbors[direc];
							if (!newregion3 || newregion3->type != R_OCEAN)
								continue;

							for (int d3 = -1; d3< 2; ++d3)
							{
								direc = d + d3;
								if (direc < 0)
									direc = NDIRS - direc;
								if (direc >= NDIRS)
									direc = direc - NDIRS;

								ARegion *newregion4 = newregion3->neighbors[direc];
								if (!newregion4 || newregion4->type != R_OCEAN)
									continue;

								for (int d4 = -1; d4 < 2; ++d4)
								{
									direc = d + d4;
									if (direc < 0)
										direc = NDIRS - direc;
									if (direc >= NDIRS)
										direc = direc - NDIRS;

									ARegion *newregion5 = newregion4->neighbors[direc];
									if (!newregion5 || newregion5->type != R_OCEAN)
										continue;

									remainocean = true;
								}
							}
						}
					}
				}

				if (remainocean)
					continue;

				reg->wages = 0;
				if (getrandom(100) < Globals->LAKES_EXIST)
				{
					reg->type = R_LAKE;
				}
				else
					reg->type = R_NUM;

				Adot();
			}
		}
	}

	Awrite("");
}

void ARegionList::RemoveCoastalLakes(ARegionArray *pRegs)
{
	Awrite("Removing coastal 'lakes'");

	for (int c = 0; c < 2; ++c)
	{
		//foreach x
		for (int i = 0; i < pRegs->x; ++i)
		{
			//foreach y
			for (int j = 0; j < pRegs->y; ++j)
			{
				ARegion *reg = pRegs->GetRegion(i, j);
				if (!reg || reg->type != R_LAKE)
					continue;

				if (reg->IsCoastal() > 0)
				{
					reg->type = R_OCEAN;
					reg->wages = -1;
					Adot();
					continue;
				}

				if (reg->wages > 0)
					continue;

				// name the Lake
				int wage1 = 0;
				int count1 = 0;
				int wage2 = 0;
				int count2 = 0;

				for (int d = 0; d < NDIRS; ++d)
				{
					ARegion *newregion = reg->neighbors[d];
					if (!newregion)
						continue;

					// name after neighboring lake regions preferentially
					if (newregion->wages > 0 && newregion->type == R_LAKE)
					{
						count1 = 1;
						wage1 = newregion->wages;
						break;
					}

					if (TerrainDefs[newregion->type].similar_type != R_OCEAN && newregion->wages > -1)
					{
						if (newregion->wages == wage1) count1++;
						else if (newregion->wages == wage2) count2++;
						else if (count2 == 0)
						{
							wage2 = newregion->wages;
							count2++;
						}

						if (count2 > count1)
						{
							const int temp = wage1;
							wage1 = wage2;
							wage2 = temp;

							const int tmpin = count1;
							count1 = count2;
							count2 = tmpin;
						}
					}
				}

				if (count1 > 0)
					reg->wages = wage1;
			}
		}
	}

	Awrite("");
}

void ARegionList::SeverLandBridges(ARegionArray *pRegs)
{
	Awrite("Severing land bridges");

	// mark land hexes to delete
	for (int i = 0; i < pRegs->x; ++i)
	{
		for (int j = 0; j < pRegs->y; ++j)
		{
			ARegion *reg = pRegs->GetRegion(i, j);
			if (!reg || TerrainDefs[reg->type].similar_type == R_OCEAN)
				continue;

			if (reg->IsCoastal() != 4)
				continue;

			int tidych = 30;
			for (int d = 0; d < NDIRS; ++d)
			{
				ARegion *newregion = reg->neighbors[d];
				if (!newregion || TerrainDefs[newregion->type].similar_type == R_OCEAN)
					continue;

				if (newregion->IsCoastal() == 4)
					tidych = tidych * 2;
			}

			if (getrandom(100) < tidych)
				reg->wages = -2;
		}
	}

	// now change to ocean
	for (int i = 0; i < pRegs->x; ++i)
	{
		for (int j = 0; j < pRegs->y; ++j)
		{
			ARegion *reg = pRegs->GetRegion(i, j);
			if (!reg || reg->wages > -2)
				continue;

			reg->type = R_OCEAN;
			reg->wages = -1;
			Adot();
		}
	}
	Awrite("");
}

void ARegionList::SetRegTypes(ARegionArray *pRegs, int newType)
{
	for (int i = 0; i < pRegs->x; ++i)
	{
		for (int j = 0; j < pRegs->y; ++j)
		{
			ARegion *reg = pRegs->GetRegion(i, j);

			if (reg && reg->type == -1)
				reg->type = newType;
		}
	}
}

void ARegionList::SetupAnchors(ARegionArray *ta)
{
	Awrite("Setting up the anchors");
	for (int x = 0; x < (ta->x)/4; ++x)
	{
		for (int y = 0; y < (ta->y)/8; ++y)
		{
			ARegion *reg = NULL;
			for (int i = 0; i < 4; ++i)
			{
				const int tempx = x * 4 + getrandom(4);
				const int tempy = y * 8 + getrandom(4)*2 + tempx%2;
				reg = ta->GetRegion(tempx, tempy);
				if (reg->type == R_NUM)
				{
					reg->type = GetRegType(reg);
					if (TerrainDefs[reg->type].similar_type != R_OCEAN)
						reg->wages = AGetName(0);
					break;
				}
			}
			Adot();
		}
	}

	Awrite("");
}

void ARegionList::GrowTerrain(ARegionArray *pArr, int growOcean)
{
	for (int j = 0; j < 10; ++j)
	{
		for (int x = 0; x < pArr->x; ++x)
		{
			for (int y = 0; y < pArr->y; ++y)
			{
				ARegion *reg = pArr->GetRegion(x, y);
				if (!reg)
					continue;

				if (reg->type == R_NUM)
				{
					// check for Lakes
					if (Globals->LAKES_EXIST && getrandom(100) < (Globals->LAKES_EXIST/10 + 1))
					{
						reg->type = R_LAKE;
						break;
					}

					// check for Odd Terrain
					if (getrandom(1000) < Globals->ODD_TERRAIN)
					{
						reg->type = GetRegType(reg);
						if (TerrainDefs[reg->type].similar_type != R_OCEAN)
							reg->wages = AGetName(0);

						break;
					}

					const int init = getrandom(6);
					for (int i = 0; i < NDIRS; ++i)
					{
						ARegion *t = reg->neighbors[(i+init) % NDIRS];
						if (!t)
							continue;

						if (t->type != R_NUM &&
						    (TerrainDefs[t->type].similar_type != R_OCEAN ||
						     (growOcean && t->type != R_LAKE)))
						{
							reg->race = t->type;
							reg->wages = t->wages;
							break;
						}
					}
				}
			}
		}

		for (int x = 0; x < pArr->x; ++x)
		{
			for (int y = 0; y < pArr->y; ++y)
			{
				ARegion *reg = pArr->GetRegion(x, y);

				if (reg && reg->type == R_NUM && reg->race != -1)
					reg->type = reg->race;
			}
		}
	}
}

void ARegionList::RandomTerrain(ARegionArray *pArr)
{
	for (int x = 0; x < pArr->x; ++x)
	{
		for (int y = 0; y < pArr->y; ++y)
		{
			ARegion *reg = pArr->GetRegion(x, y);
			if (!reg || reg->type != R_NUM)
				continue;

			int adjtype = 0;
			int adjname = -1;

			for (int d = 0; d < NDIRS; ++d)
			{
				ARegion *newregion = reg->neighbors[d];
				if (!newregion)
					continue;

				if (TerrainDefs[newregion->type].similar_type != R_OCEAN &&
				    newregion->type != R_NUM && newregion->wages > 0)
				{
					adjtype = newregion->type;
					adjname = newregion->wages;
				}
			}

			if (adjtype && !Globals->CONQUEST_GAME)
			{
				reg->type = adjtype;
				reg->wages = adjname;
			}
			else
			{
				reg->type = GetRegType(reg);
				reg->wages = AGetName(0);
			}
		}
	}
}

void ARegionList::MakeUWMaze(ARegionArray *pArr)
{
	for (int x = 0; x < pArr->x; ++x)
	{
		for (int y = 0; y < pArr->y; ++y)
		{
			ARegion *reg = pArr->GetRegion(x, y);
			if (!reg)
				continue;

			for (int i = D_SOUTHEAST; i <= D_SOUTHWEST; ++i)
			{
				int count = 0;
				for (int j = D_NORTH; j < NDIRS; ++j)
					if (reg->neighbors[j])
						++count;

				if (count <= 1)
					break;

				ARegion *n = reg->neighbors[i];
				if (!n || CheckRegionExit(reg, n))
					continue;

				count = 0;
				for (int k = D_NORTH; k < NDIRS; ++k)
				{
					if (n->neighbors[k])
						++count;
				}

				if (count <= 1)
					break;

				reg->neighbors[i] = 0;
				n->neighbors[(i+3) % NDIRS] = 0;
			}
		}
	}
}

void ARegionList::AssignTypes(ARegionArray *pArr)
{
	// RandomTerrain() will set all of the un-set region types and names
	RandomTerrain(pArr);
}

void ARegionList::FinalSetup(ARegionArray *pArr)
{
	for (int x = 0; x < pArr->x; ++x)
	{
		for (int y = 0; y < pArr->y; ++y)
		{
			ARegion *reg = pArr->GetRegion(x, y);
			if (!reg)
				continue;

			if (TerrainDefs[reg->type].similar_type == R_OCEAN &&
			    reg->type != R_LAKE)
			{
				if (pArr->levelType == ARegionArray::LEVEL_UNDERWORLD)
					reg->SetName("The Undersea");
				else if (pArr->levelType == ARegionArray::LEVEL_UNDERDEEP)
					reg->SetName("The Deep Undersea");
				else
				{
					AString ocean_name = Globals->WORLD_NAME;
					ocean_name += " Ocean";
					reg->SetName(ocean_name.Str());
				}
			}
			else
			{
				if (reg->wages == -1) reg->SetName("Unnamed");
				else if (reg->wages != -2)
					reg->SetName(AGetNameString(reg->wages));
				else
					reg->wages = -1;
			}

			reg->Setup(pArr);
		}
	}
}

void ARegionList::MakeShaft(ARegion *reg, ARegionArray *pFrom, ARegionArray *pTo)
{
	if (TerrainDefs[reg->type].similar_type == R_OCEAN)
		return;

	const int tempx = reg->xloc * pTo->x / pFrom->x + getrandom(pTo->x / pFrom->x);
	int tempy = reg->yloc * pTo->y / pFrom->y + getrandom(pTo->y / pFrom->y);

	// make sure we get a valid region
	tempy += (tempx + tempy) % 2;

	ARegion *temp = pTo->GetRegion(tempx, tempy);
	if (TerrainDefs[temp->type].similar_type == R_OCEAN)
		return;

	Object *o = new Object(reg);
	o->num = reg->buildingseq++;
	o->name = new AString(AString("Shaft [") + o->num + "]");
	o->type = O_SHAFT;
	o->incomplete = 0;
	o->inner = temp->num;
	reg->objects.Add(o);

	o = new Object(reg);
	o->num = temp->buildingseq++;
	o->name = new AString(AString("Shaft [") + o->num + "]");
	o->type = O_SHAFT;
	o->incomplete = 0;
	o->inner = reg->num;
	temp->objects.Add(o);
}

void ARegionList::MakeShaftLinks(int levelFrom, int levelTo, int odds)
{
	ARegionArray *pFrom = pRegionArrays[ levelFrom ];
	ARegionArray *pTo   = pRegionArrays[ levelTo ];

	for (int x = 0; x < pFrom->x; ++x)
	{
		for (int y = 0; y < pFrom->y; ++y)
		{
			ARegion *reg = pFrom->GetRegion(x, y);

			if (reg && getrandom(odds) == 0)
				MakeShaft(reg, pFrom, pTo);
		}
	}
}

void ARegionList::CalcDensities()
{
	Awrite("Densities:");

	int arr[R_NUM] = {0, };

	forlist(this)
	{
		ARegion *reg = (ARegion*)elem;
		arr[reg->type]++;
	}

	for (int i = 0; i < R_NUM; ++i)
		if (arr[i])
			Awrite(AString(TerrainDefs[i].name) + " " + arr[i]);

	Awrite("");
}

void ARegionList::SetACNeighbors(int levelSrc, int levelTo, int maxX, int maxY)
{
	ARegionArray *ar = GetRegionArray(levelSrc);

	for (int x = 0; x < ar->x; ++x)
	{
		for (int y = 0; y < ar->y; ++y)
		{
			ARegion *AC = ar->GetRegion(x, y);
			if (!AC)
				continue;

			for (int i = 0; i < NDIRS; ++i)
			{
				if (AC->neighbors[i])
					continue;

				ARegion *pReg = GetStartingCity(AC, i, levelTo, maxX, maxY);
				if (!pReg)
					continue;

				if (!Globals->NEXUS_NO_EXITS)
					AC->neighbors[i] = pReg;

				pReg->MakeStartingCity();
				if (Globals->GATES_EXIST)
				{
					numberofgates++;
				}
			}
		}
	}
}

void ARegionList::InitSetupGates(int level)
{
	if (!Globals->GATES_EXIST)
		return;

	ARegionArray *pArr = pRegionArrays[level];

	for (int i = 0; i < pArr->x / 8; ++i)
	{
		for (int j = 0; j < pArr->y / 16; ++j)
		{
			for (int k = 0; k < 5; ++k)
			{
				const int tempx = i*8 + getrandom(8);
				const int tempy = j*16 + getrandom(8)*2 + tempx%2;

				ARegion *temp = pArr->GetRegion(tempx, tempy);

				if (TerrainDefs[temp->type].similar_type != R_OCEAN && temp->gate != -1)
				{
					++numberofgates;
					temp->gate = -1;
					break;
				}
			}
		}
	}
}

void ARegionList::FinalSetupGates()
{
	if (!Globals->GATES_EXIST)
		return;

	bool *used = new bool[numberofgates];

	for (int i = 0; i < numberofgates; ++i)
		used[i] = false;

	forlist(this)
	{
		ARegion *r = (ARegion*)elem;
		if (r->gate != -1)
			continue;

		int index = getrandom(numberofgates);
		while (used[index])
		{
			index++;
			index = index % numberofgates;
		}

		r->gate = index+1;
		used[index] = true;
	}
	delete used;
}

ARegion* ARegionList::FindGate(int x)
{
	if (!x)
		return NULL;

	forlist(this)
	{
		ARegion *r = (ARegion*)elem;
		if (r->gate == x)
			return r;
	}
	return NULL;
}

int ARegionList::GetPlanarDistance(ARegion *one, ARegion *two, int penalty)
{
	if (one->zloc == ARegionArray::LEVEL_NEXUS ||
	    two->zloc == ARegionArray::LEVEL_NEXUS)
		return 10000000;

	if (Globals->ABYSS_LEVEL)
	{
		// make sure you cannot teleport into or from the abyss
		int ablevel = Globals->UNDERWORLD_LEVELS + Globals->UNDERDEEP_LEVELS + 2;
		if (one->zloc == ablevel || two->zloc == ablevel)
			return 10000000;
	}

	ARegionArray *pArr = pRegionArrays[ARegionArray::LEVEL_SURFACE];

	const int one_x = one->xloc * GetLevelXScale(one->zloc);
	const int one_y = one->yloc * GetLevelYScale(one->zloc);

	const int two_x = two->xloc * GetLevelXScale(two->zloc);
	const int two_y = two->yloc * GetLevelYScale(two->zloc);

	int maxy = one_y - two_y;
	if (maxy < 0) maxy = -maxy;

	int maxx = one_x - two_x;
	if (maxx < 0) maxx = -maxx;

	int max2 = one_x + pArr->x - two_x;
	if (max2 < 0) max2 = -max2;
	if (max2 < maxx) maxx = max2;

	max2 = one_x - (two_x + pArr->x);
	if (max2 < 0) max2 = -max2;
	if (max2 < maxx) maxx = max2;

	if (maxy > maxx) maxx = (maxx+maxy)/2;

	if (one->zloc != two->zloc)
	{
		int zdist = one->zloc - two->zloc;
		if ((two->zloc - one->zloc) > zdist)
			zdist = two->zloc - one->zloc;
		maxx += penalty * zdist;
	}

	return maxx;
}

int ARegionList::GetDistance(ARegion *one, ARegion *two)
{
	if (one->zloc != two->zloc)
		return 10000000;

	const ARegionArray *pArr = pRegionArrays[ one->zloc ];

	int maxy = one->yloc - two->yloc;
	if (maxy < 0) maxy = -maxy;

	int maxx = one->xloc - two->xloc;
	if (maxx < 0) maxx = -maxx;

	int max2 = one->xloc + pArr->x - two->xloc;
	if (max2 < 0) max2 = -max2;
	if (max2 < maxx) maxx = max2;

	max2 = one->xloc - (two->xloc + pArr->x);
	if (max2 < 0) max2 = -max2;
	if (max2 < maxx) maxx = max2;

	if (maxy > maxx)
		return (maxx + maxy) / 2;

	return maxx;
}

ARegionArray* ARegionList::GetRegionArray(int level)
{
	return pRegionArrays[level];
}

void ARegionList::CreateLevels(int n)
{
	numLevels = n;
	pRegionArrays = new ARegionArray*[n];
}

//----------------------------------------------------------------------------
ARegionArray::ARegionArray(int xx, int yy)
{
	x = xx;
	y = yy;
	strName = NULL;

	regions = new ARegion*[x * y / 2 + 1];
	for (int i = 0; i < x * y / 2; ++i)
		regions[i] = NULL;
}

ARegionArray::~ARegionArray()
{
	delete strName;
	delete[] regions;
}

void ARegionArray::SetRegion(int xx, int yy, ARegion *r)
{
	regions[ xx / 2 + yy * x / 2 ] = r;
}

ARegion* ARegionArray::GetRegion(int xx, int yy)
{
	xx = (xx + x) % x;
	yy = (yy + y) % y;

	if ((xx + yy) % 2)
		return NULL;

	return regions[ xx / 2 + yy * x / 2 ];
}

void ARegionArray::SetName(const char *name)
{
	if (name)
	{
		strName = new AString(name);
	}
	else
	{
		delete strName;
		strName = NULL;
	}
}

//----------------------------------------------------------------------------
ARegionFlatArray::ARegionFlatArray(int s)
{
	size = s;
	regions = new ARegion*[s];
}

ARegionFlatArray::~ARegionFlatArray()
{
	delete[] regions;
}

void ARegionFlatArray::SetRegion(int x, ARegion *r)
{
	regions[x] = r;
}

ARegion* ARegionFlatArray::GetRegion(int x)
{
	return regions[x];
}


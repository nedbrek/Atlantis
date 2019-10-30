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
#include "army.h"
#include "aregion.h"
#include "battle.h"
#include "faction.h"
#include "gameio.h"
#include "gamedata.h"
#include "gamedefs.h"
#include "object.h"
#include "unit.h"
#include <cstdlib>

//----------------------------------------------------------------------------
int pow(int b, int p)
{
	int b2 = b;
	for (int i = 1; i < p; ++i)
	{
		b2 *= b;
	}
	return b2;
}

int Hits(int a, int d)
{
	int tohit = 1, tomiss = 1;

	if (Globals->LINEAR_COMBAT) {
		tohit = a;
		tomiss = d;

		// make sure values are at least 1
		if (tohit < 1)
		{
			tomiss += abs(tohit) + 1;
			tohit = 1;
		}
		if (tomiss < 1)
		{
			tohit += abs(tomiss) + 1;
			tomiss = 1;
		}
	}
	// Default to power of 2 combat
	else {
		if (a > d)
		{
			tohit = pow(2, a - d);
		}
		else if (d > a)
		{
			tomiss = pow(2, d - a);
		}
	}

	return (getrandom(tohit+tomiss) < tohit) ? 1 : 0;
}

//----------------------------------------------------------------------------
enum
{
	WIN_NO_DEAD,
	WIN_DEAD,
	LOSS
};

Soldier::Soldier(Unit *u, Object *o, int regtype, int r, int ass)
{
	race = r;
	unit = u;
	building = 0;

	healing = 0;
	healtype = 0;
	healitem = -1;
	canbehealed = 1;
	regen = 0;

	armor = -1;
	riding = -1;
	weapon = -1;

	attacks = 1;
	attacktype = ATTACK_COMBAT;

	special = -1;
	slevel = 0;

	askill = 0;

	dskill[ATTACK_COMBAT]  = 0;
	dskill[ATTACK_ENERGY]  = -2;
	dskill[ATTACK_SPIRIT]  = -2;
	dskill[ATTACK_WEATHER] = -2;
	dskill[ATTACK_RIDING]  = 0;
	dskill[ATTACK_RANGED]  = 0;
	damage = 0;
	hits = 1;
	maxhits = 1;
	amuletofi = 0;
	battleItems = 0;

	effects = 0;

	// Building bonus
	if (o->capacity)
	{
		building = o->type;

		for (int i = 0; i < NUM_ATTACK_TYPES; ++i)
		{
			dskill[i] += 2;
		}

		if (o->runes)
		{
			dskill[ATTACK_ENERGY] = o->runes;
			dskill[ATTACK_SPIRIT] = o->runes;
		}

		o->capacity--;
	}

	if (ItemDefs[r].type & IT_MAN)
	{
		const int man = ItemDefs[r].index;
		hits = ManDefs[man].hits;
		if (hits < 1) hits = 1;
		maxhits = hits;
	}
	// Is this a monster?
	else if (ItemDefs[r].type & IT_MONSTER)
	{
		const int mon = ItemDefs[r].index;
		const MonType &mon_def = MonDefs[mon];

		if (u->type == U_WMON)
			name = AString(mon_def.name) + " in " + *(unit->name);
		else
			name = AString(mon_def.name) + AString(" controlled by ") + *(unit->name);

		askill = mon_def.attackLevel;

		dskill[ATTACK_COMBAT] += mon_def.defense[ATTACK_COMBAT];
		if (mon_def.defense[ATTACK_ENERGY] > dskill[ATTACK_ENERGY])
		{
			dskill[ATTACK_ENERGY] = mon_def.defense[ATTACK_ENERGY];
		}
		if (mon_def.defense[ATTACK_SPIRIT] > dskill[ATTACK_SPIRIT])
		{
			dskill[ATTACK_SPIRIT] = mon_def.defense[ATTACK_SPIRIT];
		}
		if (mon_def.defense[ATTACK_WEATHER] > dskill[ATTACK_WEATHER])
		{
			dskill[ATTACK_WEATHER] = mon_def.defense[ATTACK_WEATHER];
		}
		dskill[ATTACK_RIDING] += mon_def.defense[ATTACK_RIDING];
		dskill[ATTACK_RANGED] += mon_def.defense[ATTACK_RANGED];

		damage = 0;

		hits = mon_def.hits;
		if (hits < 1)
			hits = 1;

		maxhits = hits;

		attacks = mon_def.numAttacks;
		if (!attacks)
			attacks = 1;

		special = mon_def.special;
		slevel = mon_def.specialLevel;
		if (Globals->MONSTER_BATTLE_REGEN)
		{
			regen = mon_def.regen;
			if (regen < 0)
				regen = 0;
		}
		return;
	}

	name = *(unit->name);

	SetupHealing();

	SetupSpell();
	SetupCombatItems();

	// set up armor
	int i;
	for (i = 0; i < MAX_READY; ++i)
	{
		// check preferred armor first
		int item = unit->readyArmor[i];
		if (item == -1)
			break;

		const int armorType = ItemDefs[item].index;
		item = unit->GetArmor(armorType, ass);
		if (item != -1)
		{
			armor = item;
			break;
		}
	}

	// look for a default armor
	if (armor == -1)
	{
		for (int armorType = 1; armorType < NUMARMORS; ++armorType)
		{
			const int item = unit->GetArmor(armorType, ass);
			if (item != -1)
			{
				armor = item;
				break;
			}
		}
	}

	// check if this unit is mounted
	const int terrainflags = TerrainDefs[regtype].flags;
	const int canFly  = (terrainflags & TerrainType::FLYINGMOUNTS);
	const int canRide = (terrainflags & TerrainType::RIDINGMOUNTS);
	int ridingBonus = 0;
	if (canFly || canRide)
	{
		// mounts of some type _are_ allowed in this region
		for (int mountType = 1; mountType < NUMMOUNTS; ++mountType)
		{
			const int item = unit->GetMount(mountType, canFly, canRide, ridingBonus);
			if (item == -1)
				continue;

			// defer adding the combat bonus until we know if the weapon allows it
			riding = item;

			// defense bonus for riding can be added now however
			dskill[ATTACK_RIDING] += ridingBonus;
			break;
		}
	}

	// find the correct weapon for this soldier
	int attackBonus = 0;
	int defenseBonus = 0;
	int numAttacks = 1;

	//--- check the preferred weapon first
	for (i = 0; i < MAX_READY; ++i)
	{
		int item = unit->readyWeapon[i];
		if (item == -1)
			break;

		const int weaponType = ItemDefs[item].index;
		item = unit->GetWeapon(weaponType, riding, ridingBonus, attackBonus, defenseBonus, numAttacks);
		if (item != -1)
		{
			weapon = item;
			break;
		}
	}

	//--- equip a default weapon
	if (weapon == -1)
	{
		for (int weaponType = 1; weaponType < NUMWEAPONS; ++weaponType)
		{
			const int item = unit->GetWeapon(weaponType, riding, ridingBonus, attackBonus, defenseBonus, numAttacks);
			if (item != -1)
			{
				weapon = item;
				break;
			}
		}
	}

	// if we did not get a weapon
	if (weapon == -1)
	{
		// set attack and defense bonuses to
		// combat skill (and riding bonus if applicable)
		attackBonus = unit->GetSkill(S_COMBAT) + ridingBonus;
		defenseBonus = attackBonus;
		numAttacks = 1;
	}
	else // got a weapon
	{
		// if it has a special and don't have a special
		if ((ItemDefs[weapon].type & IT_BATTLE) && special == -1)
		{
			// use that special
			// (Weapons (like Runeswords) which are both weapons and battle items
			// will be skipped in the battle items setup and handled here)
			special = BattleItemDefs[ItemDefs[weapon].battleindex].index;
			slevel = BattleItemDefs[ItemDefs[weapon].battleindex].skillLevel;
		}
	}

	unit->Practise(S_COMBAT);
	if (ridingBonus)
		unit->Practise(S_RIDING);

	// set the attack and defense skills
	// (will include the riding bonus if they should be included)
	askill += attackBonus;
	dskill[ATTACK_COMBAT] += defenseBonus;
	attacks = numAttacks;
}

void Soldier::SetupSpell()
{
	if (unit->type != U_MAGE && unit->type != U_GUARDMAGE)
		return;

	// check for combat spell
	if (unit->combat == -1)
		return;

	slevel = unit->GetSkill(unit->combat);
	if (!slevel)
	{
		// can't cast this spell!
		unit->combat = -1;
		return;
	}

	SkillType *pST = &SkillDefs[ unit->combat ];
	if (!(pST->flags & SkillType::COMBAT))
	{
		// not a combat spell!
		unit->combat = -1;
		return;
	}

	special = pST->special;
	unit->Practise(unit->combat);
}

void Soldier::SetupCombatItems()
{
	for (int battleType = 1; battleType < NUMBATTLEITEMS; ++battleType)
	{
		const BattleItemType *const pBat = &BattleItemDefs[ battleType ];

		const int item = unit->GetBattleItem(battleType);
		if (item == -1)
			continue;

		// if:
		// prepare command disabled or
		// prepare optional and no prepared item or
		// this is the prepared item or
		// item is a shield (no prepare needed)
		if (!Globals->USE_PREPARE_COMMAND ||
		    (Globals->USE_PREPARE_COMMAND == GameDefs::PREPARE_NORMAL &&
		     unit->readyItem == -1) ||
		    pBat->itemNum == unit->readyItem ||
		    (pBat->flags & BattleItemType::SHIELD))
		{
			// if item is special and unit already has a special
			if ((pBat->flags & BattleItemType::SPECIAL) && special != -1)
			{
				// give item back as they aren't going to use it
				unit->items.SetNum(item, unit->items.GetNum(item)+1);
				continue;
			}

			// if only mages/apprentices can use this item
			if ((pBat->flags & BattleItemType::MAGEONLY) &&
			    unit->type != U_MAGE && unit->type != U_GUARDMAGE &&
			    unit->type != U_APPRENTICE)
			{
				// give item back as they aren't going to use it
				unit->items.SetNum(item, unit->items.GetNum(item)+1);
				continue;
			}

			// make sure amulets of invulnerability are marked
			if (item == I_AMULETOFI)
			{
				amuletofi = 1;
			}

			SET_BIT(battleItems, battleType);

			if (pBat->flags & BattleItemType::SPECIAL)
			{
				special = pBat->index;
				slevel = pBat->skillLevel;
			}

			if (pBat->flags & BattleItemType::SHIELD)
			{
				const SpecialType *const sp = &SpecialDefs[pBat->index];
				// if shield item with no shield FX
				if (!(sp->effectflags & SpecialType::FX_SHIELD))
				{
					continue;
				}

				for (int i = 0; i < 4; ++i)
				{
					if (sp->shield[i] == NUM_ATTACK_TYPES)
					{
						for (int j = 0; j < NUM_ATTACK_TYPES; ++j)
						{
							if (dskill[j] < pBat->skillLevel)
								dskill[j] = pBat->skillLevel;
						}
					}
					else if (sp->shield[i] >= 0)
					{
						if (dskill[sp->shield[i]] < pBat->skillLevel)
							dskill[sp->shield[i]] = pBat->skillLevel;
					}
				}
			}
		}
		else // using prepared items and this item is NOT the one we have prepared
		{
			// give it back to the unit as they won't use it
			unit->items.SetNum(item, unit->items.GetNum(item)+1);
		}
	}
}

int Soldier::HasEffect(int eff) const
{
	if (eff < 0)
		return 0;

	const int n = 1 << eff;
	return (effects & n);
}

void Soldier::SetEffect(int eff)
{
	if (eff < 0)
		return;

	const EffectType *const e = &EffectDefs[eff];
	askill += e->attackVal;

	for (int i = 0; i < 4; ++i)
	{
		if (e->defMods[i].type != -1)
		{
			dskill[e->defMods[i].type] += e->defMods[i].val;
		}
	}

	if (e->cancelEffect != -1)
	{
		ClearEffect(e->cancelEffect);
	}

	if (!(e->flags & EffectType::EFF_NOSET))
	{
		const int n = 1 << eff;

		effects |= n;
	}
}

void Soldier::ClearEffect(int eff)
{
	if (eff < 0)
		return;

	const EffectType *const e = &EffectDefs[eff];
	askill -= e->attackVal;

	for (int i = 0; i < 4; ++i)
	{
		if (e->defMods[i].type != -1)
		{
			dskill[e->defMods[i].type] -= e->defMods[i].val;
		}
	}

	const int n = 1 << eff;
	effects &= ~n;
}

void Soldier::ClearOneTimeEffects()
{
	for (int i = 0; i < NUMEFFECTS; ++i)
	{
		if (HasEffect(i) && (EffectDefs[i].flags & EffectType::EFF_ONESHOT))
			ClearEffect(i);
	}
}

int Soldier::ArmorProtect(int weaponClass) const
{
	int armorType = ARMOR_NONE;
	if (armor > 0)
		armorType = ItemDefs[armor].index;
	if (armorType < ARMOR_NONE || armorType >= NUMARMORS)
		armorType = ARMOR_NONE;

	const ArmorType *const pArm = &ArmorDefs[armorType];
	const int chance = pArm->saves[weaponClass];
	if (chance <= 0)
		return 0;

	return (chance > getrandom(pArm->from)) ? 1 : 0;
}

void Soldier::RestoreItems()
{
	// give back healing items
	if (healing && healitem != -1)
	{
		if (healitem == I_HERBS)
		{
			unit->items.SetNum(healitem,
			      unit->items.GetNum(healitem) + healing);
		}
		else if (healitem == I_HEALPOTION)
		{
			unit->items.SetNum(healitem,
			      unit->items.GetNum(healitem)+healing/10);
		}
	}

	if (weapon != -1)
		unit->items.SetNum(weapon, unit->items.GetNum(weapon) + 1);

	if (armor != -1)
		unit->items.SetNum(armor, unit->items.GetNum(armor) + 1);

	if (riding != -1)
		unit->items.SetNum(riding, unit->items.GetNum(riding) + 1);

	for (int battleType = 1; battleType < NUMBATTLEITEMS; ++battleType)
	{
		const BattleItemType *const pBat = &BattleItemDefs[ battleType ];

		if (GET_BIT(battleItems, battleType))
		{
			const int item = pBat->itemNum;
			unit->items.SetNum(item, unit->items.GetNum(item) + 1);
		}
	}
}

void Soldier::Alive(int state)
{
	RestoreItems();

	if (state == LOSS)
	{
		unit->canattack = 0;

		// guards with amuletofi will not go off guard
		if (!amuletofi &&
		    (unit->guard == GUARD_GUARD || unit->guard == GUARD_SET))
		{
			unit->guard = GUARD_NONE;
		}
	}
	else
	{
		unit->advancefrom = 0;
	}

	if (state == WIN_DEAD)
	{
		unit->canattack = 0;
		unit->nomove = 1;
	}
}

void Soldier::Dead()
{
	RestoreItems();

	unit->SetMen(race, unit->GetMen(race) - 1);
}

//----------------------------------------------------------------------------
Army::Army(Unit *ldr, AList *locs, int regtype, int ass)
{
	for (unsigned i = 0; i < 6; ++i)
	{
		kills_from[i] = 0;
		hits_from[i] = 0;
	}

	int tacspell = 0;
	Unit *tactitian = ldr;

	leader = ldr;
	round = 0;
	tac = ldr->GetSkill(S_TACTICS);
	count = 0;
	hitstotal = 0;

	if (ass)
	{
		count = 1;
		ldr->losses = 0;
	}
	else
	{
		forlist(locs)
		{
			Unit *u = ((Location*)elem)->unit;
			count += u->GetSoldiers();
			u->losses = 0;
			int temp = u->GetSkill(S_TACTICS);
			if (temp > tac)
			{
				tac = temp;
				tactitian = u;
			}
		}
	}
	tactitian->Practise(S_TACTICS);

	soldiers = new SoldierPtr[count];
	int x = 0;
	int y = count;

	forlist(locs)
	{
		Unit *u = ((Location*)elem)->unit;
		Object *obj = ((Location*)elem)->obj;
		if (ass)
		{
			forlist(&u->items)
			{
				Item *it = (Item*)elem;
				if (!it)
					continue;

				if (ItemDefs[ it->type ].type & IT_MAN)
				{
					soldiers[x] = new Soldier(u, obj, regtype, it->type, ass);
					hitstotal = soldiers[x]->hits;
					++x;
					goto finished_army; // done
				}
			}
		}
		else
		{
			Item *it = (Item*)u->items.First();
			do
			{
				if (IsSoldier(it->type))
				{
					for (int i = 0; i < it->num; ++i)
					{
						// if behind
						if ((ItemDefs[ it->type ].type & IT_MAN) &&
						    u->GetFlag(FLAG_BEHIND))
						{
							--y;
							soldiers[y] = new Soldier(u, obj, regtype, it->type);
							hitstotal += soldiers[y]->hits;
						}
						else
						{
							soldiers[x] = new Soldier(u, obj, regtype, it->type);
							hitstotal += soldiers[x]->hits;
							++x;
						}
					}
				}
				it = (Item*)u->items.Next(it);
			} while (it);
		}
	}

finished_army:
	tac = tac + tacspell;

	canfront = x;
	canbehind = count;
	notfront = count;
	notbehind = count;

	hitsalive = hitstotal;

	if (!NumFront())
	{
		canfront = canbehind;
		notfront = notbehind;
	}
}

Army::~Army()
{
	for (int i = 0; i < count; ++i)
	{
		delete soldiers[i];
		soldiers[i] = NULL;
	}
	delete[] soldiers;
}

void Army::Reset()
{
	canfront = notfront;
	canbehind = notbehind;
	notfront = notbehind;
}

void Army::endRound(Battle *b)
{
	bool had_losses = false;

	const char *attack_type_str[6] =
	{
		"melee",
		"cavalry",
		"ranged",
		"energy",
		"weather",
		"spirit"
	};
	for (unsigned i = 0; i < 6; ++i)
	{
		if (hits_from[i] || kills_from[i])
		{
			b->AddLine(*(leader->faction->name) + " takes " + hits_from[i] + " hits from " +
			    attack_type_str[i] + " attacks, resulting in " + kills_from[i] + " losses."
			);
			if (kills_from[i])
				had_losses = true;
			hits_from[i] = 0;
			kills_from[i] = 0;
		}
	}

	if (!had_losses)
		b->AddLine(*(leader->faction->name) + " loses 0.");
}

void Army::WriteLosses(Battle *b)
{
	const int num_lost = count - NumAlive();
	b->AddLine(*(leader->faction->name) + " loses " + num_lost + ".");

	if (num_lost == 0)
		return; // no losses

	AList damaged_units; // tmp unit list
	AList destroyed_units; // tmp unit list

	for (int i = notbehind; i < count; ++i)
	{
		Unit *u = soldiers[i]->unit;
		if (u->IsAlive())
		{
			if (!GetUnitList(&damaged_units, u))
			{
				UnitPtr *u = new UnitPtr;
				u->ptr = soldiers[i]->unit;
				damaged_units.Add(u);
			}
		}
		else
		{
			if (!GetUnitList(&destroyed_units, u))
			{
				UnitPtr *u = new UnitPtr;
				u->ptr = soldiers[i]->unit;
				destroyed_units.Add(u);
			}
		}
	}

	if (damaged_units.size() > 0)
	{
		b->AddLine("Damaged units:");
		forlist(&damaged_units)
		{
			UnitPtr *u = (UnitPtr*)elem;
			b->AddLine(AString("   ") + *u->ptr->name);
		}

		damaged_units.DeleteAll();
	}

	if (destroyed_units.size() > 0)
	{
		b->AddLine("Destroyed units:");
		forlist(&destroyed_units)
		{
			UnitPtr *u = (UnitPtr*)elem;
			b->AddLine(AString("   ") + *u->ptr->name);
		}

		destroyed_units.DeleteAll();
	}
}

void Army::GetMonSpoils(ItemList *spoils, int monitem, int free)
{
	if (Globals->MONSTER_NO_SPOILS > 0 &&
	    free >= Globals->MONSTER_SPOILS_RECOVERY)
	{
		// this monster is in it's period of absolutely no spoils
		return;
	}

	// first, silver
	int silv = MonDefs[ItemDefs[monitem].index].silver;
	if (Globals->MONSTER_NO_SPOILS > 0 && free > 0)
	{
		// adjust the spoils for length of freedom
		silv *= (Globals->MONSTER_SPOILS_RECOVERY-free);
		silv /= Globals->MONSTER_SPOILS_RECOVERY;
	}
	spoils->SetNum(I_SILVER, spoils->GetNum(I_SILVER) + getrandom(silv));

	int thespoil = MonDefs[ItemDefs[monitem].index].spoiltype;
	if (thespoil == -1)
		return;

	if (thespoil == IT_NORMAL && getrandom(2))
		thespoil = IT_TRADE;

	int count = 0;
	int i;
	for (i = 0; i < NITEMS; ++i)
	{
		if ( (ItemDefs[i].type & thespoil) &&
		    !(ItemDefs[i].type & IT_SPECIAL) &&
		    !(ItemDefs[i].flags & ItemType::DISABLED))
		{
			count++;
		}
	}

	count = getrandom(count) + 1;

	for (i = 0; i < NITEMS; ++i)
	{
		if ( (ItemDefs[i].type & thespoil) &&
		    !(ItemDefs[i].type & IT_SPECIAL) &&
		    !(ItemDefs[i].flags & ItemType::DISABLED))
		{
			count--;
			if (count == 0)
			{
				thespoil = i;
				break;
			}
		}
	}

	int val = getrandom(MonDefs[ItemDefs[monitem].index].silver * 2);
	if ((Globals->MONSTER_NO_SPOILS > 0) && (free > 0))
	{
		// adjust for length of monster freedom
		val *= (Globals->MONSTER_SPOILS_RECOVERY-free);
		val /= Globals->MONSTER_SPOILS_RECOVERY;
	}

	spoils->SetNum(thespoil, spoils->GetNum(thespoil) +
	      (val + getrandom(ItemDefs[thespoil].baseprice)) /
	      ItemDefs[thespoil].baseprice);
}

void Army::Regenerate(Battle *b)
{
	for (int i = 0; i < count; ++i)
	{
		Soldier *s = soldiers[i];
		// only front-line troops need to regenerate
		if (i >= notbehind)
			continue;

		const int diff = s->maxhits - s->hits;
		if (diff <= 0)
			continue; // no damage

		AString aName = s->name;

		if (s->damage != 0)
		{
			s->damage = 0;
		}

		if (s->regen)
		{
			const int regen = (s->regen > diff) ? diff : s->regen;

			s->hits += regen;
			b->AddLine(aName + " regenerates " + regen +
			      " hits bringing it to " + s->hits + "/" +
			      s->maxhits + ".");
		}
	}
}

void Army::Lose(Battle *b, ItemList *spoils)
{
	for (int i = 0; i < count; ++i)
	{
		Soldier *&s = soldiers[i];
		if (i < notbehind)
		{
			s->Alive(LOSS);
		}
		else
		{
			if (s->unit->type == U_WMON && (ItemDefs[s->race].type & IT_MONSTER))
				GetMonSpoils(spoils, s->race, s->unit->free);

			s->Dead();
		}
	}

	WriteLosses(b);
}

void Army::Tie(Battle *b)
{
	for (int x = 0; x < count; ++x)
	{
		Soldier *&s = soldiers[x];
		if (x < NumAlive())
		{
			s->Alive(WIN_DEAD);
		}
		else
		{
			s->Dead();
		}
	}

	WriteLosses(b);
}

int Army::CanBeHealed()
{
	for (int i = notbehind; i < count; ++i)
	{
		const Soldier *temp = soldiers[i];
		if (temp->canbehealed)
			return 1;
	}
	return 0;
}

void Army::DoHeal(Battle *b)
{
	// do magical healing
	for (int i = 5; i > 0; --i)
		DoHealLevel(b, i, 0);

	// do normal healing
	DoHealLevel(b, 1, 1);
}

void Army::DoHealLevel(Battle *b, int type, int useItems)
{
	int rate = HealDefs[type].rate;

	for (int i = 0; i < NumAlive(); ++i)
	{
		Soldier *s = soldiers[i];
		if (!CanBeHealed())
			break;

		if (s->healtype <= 0)
			continue;

		// use the appropriate healing
		if (s->healtype != type)
			continue;

		// if not healing
		if (!s->healing)
			continue;

		int n = 0;
		if (useItems)
		{
			if (s->healitem == -1)
				continue;

			if (s->healitem != I_HEALPOTION)
				s->unit->Practise(S_HEALING);
		}
		else
		{
			if (s->healitem != -1)
				continue;

			s->unit->Practise(S_MAGICAL_HEALING);
		}

		while (s->healing)
		{
			if (!CanBeHealed())
				break;

			const int j = getrandom(count - NumAlive()) + notbehind;
			Soldier *temp = soldiers[j];
			if (temp->canbehealed)
			{
				s->healing--;
				if (getrandom(100) < rate)
				{
					n++;
					soldiers[j] = soldiers[notbehind];
					soldiers[notbehind] = temp;
					notbehind++;
				}
				else
					temp->canbehealed = 0;
			}
		}
		b->AddLine(*(s->unit->name) + " heals " + n + ".");
	}
}

void Army::Win(Battle *b, ItemList *spoils)
{
	DoHeal(b);

	// check for casualties
	const int na = NumAlive();

	const int wintype = (count - na) ? WIN_DEAD : WIN_NO_DEAD;

	// return soldiers to units
	for (int x = 0; x < count; ++x)
	{
		Soldier *&s = soldiers[x];
		if (x < NumAlive())
			s->Alive(wintype);
		else
			s->Dead();
	}

	WriteLosses(b);

	if (na == 0)
		return; // not sure how you win with 0 alive

	// divide spoils
	// TODO: special case I_SILV?
	forlist(spoils)
	{
		const Item *i = (Item*)elem;
		if (!i)
			continue;

		int num_items = i->num;

		bool done = false;
		int offset = 0;
		while (!done)
		{
			done = true; // last time through the loop
			int x = 0;
			for (; num_items && x < na; ++x)
			{
				// give it to next soldier in line
				Unit *u = soldiers[(x + offset) % na]->unit;

				// check for spoils flags
				if (!u->CanGetSpoil(i))
					continue; // try someone else

				// ok add one
				u->items.SetNum(i->type, u->items.GetNum(i->type)+1);
				u->faction->DiscoverItem(i->type, 0, 1);
				--num_items;
				done = false; // keep going
			}

			// adjust the next soldier to get spoils
			offset = x == na ? 0 : x;
		}
	}
}

int Army::Broken() const
{
	if (Globals->ARMY_ROUT == GameDefs::ARMY_ROUT_FIGURES)
	{
		if ((NumAlive() << 1) < count)
			return 1;
	}
	else
	{
		if ((hitsalive << 1) < hitstotal)
			return 1;
	}

	return 0;
}

int Army::NumSpoilers() const
{
	const int na = NumAlive();
	int count = 0;
	for (int x = 0; x < na; ++x)
	{
		Unit *u = soldiers[x]->unit;
		if (!(u->flags & FLAG_NOSPOILS))
			count++;
	}
	return count;
}

int Army::NumAlive() const
{
	return notbehind;
}

int Army::CanAttack() const
{
	return canbehind;
}

int Army::NumFront() const
{
	return (canfront + notfront - canbehind);
}

Soldier* Army::GetAttacker(int i, int &behind)
{
	Soldier *retval = soldiers[i];
	if (i < canfront)
	{
		soldiers[i] = soldiers[canfront-1];
		soldiers[canfront-1] = soldiers[canbehind-1];
		soldiers[canbehind-1] = retval;
		canfront--;
		canbehind--;
		behind = 0;
		return retval;
	}

	soldiers[i] = soldiers[canbehind-1];
	soldiers[canbehind-1] = soldiers[notfront-1];
	soldiers[notfront-1] = retval;
	canbehind--;
	notfront--;
	behind = 1;
	return retval;
}

int Army::GetTargetNum(int special)
{
	int tars = NumFront();
	// if front rank depleted
	if (tars == 0)
	{
		// make behind units in front
		canfront = canbehind;
		notfront = notbehind;
		tars = NumFront();
		if (tars == 0)
			return -1;
	}

	if (!SpecialDefs[special].targflags)
	{
		const int i = getrandom(tars);
		if (i < canfront)
			return i;

		return i + canbehind - canfront;
	}

	int validtargs = 0;
	int i, start = -1;

	for (i = 0; i < canfront; ++i)
	{
		if (CheckSpecialTarget(special, i))
		{
			validtargs++;
			// slight scan optimisation - skip empty initial sequences
			if (start == -1)
				start = i;
		}
	}

	for (i = canbehind; i < notfront; ++i)
	{
		if (CheckSpecialTarget(special, i))
		{
			validtargs++;
			// slight scan optimisation - skip empty initial sequences
			if (start == -1)
				start = i;
		}
	}

	if (validtargs)
	{
		int targ = getrandom(validtargs);

		for (i = start; i < notfront; ++i)
		{
			if (i == canfront)
				i = canbehind;

			if (CheckSpecialTarget(special, i))
			{
				if (!targ--)
					return i;
			}
		}
	}

	return -1;
}

int Army::GetEffectNum(int effect)
{
	int validtargs = 0;
	int start = -1;

	for (int i = 0; i < canfront; ++i)
	{
		if (soldiers[i]->HasEffect(effect))
		{
			validtargs++;
			// slight scan optimisation - skip empty initial sequences
			if (start == -1)
				start = i;
		}
	}

	for (int i = canbehind; i < notfront; ++i)
	{
		if (soldiers[i]->HasEffect(effect))
		{
			validtargs++;
			// slight scan optimisation - skip empty initial sequences
			if (start == -1)
				start = i;
		}
	}

	if (validtargs)
	{
		int targ = getrandom(validtargs);
		for (int i = start; i < notfront; ++i)
		{
			if (i == canfront)
				i = canbehind;

			if (soldiers[i]->HasEffect(effect))
			{
				if (!targ--)
					return i;
			}
		}
	}
	return -1;
}

Soldier* Army::GetTarget(int i)
{
	return soldiers[i];
}

int Army::RemoveEffects(int num, int effect)
{
	int ret = 0;
	for (int i = 0; i < num; ++i)
	{
		// try to find a target unit
		const int tarnum = GetEffectNum(effect);
		if (tarnum == -1)
			continue;

		// remove the effect
		Soldier *tar = GetTarget(tarnum);
		tar->ClearEffect(effect);

		++ret;
	}

	return ret;
}

int Army::DoAnAttack(int special, int numAttacks, int attackType,
      int attackLevel, int flags, int weaponClass, int effect,
      int mountBonus, int *num_killed, bool riding)
{
	// 1. check against Global effects (not sure how yet)
	// 2. attack shield
	int combat = 0;
	int canShield = 0;
	switch (attackType)
	{
		case ATTACK_RANGED:
			canShield = 1;
			// fall through
		case ATTACK_COMBAT:
		case ATTACK_RIDING:
			combat = 1;
			break;

		case ATTACK_ENERGY:
		case ATTACK_WEATHER:
		case ATTACK_SPIRIT:
			canShield = 1;
			break;
	}
	int attack_bin = 0;
	if (attackType == ATTACK_COMBAT)
	{
		if (riding)
			attack_bin = 1;
		else
			attack_bin = 0;
	}
	else if (attackType == ATTACK_RIDING)
		attack_bin = 1;
	else if (attackType == ATTACK_RANGED)
		attack_bin = 2;
	else if (attackType == ATTACK_ENERGY)
		attack_bin = 3;
	else if (attackType == ATTACK_WEATHER)
		attack_bin = 4;
	else if (attackType == ATTACK_SPIRIT)
		attack_bin = 5;

	if (canShield)
	{
		Shield *hi = shields.GetHighShield(attackType);
		if (hi)
		{
			// if we can't get through shield
			if (!Hits(attackLevel, hi->shieldskill))
			{
				return -1;
			}

			if (!effect && !combat)
			{
				// we got through shield... if killing spell, destroy shield
				shields.Remove(hi);
				delete hi;
			}
		}
	}

	// now, loop through and do attacks
	int ret = 0;
	for (int i = 0; i < numAttacks; ++i)
	{
		// 3. get the target
		const int tarnum = GetTargetNum(special);
		if (tarnum == -1)
			continue;

		Soldier *tar = GetTarget(tarnum);
		int tarFlags = 0;
		if (tar->weapon != -1)
		{
			tarFlags = WeaponDefs[ItemDefs[tar->weapon].index].flags;
		}

		// 4. add in any effects, if applicable
		int tlev = 0;
		if (attackType != NUM_ATTACK_TYPES)
			tlev = tar->dskill[ attackType ];

		if (special > 0)
		{
			if ((SpecialDefs[special].effectflags & SpecialType::FX_NOBUILDING) &&
			    tar->building)
			{
				tlev -= 2;
			}
		}

		// 4.1 check whether defense is allowed against this weapon
		if ((flags & WeaponType::NODEFENSE) && tlev > 0)
			tlev = 0;

		if (!(flags & WeaponType::RANGED))
		{
			// 4.2 check relative weapon length
			int attLen = 1; // start at NORMAL

			if (flags & WeaponType::LONG)
				attLen = 2;
			else if (flags & WeaponType::SHORT)
				attLen = 0;

			int defLen = 1;
			if (tarFlags & WeaponType::LONG)
				defLen = 2;
			else if (tarFlags & WeaponType::SHORT)
				defLen = 0;

			if (attLen > defLen)
				attackLevel++;
			else if (defLen > attLen)
				tlev++;
		}

		// 4.3 add bonuses versus mounted
		if (tar->riding != -1)
			attackLevel += mountBonus;

		// 5. attack soldier
		if (attackType != NUM_ATTACK_TYPES)
		{
			// see if weapon is ready
			if (!(flags & WeaponType::ALWAYSREADY) && getrandom(2))
			{
				continue; // no
			}

			// check for hit
			if (!Hits(attackLevel, tlev))
				continue; // miss
		}

		// 6. if attack got through, apply effect, or kill
		if (!effect)
		{
			// 7. last chance... Check armor
			if (tar->ArmorProtect(weaponClass))
				continue;

			// 8. seeya!
			const bool died = DamageSoldier(tarnum);
			++hits_from[attack_bin];
			if (died)
			{
				++*num_killed;
				++kills_from[attack_bin];
			}

			++ret;
		}
		else
		{
			if (tar->HasEffect(effect))
				continue;

			tar->SetEffect(effect);
			++ret;
		}
	}

	return ret;
}

bool Army::DamageSoldier(int killed)
{
	Soldier *const temp = soldiers[killed];
	if (temp->amuletofi)
		return false;

	if (Globals->ARMY_ROUT == GameDefs::ARMY_ROUT_HITS_INDIVIDUAL)
		--hitsalive;

	temp->damage++;
	temp->hits--;

	// if soldier can take multiple hits
	if (temp->hits > 0)
		return false; // done

	// notify unit of loss
	temp->unit->losses++;

	if (Globals->ARMY_ROUT == GameDefs::ARMY_ROUT_HITS_FIGURE)
	{
		if (ItemDefs[temp->race].type & IT_MONSTER)
		{
			hitsalive -= MonDefs[ItemDefs[temp->race].index].hits;
		}
		else
		{
			// assume everything that isn't a monster is a man
			const int man = ItemDefs[temp->race].index;
			hitsalive -= ManDefs[man].hits;
		}
	}

	if (killed < canfront)
	{
		soldiers[killed] = soldiers[canfront-1];
		soldiers[canfront-1] = temp;
		killed = canfront - 1;
		canfront--;
	}

	if (killed < canbehind)
	{
		soldiers[killed] = soldiers[canbehind-1];
		soldiers[canbehind-1] = temp;
		killed = canbehind-1;
		canbehind--;
	}

	if (killed < notfront)
	{
		soldiers[killed] = soldiers[notfront-1];
		soldiers[notfront-1] = temp;
		killed = notfront-1;
		notfront--;
	}

	soldiers[killed] = soldiers[notbehind-1];
	soldiers[notbehind-1] = temp;
	notbehind--;
	return true;
}


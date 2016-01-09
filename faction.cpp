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
// Date        Person          Comments
// ----        ------          --------
// 2000/MAR/14 Larry Stanbery  Modified the elimiation message.
// 2000/MAR/14 Davis Kulis     Added a new reporting Template.
// 2001/Feb/18 Joseph Traub    Added Apprentices concept from Lacandon Conquest
// 2001/Feb/21 Joseph Traub    Added a FACLIM_UNLIMITED option
// 2001/Feb/22 Joseph Traub    Modified to always save out faction type values
//                             even in scenarios where they aren't used or
//                             else your city guard and monster facs lose
//                             their NPC status in UNLIMITED
//
#include "gamedata.h"
#include "game.h"
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

int ParseAttitude(AString *token)
{
	for (int i=0; i<NATTITUDES; i++)
		if (*token == AttitudeStrs[i]) return i;
	return -1;
}

static AString MonResist(int type, int val, int full)
{
	AString temp = "This monster ";
	if(full) {
		temp += AString("has a resistance of ") + val;
	} else {
		temp += "is ";
		if(val < 1) temp += "very susceptible";
		else if(val == 1) temp += "susceptible";
		else if(val > 1 && val < 3) temp += "typically resistant";
		else if(val > 2 && val < 5) temp += "slightly resistant";
		else temp += "very resistant";
	}
	temp += " to ";
	temp += AttType(type);
	temp += " attacks.";
    return temp;
}

static AString WeapClass(int wclass)
{
	switch(wclass) {
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

static AString WeapType(int flags, int wclass)
{
	AString type;
	if(flags & WeaponType::RANGED) type = "ranged";
	if(flags & WeaponType::LONG) type = "long";
	if(flags & WeaponType::SHORT) type = "short";
	type += " ";
	type += WeapClass(wclass);
	return type;
}

AString* ItemDescription(int item, int full)
{
	int i;

	if(ItemDefs[item].flags & ItemType::DISABLED)
		return NULL;

	AString *temp = new AString;
	int illusion = ((ItemDefs[item].type & IT_MONSTER) &&
			(ItemDefs[item].index == MONSTER_ILLUSION));

	*temp += AString(illusion?"illusory ":"")+ ItemDefs[item].name + " [" +
		(illusion?"I":"") + ItemDefs[item].abr + "], weight " +
		ItemDefs[item].weight;

	if (ItemDefs[item].walk) {
		int cap = ItemDefs[item].walk - ItemDefs[item].weight;
		if(cap) {
			*temp += AString(", walking capacity ") + cap;
		} else {
			*temp += ", can walk";
		}
	}
	if((ItemDefs[item].hitchItem != -1 )&&
			!(ItemDefs[ItemDefs[item].hitchItem].flags & ItemType::DISABLED)) {
		int cap = ItemDefs[item].walk - ItemDefs[item].weight +
			ItemDefs[item].hitchwalk;
		if(cap) {
			*temp += AString(", walking capacity ") + cap +
				" when hitched to a " +
				ItemDefs[ItemDefs[item].hitchItem].name;
		}
	}
	if (ItemDefs[item].ride) {
		int cap = ItemDefs[item].ride - ItemDefs[item].weight;
		if(cap) {
			*temp += AString(", riding capacity ") + cap;
		} else {
			*temp += ", can ride";
		}
	}
	if (ItemDefs[item].swim) {
		int cap = ItemDefs[item].swim - ItemDefs[item].weight;
		if(cap) {
			*temp += AString(", swimming capacity ") + cap;
		} else {
			*temp += ", can swim";
		}
	}
	if (ItemDefs[item].fly) {
		int cap = ItemDefs[item].fly - ItemDefs[item].weight;
		if(cap) {
			*temp += AString(", flying capacity ") + cap;
		} else {
			*temp += ", can fly";
		}
	}

	if(Globals->ALLOW_WITHDRAW) {
		if(ItemDefs[item].type & IT_NORMAL && item != I_SILVER) {
			*temp += AString(", costs ") + (ItemDefs[item].baseprice*5/2) +
				" silver to withdraw";
		}
	}
	*temp += ".";

	if(ItemDefs[item].type & IT_MAN) {
		int man = ItemDefs[item].index;
		int found = 0;
		*temp += " This race may study ";
		unsigned int c;
		unsigned int len = sizeof(ManDefs[man].skills) /
							sizeof(ManDefs[man].skills[0]);
		for(c = 0; c < len; c++) {
			int skill = ManDefs[man].skills[c];
			if(skill != -1) {
				if(SkillDefs[skill].flags & SkillType::DISABLED) continue;
				if(found) *temp += ", ";
				if(found && c == len - 1) *temp += "and ";
				found = 1;
				*temp += SkillStrs(ManDefs[man].skills[c]);
			}
		}
		if(found) {
			*temp += AString(" to level ") + ManDefs[man].speciallevel +
				" and all others to level " + ManDefs[man].defaultlevel;
		} else {
			*temp += AString("all skills to level ") +
				ManDefs[man].defaultlevel;
		}
	}
	if(ItemDefs[item].type & IT_MONSTER) {
		*temp += " This is a monster.";
		int mon = ItemDefs[item].index;
		*temp += AString(" This monster attacks with a combat skill of ") +
			MonDefs[mon].attackLevel + ".";
		for(int c = 0; c < NUM_ATTACK_TYPES; c++) {
			*temp += AString(" ") + MonResist(c,MonDefs[mon].defense[c], full);
		}
		if(MonDefs[mon].special && MonDefs[mon].special != -1) {
			*temp += AString(" ") +
				"Monster can cast " +
				ShowSpecial(MonDefs[mon].special, MonDefs[mon].specialLevel,
						1, 0);
		}
		if(full) {
			int hits = MonDefs[mon].hits;
			int atts = MonDefs[mon].numAttacks;
			int regen = MonDefs[mon].regen;
			if(!hits) hits = 1;
			if(!atts) atts = 1;
			*temp += AString(" This monster has ") + atts + " melee " +
				((atts > 1)?"attacks":"attack") + " per round and takes " +
				hits + " " + ((hits > 1)?"hits":"hit") + " to kill.";
			if (regen > 0) {
				*temp += AString(" This monsters regenerates ") + regen +
					" hits per round of battle.";
			}
			*temp += AString(" This monster has a tactics score of ") +
				MonDefs[mon].tactics + ", a stealth score of " +
				MonDefs[mon].stealth + ", and an observation score of " +
				MonDefs[mon].obs + ".";
		}
		*temp += " This monster might have ";
		if(MonDefs[mon].spoiltype != -1) {
			if(MonDefs[mon].spoiltype & IT_MAGIC) {
				*temp += "magic items and ";
			} else if(MonDefs[mon].spoiltype & IT_ADVANCED) {
				*temp += "advanced items and ";
			} else if(MonDefs[mon].spoiltype & IT_NORMAL) {
				*temp += "normal or trade items and ";
			}
		}
		*temp += "silver as treasure.";
	}

	if(ItemDefs[item].type & IT_WEAPON) {
		int wep = ItemDefs[item].index;
		WeaponType *pW = &WeaponDefs[wep];
		*temp += " This is a ";
		*temp += WeapType(pW->flags, pW->weapClass) + " weapon.";
		if(pW->flags & WeaponType::NEEDSKILL) {
			*temp += AString(" Knowledge of ") + SkillStrs(pW->baseSkill);
			if(pW->orSkill != -1)
				*temp += AString(" or ") + SkillStrs(pW->orSkill);
			*temp += " is needed to wield this weapon.";
		} else {
			if(pW->baseSkill == -1 && pW->orSkill == -1)
				*temp += " No skill is needed to wield this weapon.";
		}

		int flag = 0;
		if(pW->attackBonus != 0) {
			*temp += " This weapon grants a ";
			*temp += ((pW->attackBonus > 0) ? "bonus of " : "penalty of ");
			*temp += abs(pW->attackBonus);
			*temp += " on attack";
			flag = 1;
		}
		if(pW->defenseBonus != 0) {
			if(flag) {
				if(pW->attackBonus == pW->defenseBonus) {
					*temp += " and defense.";
					flag = 0;
				} else {
					*temp += " and a ";
				}
			} else {
				*temp += " This weapon grants a ";
				flag = 1;
			}
			if(flag) {
				*temp += ((pW->defenseBonus > 0)?"bonus of ":"penalty of ");
				*temp += abs(pW->defenseBonus);
				*temp += " on defense.";
				flag = 0;
			}
		}
		if(flag) *temp += ".";
		if(pW->mountBonus && full) {
			*temp += " This weapon ";
			if(pW->attackBonus != 0 || pW->defenseBonus != 0)
				*temp += "also ";
			*temp += "grants a ";
			*temp += ((pW->mountBonus > 0)?"bonus of ":"penalty of ");
			*temp += abs(pW->mountBonus);
			*temp += " against mounted opponents.";
		}

		if(pW->flags & WeaponType::NOFOOT)
			*temp += " Only mounted troops may use this weapon.";
		else if(pW->flags & WeaponType::NOMOUNT)
			*temp += " Only foot troops may use this weapon.";

		if(pW->flags & WeaponType::RIDINGBONUS) {
			*temp += " Wielders of this weapon, if mounted, get their riding "
				"skill bonus on combat attack and defense.";
		} else if(pW->flags & WeaponType::RIDINGBONUSDEFENSE) {
			*temp += " Wielders of this weapon, if mounted, get their riding "
				"skill bonus on combat defense.";
		}

		if(pW->flags & WeaponType::NODEFENSE) {
			*temp += " Defenders are treated as if they have an "
				"effective combat skill of 0.";
		}

		if(pW->flags & WeaponType::NOATTACKERSKILL) {
			*temp += " Attackers do not get skill bonus on defense.";
		}

		if(pW->flags & WeaponType::ALWAYSREADY) {
			*temp += " Wielders of this weapon never miss a round to ready "
				"their weapon.";
		} else {
			*temp += " There is a 50% chance that the wielder of this weapon "
				"gets a chance to attack in any given round.";
		}

		if(full) {
			int atts = pW->numAttacks;
			*temp += AString(" This weapon attacks versus the target's ") +
				"defense against " + AttType(pW->attackType) + " attacks.";
			*temp += AString(" This weapon allows ");
			if(atts > 0) {
				if(atts >= WeaponType::NUM_ATTACKS_HALF_SKILL) {
					int max = WeaponType::NUM_ATTACKS_HALF_SKILL;
					const char *attd = "half the skill level (rounded up)";
					if(atts >= WeaponType::NUM_ATTACKS_SKILL) {
						max = WeaponType::NUM_ATTACKS_SKILL;
						attd = "the skill level";
					}
					*temp += "a number of attacks equal to ";
					*temp += attd;
					*temp += " of the attacker";
					int val = atts - max;
					if(val > 0) *temp += AString(" plus ") + val;
				} else {
					*temp += AString(atts) + ((atts==1)?" attack ":" attacks");
				}
				*temp += " per round.";
			} else {
				atts = -atts;
				*temp += "1 attack every ";
				if(atts == 1) *temp += "round .";
				else *temp += AString(atts) + " rounds.";
			}
		}
	}

	if(ItemDefs[item].type & IT_ARMOR) {
		*temp += " This is a type of armor.";
		int arm = ItemDefs[item].index;
		ArmorType *pA = &ArmorDefs[arm];
		*temp += " This armor will protect its wearer ";
		for(i = 0; i < NUM_WEAPON_CLASSES; i++) {
			if(i == NUM_WEAPON_CLASSES - 1) {
				*temp += ", and ";
			} else if(i > 0) {
				*temp += ", ";
			}
			int percent = (int)(((float)pA->saves[i]*100.0) /
					(float)pA->from+0.5);
			*temp += AString(percent) + "% of the time versus " +
				WeapClass(i) + " attacks";
		}
		*temp += ".";
		if(full) {
			if(pA->flags & ArmorType::USEINASSASSINATE) {
				*temp += " This armor may be worn during assassination "
					"attempts.";
			}
		}
	}

	if(ItemDefs[item].type & IT_TOOL) {
		int comma = 0;
		int last = -1;
		*temp += " This is a tool.";
		*temp += " This item increases the production of ";
		for(i = NITEMS - 1; i > 0; i--) {
			if(ItemDefs[i].flags & ItemType::DISABLED) continue;
			if(ItemDefs[i].mult_item == item) {
				last = i;
				break;
			}
		}
		for(i = 0; i < NITEMS; i++) {
		   if(ItemDefs[i].flags & ItemType::DISABLED) continue;
		   if(ItemDefs[i].mult_item == item) {
			   if(comma) {
				   if(last == i) {
					   if(comma > 1) *temp += ",";
					   *temp += " and ";
				   } else {
					   *temp += ", ";
				   }
			   }
			   comma++;
			   if(i == I_SILVER) {
				   *temp += "entertainment";
			   } else {
				   *temp += ItemDefs[i].names;
			   }
			   *temp += AString(" by ") + ItemDefs[i].mult_val;
		   }
		}
		*temp += ".";
	}

	if(ItemDefs[item].type & IT_TRADE) {
		*temp += " This is a trade good.";
		if(full) {
			if(Globals->RANDOM_ECONOMY) {
				int maxbuy, minbuy, maxsell, minsell;
				if(Globals->MORE_PROFITABLE_TRADE_GOODS) {
					minsell = (ItemDefs[item].baseprice*250)/100;
					maxsell = (ItemDefs[item].baseprice*350)/100;
					minbuy = (ItemDefs[item].baseprice*100)/100;
					maxbuy = (ItemDefs[item].baseprice*190)/100;
				} else {
					minsell = (ItemDefs[item].baseprice*150)/100;
					maxsell = (ItemDefs[item].baseprice*200)/100;
					minbuy = (ItemDefs[item].baseprice*100)/100;
					maxbuy = (ItemDefs[item].baseprice*150)/100;
				}
				*temp += AString(" This item can be bought for between ") +
					minbuy + " and " + maxbuy + " silver.";
				*temp += AString(" This item can be sold for between ") +
					minsell+ " and " + maxsell+ " silver.";
			} else {
				*temp += AString(" This item can be bought and sold for ") +
					ItemDefs[item].baseprice + " silver.";
			}
		}
	}

	if(ItemDefs[item].type & IT_MOUNT) {
		*temp += " This is a mount.";
		int mnt = ItemDefs[item].index;
		MountType *pM = &MountDefs[mnt];
		if(pM->skill == -1) {
			*temp += " No skill is required to use this mount.";
		} else if(SkillDefs[pM->skill].flags & SkillType::DISABLED) {
			*temp += " This mount is unridable.";
		} else {
			*temp += AString(" This mount requires ") + SkillStrs(pM->skill) +
				" of at least level " + pM->minBonus + " to ride in combat.";
		}
		*temp += AString(" This mount gives a minimum bonus of +") +
			pM->minBonus + " when ridden into combat.";
		*temp += AString(" This mount gives a maximum bonus of +") +
			pM->maxBonus + " when ridden into combat.";
		if(full) {
			if(ItemDefs[item].fly) {
				*temp += AString(" This mount gives a maximum bonus of +") +
					pM->maxHamperedBonus + " when ridden into combat in " +
					"terrain which allows ridden mounts but not flying "+
					"mounts.";
			}
			if(pM->mountSpecial != -1) {
				*temp += AString(" When ridden, this mount causes ") +
					ShowSpecial(pM->mountSpecial, pM->specialLev, 1, 0);
			}
		}
	}

	if(ItemDefs[item].pSkill != -1 &&
			!(SkillDefs[ItemDefs[item].pSkill].flags & SkillType::DISABLED)) {
		unsigned int c;
		unsigned int len;
		*temp += AString(" Units with ") + SkillStrs(ItemDefs[item].pSkill) +
			" " + ItemDefs[item].pLevel + " may PRODUCE ";
		if (ItemDefs[item].flags & ItemType::SKILLOUT)
			*temp += "a number of this item equal to their skill level";
		else
			*temp += "this item";
		len = sizeof(ItemDefs[item].pInput)/sizeof(Materials);
		int count = 0;
		int tot = len;
		for(c = 0; c < len; c++) {
			int itm = ItemDefs[item].pInput[c].item;
			int amt = ItemDefs[item].pInput[c].amt;
			if(itm == -1 || ItemDefs[itm].flags & ItemType::DISABLED) {
				tot--;
				continue;
			}
			if(count == 0) {
				*temp += " from ";
				if (ItemDefs[item].flags & ItemType::ORINPUTS)
					*temp += "any of ";
			} else if (count == tot) {
				if(c > 1) *temp += ",";
				*temp += " and ";
			} else {
				*temp += ", ";
			}
			count++;
			*temp += AString(amt) + " " + ItemDefs[itm].names;
		}
		if(ItemDefs[item].pOut) {
			*temp += AString(" at a rate of ") + ItemDefs[item].pOut;
			if(ItemDefs[item].pMonths) {
				if(ItemDefs[item].pMonths == 1) {
					*temp += " per man-month.";
				} else {
					*temp += AString(" per ") + ItemDefs[item].pMonths +
						" man-months.";
				}
			}
		}
	}
	if(ItemDefs[item].mSkill != -1 &&
			!(SkillDefs[ItemDefs[item].mSkill].flags & SkillType::DISABLED)) {
		unsigned int c;
		unsigned int len;
		*temp += AString(" Units with ") + SkillStrs(ItemDefs[item].mSkill) +
			" of at least level " + ItemDefs[item].mLevel +
			" may attempt to create this item via magic";
		len = sizeof(ItemDefs[item].mInput)/sizeof(Materials);
		int count = 0;
		int tot = len;
		for(c = 0; c < len; c++) {
			int itm = ItemDefs[item].mInput[c].item;
			int amt = ItemDefs[item].mInput[c].amt;
			if(itm == -1 || ItemDefs[itm].flags & ItemType::DISABLED) {
				tot--;
				continue;
			}
			if(count == 0) {
				*temp += " at a cost of ";
			} else if (count == tot) {
				if(c > 1) *temp += ",";
				*temp += " and ";
			} else {
				*temp += ", ";
			}
			count++;
			*temp += AString(amt) + " " + ItemDefs[itm].names;
		}
		*temp += ".";
	}

	if((ItemDefs[item].type & IT_BATTLE) && full) {
		*temp += " This item is a miscellaneous combat item.";
		for(i = 0; i < NUMBATTLEITEMS; i++) {
			if(BattleItemDefs[i].itemNum == item) {
				if(BattleItemDefs[i].flags & BattleItemType::MAGEONLY) {
					*temp += " This item may only be used by a mage";
					if(Globals->APPRENTICES_EXIST) {
						*temp += " or an apprentice";
					}
					*temp += ".";
				}
				*temp += AString(" ") + "Item can cast " +
					ShowSpecial(BattleItemDefs[i].index,
							BattleItemDefs[i].skillLevel, 1, 1);
			}
		}
	}
	if((ItemDefs[item].flags & ItemType::CANTGIVE) && full) {
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
	delete vector;
}

void FactionVector::ClearVector()
{
	for (int i=0; i<vectorsize; i++) vector[i] = 0;
}

void FactionVector::SetFaction(int x, Faction *fac)
{
	vector[x] = fac;
}

Faction *FactionVector::GetFaction(int x)
{
	return vector[x];
}

Attitude::Attitude()
{
}

Attitude::~Attitude()
{
}

void Attitude::Writeout( Aoutfile *f )
{
	f->PutInt(factionnum);
	f->PutInt(attitude);
}

void Attitude::Readin( Ainfile *f, ATL_VER v )
{
	factionnum = f->GetInt();
	attitude = f->GetInt();
}

Faction::Faction()
{
	exists = 1;
	name = 0;
	for (int i=0; i<NFACTYPES; i++) {
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
	pStartLoc = NULL;
	noStartLeader = 0;
}

Faction::Faction(int n)
{
	exists = 1;
	num = n;
	for (int i=0; i<NFACTYPES; i++) {
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
	pStartLoc = NULL;
	noStartLeader = 0;
}

Faction::~Faction()
{
	if (name) delete name;
	if (address) delete address;
	if (password) delete password;
	attitudes.DeleteAll();
}

void Faction::Writeout( Aoutfile *f )
{
	f->PutInt(num);

	for (int i=0; i<NFACTYPES; i++) {
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
		((Attitude *) elem)->Writeout( f );
}

void Faction::Readin( Ainfile *f, ATL_VER v )
{
	num = f->GetInt();
	int i;

	for (i=0; i<NFACTYPES; i++) {
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
	if(defaultattitude == -1) {
		items.Readin(f);
		defaultattitude = f->GetInt();
	}

	int n = f->GetInt();
	for (i=0; i<n; i++) {
		Attitude * a = new Attitude;
		a->Readin(f,v);
		if (a->factionnum == num) {
			delete a;
		} else {
			attitudes.Add(a);
		}
	}
}

void Faction::View()
{
	AString temp;
	temp = AString("Faction ") + num + AString(" : ") + *name;
	Awrite(temp);
}

void Faction::SetName(AString * s)
{
	if (s) {
		AString * newname = s->getlegal();
		delete s;
		if (!newname) return;
		delete name;
		*newname += AString(" (") + num + ")";
		name = newname;
	}
}

void Faction::SetNameNoChange( AString *s )
{
	if( s ) {
		delete name;
		name = new AString( *s );
	}
}

void Faction::SetAddress( AString &strNewAddress )
{
	delete address;
	address = new AString( strNewAddress );
}

AString Faction::FactionTypeStr()
{
	AString temp;
	if (IsNPC()) return AString("NPC");

	if( Globals->FACTION_LIMIT_TYPE == GameDefs::FACLIM_UNLIMITED) {
		return (AString("Unlimited"));
	} else if(Globals->FACTION_LIMIT_TYPE == GameDefs::FACLIM_MAGE_COUNT) {
		return( AString( "Normal" ));
	} else if(Globals->FACTION_LIMIT_TYPE == GameDefs::FACLIM_FACTION_TYPES) {
		int comma = 0;
		for (int i=0; i<NFACTYPES; i++) {
			if (type[i]) {
				if (comma) {
					temp += ", ";
				} else {
					comma = 1;
				}
				temp += AString(FactionStrs[i]) + " " + type[i];
			}
		}
		if (!comma) return AString("none");
	}
	return temp;
}

void Faction::WriteReport( Areport *f, Game *pGame )
{
	if (IsNPC() && num == 1) {
		if(Globals->GM_REPORT || (pGame->month == 0 && pGame->year == 1)) {
			int i, j;
			// Put all skills, items and objects in the GM report
			shows.DeleteAll();
			for(i = 0; i < NSKILLS; i++) {
				for(j = 1; j < 6; j++) {
					shows.Add(new ShowSkill(i, j));
				}
			}
			if(shows.Num()) {
				f->PutStr("Skill reports:" );
				forlist(&shows) {
					AString *string = ((ShowSkill *)elem)->Report(this);
					if(string) {
						f->PutStr("");
						f->PutStr(*string);
						delete string;
					}
				}
				shows.DeleteAll();
				f->EndLine();
			}

			itemshows.DeleteAll();
			for(i = 0; i < NITEMS; i++) {
				AString *show = ItemDescription(i, 1);
				if(show) {
					itemshows.Add(show);
				}
			}
			if(itemshows.Num()) {
				f->PutStr("Item reports:");
				forlist(&itemshows) {
					f->PutStr("");
					f->PutStr(*((AString *)elem));
				}
				itemshows.DeleteAll();
				f->EndLine();
			}

			objectshows.DeleteAll();
			for(i = 0; i < NOBJECTS; i++) {
				AString *show = ObjectDescription(i);
				if(show) {
					objectshows.Add(show);
				}
			}
			if(objectshows.Num()) {
				f->PutStr("Object reports:");
				forlist(&objectshows) {
					f->PutStr("");
					f->PutStr(*((AString *)elem));
				}
				objectshows.DeleteAll();
				f->EndLine();
			}

			present_regions.DeleteAll();
			forlist(&(pGame->regions)) {
				ARegion *reg = (ARegion *)elem;
				ARegionPtr *ptr = new ARegionPtr;
				ptr->ptr = reg;
				present_regions.Add(ptr);
			}
			{
				forlist(&present_regions) {
					((ARegionPtr*)elem)->ptr->WriteReport(f, this,
														  pGame->month,
														  &(pGame->regions));
				}
			}
			present_regions.DeleteAll();
		}
		errors.DeleteAll();
		events.DeleteAll();
		battles.DeleteAll();
		return;
	}

	f->PutStr("Atlantis Report For:");
	if((Globals->FACTION_LIMIT_TYPE == GameDefs::FACLIM_MAGE_COUNT) ||
			(Globals->FACTION_LIMIT_TYPE == GameDefs::FACLIM_UNLIMITED)) {
		f->PutStr( *name );
	} else if(Globals->FACTION_LIMIT_TYPE == GameDefs::FACLIM_FACTION_TYPES) {
		f->PutStr(*name + " (" + FactionTypeStr() + ")");
	}
	f->PutStr(AString(MonthNames[ pGame->month ]) + ", Year " + pGame->year );
	f->EndLine();

	f->PutStr( AString( "Atlantis Engine Version: " ) +
			ATL_VER_STRING( CURRENT_ATL_VER ));
	f->PutStr( AString( Globals->RULESET_NAME ) + ", Version: " +
			ATL_VER_STRING( Globals->RULESET_VERSION ));
	f->EndLine();

	if (!times) {
		f->PutStr("Note: The Times is not being sent to you.");
		f->EndLine();
	}

	if(*password == "none") {
		f->PutStr("REMINDER: You have not set a password for your faction!");
		f->EndLine();
	}

	if(Globals->MAX_INACTIVE_TURNS != -1) {
		int cturn = pGame->TurnNumber() - lastorders;
		if((cturn >= (Globals->MAX_INACTIVE_TURNS - 3)) && !IsNPC()) {
			cturn = Globals->MAX_INACTIVE_TURNS - cturn;
			f->PutStr( AString("WARNING: You have ") + cturn +
					AString(" turns until your faction is automatically ")+
					AString("removed due to inactivity!"));
			f->EndLine();
		}
	}

	if (!exists) {
		if (quit == QUIT_AND_RESTART) {
			f->PutStr( "You restarted your faction this turn. This faction "
					"has been removed, and a new faction has been started "
					"for you. (Your new faction report will come in a "
					"separate message.)" );
		} else if( quit == QUIT_GAME_OVER ) {
			f->PutStr( "I'm sorry, the game has ended. Better luck in "
					"the next game you play!" );
		} else if( quit == QUIT_WON_GAME ) {
			f->PutStr( "Congratulations, you have won the game!" );
		} else {
			f->PutStr( "I'm sorry, your faction has been eliminated." );
			// LLS
			f->PutStr( "If you wish to restart, please let the "
					"Gamemaster know, and you will be restarted for "
					"the next available turn." );
		}
		f->PutStr( "" );
	}

	f->PutStr("Faction Status:");
	if(Globals->FACTION_LIMIT_TYPE == GameDefs::FACLIM_MAGE_COUNT) {
		f->PutStr( AString("Mages: ") + nummages + " (" +
				pGame->AllowedMages( this ) + ")");
		if(Globals->APPRENTICES_EXIST) {
			f->PutStr( AString("Apprentices: ") + numapprentices + " (" +
					pGame->AllowedApprentices(this)+ ")");
		}
	} else if(Globals->FACTION_LIMIT_TYPE == GameDefs::FACLIM_FACTION_TYPES) {
		f->PutStr( AString("Tax Regions: ") + war_regions.Num() + " (" +
				pGame->AllowedTaxes( this ) + ")");
		f->PutStr( AString("Trade Regions: ") + trade_regions.Num() + " (" +
				pGame->AllowedTrades( this ) + ")");
		f->PutStr( AString("Mages: ") + nummages + " (" +
				pGame->AllowedMages( this ) + ")");
		if(Globals->APPRENTICES_EXIST) {
			f->PutStr( AString("Apprentices: ") + numapprentices + " (" +
					pGame->AllowedApprentices(this)+ ")");
		}
	}
	f->PutStr("");

	if (errors.Num()) {
		f->PutStr("Errors during turn:");
		forlist((&errors)) {
			f->PutStr(*((AString *) elem));
		}
		errors.DeleteAll();
		f->EndLine();
	}

	if (battles.Num()) {
		f->PutStr("Battles during turn:");
		forlist(&battles) {
			((BattlePtr *) elem)->ptr->Report(f,this);
		}
		battles.DeleteAll();
	}

	if (events.Num()) {
		f->PutStr("Events during turn:");
		forlist((&events)) {
			f->PutStr(*((AString *) elem));
		}
		events.DeleteAll();
		f->EndLine();
	}

	if (shows.Num()) {
		f->PutStr("Skill reports:");
		forlist(&shows) {
			AString * string = ((ShowSkill *) elem)->Report(this);
			if (string) {
				f->PutStr("");
				f->PutStr(*string);
			}
			delete string;
		}
		shows.DeleteAll();
		f->EndLine();
	}

	if (itemshows.Num()) {
		f->PutStr("Item reports:");
		forlist(&itemshows) {
			f->PutStr("");
			f->PutStr(*((AString *) elem));
		}
		itemshows.DeleteAll();
		f->EndLine();
	}

	if(objectshows.Num()) {
		f->PutStr("Object reports:");
		forlist(&objectshows) {
			f->PutStr("");
			f->PutStr(*((AString *)elem));
		}
		objectshows.DeleteAll();
		f->EndLine();
	}

	/* Attitudes */
	AString temp = AString("Declared Attitudes (default ") +
		AttitudeStrs[defaultattitude] + "):";
	f->PutStr(temp);
	for (int i=0; i<NATTITUDES; i++) {
		int j=0;
		temp = AString(AttitudeStrs[i]) + " : ";
		forlist((&attitudes)) {
			Attitude * a = (Attitude *) elem;
			if (a->attitude == i) {
				if (j) temp += ", ";
				temp += *( GetFaction( &( pGame->factions ),
							a->factionnum)->name);
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
		((ARegionPtr *) elem)->ptr->WriteReport( f, this, pGame->month,
												 &( pGame->regions ));
	} 

	if (temformat != TEMPLATE_OFF) {
		f->PutStr("");

		switch (temformat) {
			case TEMPLATE_SHORT:
				f->PutStr("Orders Template (Short Format):");
				break;
			case TEMPLATE_LONG:
				f->PutStr("Orders Template (Long Format):");
				break;
				// DK
			case TEMPLATE_MAP:
				f->PutStr("Orders Template (Map Format):");
				break;
		}

		f->PutStr("");
		temp = AString("#atlantis ") + num;
		if (!(*password == "none")) {
			temp += AString(" \"") + *password + "\"";
		}
		f->PutStr(temp);
		forlist((&present_regions)) {
			// DK
			((ARegionPtr *) elem)->ptr->WriteTemplate( f, this,
													   &( pGame->regions ),
													   pGame->month );
		}
	} else {
		f->PutStr("");
		f->PutStr("Orders Template (Off)");
	}

	f->PutStr("");
	f->PutStr("#end");
	f->EndLine();

	present_regions.DeleteAll();
}

void Faction::WriteFacInfo( Aoutfile *file )
{
	file->PutStr( AString( "Faction: " ) + num );
	file->PutStr( AString( "Name: " ) + *name );
	file->PutStr( AString( "Email: " ) + *address );
	file->PutStr( AString( "Password: " ) + *password );
	file->PutStr( AString( "LastOrders: " ) + lastorders );
	file->PutStr( AString( "SendTimes: " ) + times );

	forlist( &extraPlayers ) {
		AString *pStr = (AString *) elem;
		file->PutStr( *pStr );
	}

	extraPlayers.DeleteAll();
}

void Faction::CheckExist(ARegionList * regs)
{
    if (IsNPC()) return;
	exists = 0;
	forlist(regs) {
		ARegion * reg = (ARegion *) elem;
		if (reg->Present(this)) {
			exists = 1;
			return;
		}
	}
}

void Faction::Error(const AString &s)
{
	if (IsNPC()) return;
	if (errors.Num() > 1000) {
		if (errors.Num() == 1001) {
			errors.Add(new AString("Too many errors!"));
		}
		return;
	}

	AString *temp = new AString(s);
	errors.Add(temp);
}

void Faction::Event(const AString &s)
{
	if (IsNPC()) return;
	AString *temp = new AString(s);
	events.Add(temp);
}

void Faction::RemoveAttitude(int f)
{
	forlist((&attitudes)) {
		Attitude *a = (Attitude *) elem;
		if (a->factionnum == f) {
			attitudes.Remove(a);
			delete a;
			return;
		}
	}
}

int Faction::GetAttitude(int n)
{
	if (n == num) return A_ALLY;
	forlist((&attitudes)) {
		Attitude *a = (Attitude *) elem;
		if (a->factionnum == n)
			return a->attitude;
	}
	return defaultattitude;
}

void Faction::SetAttitude(int num,int att)
{
	forlist((&attitudes)) {
		Attitude *a = (Attitude *) elem;
		if (a->factionnum == num) {
			if (att == -1) {
				attitudes.Remove(a);
				delete a;
				return;
			} else {
				a->attitude = att;
				return;
			}
		}
	}
	if (att != -1) {
		Attitude *a = new Attitude;
		a->factionnum = num;
		a->attitude = att;
		attitudes.Add(a);
	}
}

int Faction::CanCatch(ARegion *r, Unit *t)
{
	if (TerrainDefs[r->type].similar_type == R_OCEAN) return 1;

	int def = t->GetDefenseRiding();

	forlist(&r->objects) {
		Object *o = (Object *) elem;
		forlist(&o->units) {
			Unit *u = (Unit *) elem;
			if (u == t && o->type != O_DUMMY) return 1;
			if (u->faction == this && u->GetAttackRiding() >= def) return 1;
		}
	}
	return 0;
}

int Faction::CanSee(ARegion * r,Unit * u, int practise)
{
	int detfac = 0;
	if (u->faction == this) return 2;
	if (u->reveal == REVEAL_FACTION) return 2;
	int retval = 0;
	if (u->reveal == REVEAL_UNIT) retval = 1;
	forlist((&r->objects)) {
		Object * obj = (Object *) elem;
		int dummy = 0;
		if (obj->type == O_DUMMY) dummy = 1;
		forlist((&obj->units)) {
			Unit * temp = (Unit *) elem;
			if (u == temp && dummy == 0) retval = 1;
			if (temp->faction == this) {
				if (temp->GetSkill(S_OBSERVATION) > u->GetSkill(S_STEALTH)) {
					if (practise) {
						temp->Practise(S_OBSERVATION);
						temp->Practise(S_TRUE_SEEING);
						retval = 2;
					}
					else
						return 2;
				} else {
					if (temp->GetSkill(S_OBSERVATION)==u->GetSkill(S_STEALTH)) {
						if (practise) {
							temp->Practise(S_OBSERVATION);
							temp->Practise(S_TRUE_SEEING);
						}
						if (retval < 1) retval = 1;
					}
				}
				if (temp->GetSkill(S_MIND_READING) > 2) detfac = 1;
			}
		}
	}
	if (retval == 1 && detfac) return 2;
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
	if (Globals->TIMES_REWARD) {
		Event(AString("Times reward of ") + Globals->TIMES_REWARD + " silver.");
		unclaimed += Globals->TIMES_REWARD;
	}
}

void Faction::SetNPC()
{
	for (int i=0; i<NFACTYPES; i++) type[i] = -1;
}

int Faction::IsNPC()
{
	if (type[F_WAR] == -1) return 1;
	return 0;
}

Faction *GetFaction(AList *facs, int n)
{
	forlist(facs)
		if (((Faction *) elem)->num == n)
			return (Faction *) elem;
	return 0;
}

Faction *GetFaction2(AList *facs, int n)
{
	forlist(facs)
		if (((FactionPtr *) elem)->ptr->num == n)
			return ((FactionPtr *) elem)->ptr;
	return 0;
}

void Faction::DiscoverItem(int item, int force, int full)
{
	int seen = items.GetNum(item);
	if(!seen) {
		if(full) {
			items.SetNum(item, 2);
		} else {
			items.SetNum(item, 1);
		}
		force = 1;
	} else {
		if(seen == 1) {
			if(full) {
				items.SetNum(item, 2);
			}
			force = 1;
		} else {
			full = 1;
		}
	}
	if(force) {
		itemshows.Add(ItemDescription(item, full));
	}   
}

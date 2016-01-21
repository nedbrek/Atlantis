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
#include "skills.h"
#include "items.h"
#include "gamedata.h"
#include "gamedefs.h"
#include "fileio.h"
#include "astring.h"

int ParseSkill(const AString *token)
{
	for (int i = 0; i < NSKILLS; ++i)
	{
		if (*token == SkillDefs[i].name ||
		    *token == SkillDefs[i].abbr)
		{
			if (SkillDefs[i].flags & SkillType::DISABLED)
				return -1;

			return i;
		}
	}

	return -1;
}

AString SkillStrs(int i)
{
	return AString(SkillDefs[i].name) + " [" + SkillDefs[i].abbr + "]";
}

int SkillCost(int skill)
{
	return SkillDefs[skill].cost;
}

int SkillMax(int skill, int race)
{
	// if non-leaders can't study magic
	if (!Globals->MAGE_NONLEADERS)
	{
		// if skill requires magic and unit is not leaders
		if ((SkillDefs[skill].flags & SkillType::MAGIC) && race != I_LEADERS)
			return 0;
	}

	const int mantype = ItemDefs[race].index;

	for (unsigned c = 0; c < sizeof(ManDefs[mantype].skills) / sizeof(ManDefs[mantype].skills[0]); ++c)
	{
		if (ManDefs[mantype].skills[c] == skill)
			return ManDefs[mantype].speciallevel;
	}
	return ManDefs[mantype].defaultlevel;
}

int GetLevelByDays(int dayspermen)
{
	int z = 30;
	int i = 0;
	while (dayspermen >= z)
	{
		++i;
		dayspermen -= z;
		z += 30;
	}
	return i;
}

int GetDaysByLevel(int level)
{
	int days = 0;

	for (; level > 0; --level)
	{
		days += level * 30;
	}

	return days;
}

//----------------------------------------------------------------------------
Skill::Skill(int t, unsigned d)
: type(t)
, days(d)
{
}

void Skill::Readin(Ainfile *f)
{
	type = f->GetInt();
	days = f->GetInt();
}

void Skill::Writeout(Aoutfile *f) const
{
	f->PutInt(type);
	f->PutInt(days);
}

Skill* Skill::Split(int total, int leave)
{
	Skill *temp = new Skill(type, (days * leave) / total);

	days -= temp->days;
	return temp;
}

//----------------------------------------------------------------------------
int SkillList::GetDays(int skill)
{
	forlist(this)
	{
		const Skill *const s = (Skill*)elem;
		if (s->type == skill)
			return s->days;
	}
	return 0;
}

void SkillList::SetDays(int skill, int days)
{
	//foreach skill known
	forlist(this)
	{
		// look for the given skill
		Skill *s = (Skill*)elem;
		if (s->type != skill)
			continue;

		if (days == 0)
		{
			Remove(s);
			delete s;
			return;
		}
		//else
		s->days = days;
		return;
	}
	//else new skill

	if (days != 0)
		Add(new Skill(skill, days));
}

SkillList* SkillList::Split(int total, int leave)
{
	SkillList *ret = new SkillList;
	forlist(this)
	{
		Skill *s = (Skill*)elem;

		Skill *n = s->Split(total, leave);
		if (s->days == 0)
		{
			Remove(s);
			delete s;
		}
		ret->Add(n);
	}
	return ret;
}

void SkillList::Combine(SkillList *b)
{
	forlist(b)
	{
		const Skill *const s = (Skill*)elem;
		SetDays(s->type, GetDays(s->type) + s->days);
	}
}

AString SkillList::Report(int nummen)
{
	if (!Num())
		return "none";

	AString temp;
	bool i = false;
	forlist(this)
	{
		const Skill *const s = (Skill*)elem;

		if (i)
			temp += ", ";
		else
			i = true;

		temp += SkillStrs(s->type);
		temp += AString(" ") + GetLevelByDays(s->days/nummen) +
		   AString(" (") + AString(s->days/nummen) + AString(")");
	}
	return temp;
}

void SkillList::Readin(Ainfile *f)
{
	const int n = f->GetInt();
	for (int i = 0; i < n; ++i)
	{
		Skill *s = new Skill;
		s->Readin(f);

		if (s->days == 0)
			delete s;
		else
			Add(s);
	}
}

void SkillList::Writeout(Aoutfile *f)
{
	f->PutInt(Num());
	forlist(this)
	{
		((Skill*)elem)->Writeout(f);
	}
}

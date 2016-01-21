#ifndef SKILLS_H
#define SKILLS_H
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
#include "alist.h"

class Ainfile;
class Aoutfile;
class AString;
class Faction;

//----------------------------------------------------------------------------
/* Dependencies:
  A value of depend == -1 indicates no more dependencies.
  If depend is set to a skill, to study this skill, you must know
  the depended skill at level equal to (at least) the level in the
  structure, or at the level you are trying to study to.

  Example:
  SANDLE has depends[0].skill = SHOE and depends[0].level = 3.

  To study:   requires:
  SANDLE 1    SHOE 3
  SANDLE 2    SHOE 3
  SANDLE 3    SHOE 3
  SANDLE 4    SHOE 4
  SANDLE 5    SHOE 5
*/
struct SkillDepend
{
	int skill; ///< index of the skill depended on
	int level; ///< level required in prerequisite skill
};

//----------------------------------------------------------------------------
/// Global definition of a skill
class SkillType
{
public:
	const char *name; ///< long name
	const char *abbr; ///< short name
	int cost; ///< in silver, per man-month

	enum
	{
		MAGIC      = 0x01, ///< related to magic
		COMBAT     = 0x02, ///< can help cause damage
		CAST       = 0x04, ///< must be cast
		FOUNDATION = 0x08, ///< a foundation of magic
		APPRENTICE = 0x10, ///< helps users of magic items
		DISABLED   = 0x20, ///< disabled in this game
		SLOWSTUDY  = 0x40  ///< study rate halved outside buildings
	};
	int flags; ///< combination of above

	int special; ///< (for combat spells only)

	int rangeIndex; ///< range class for ranged skills (-1 for all others)

	SkillDepend depends[3]; ///< prerequisites
};
extern SkillType *SkillDefs; ///< global table of skill definitions

///@return index for skill named by 'token' (-1 if not found)
int ParseSkill(const AString *token);

///@return long name and short name together ("Long Name [LNNM]")
AString SkillStrs(int skill);

///@return cost (in silver) to study 'skill' for one man for one month
int SkillCost(int skill);

///@return the maximum level that can be studied in 'skill' for a man of type 'race'
int SkillMax(int skill, int race);

///@return level corresponding to study of 'days per man'
int GetLevelByDays(int days_per_man);

///@return number of days of study corresponding to 'level'
int GetDaysByLevel(int level);

//----------------------------------------------------------------------------
/// Description of a skill (for player reports)
class ShowSkill : public AListElem
{
public:
	ShowSkill(int s, int l);

	///@return new string with description of this (NULL if disabled)
	AString* Report(Faction *f);

	int skill; ///< index into global table
	int level; ///< level achieved
};

//----------------------------------------------------------------------------
/// One skill known by a unit of men
class Skill : public AListElem
{
public:
	Skill(int t = 0, unsigned d = 0);

	void Readin(Ainfile *f);
	void Writeout(Aoutfile *f) const;

	/// adjust this 'days' where some men leave (out of the total)
	///@return new Skill of men leaving
	Skill* Split(int total, int num_leaving);

	int      type; ///< index into global table
	unsigned days; ///< total number of man-days studied
};

//----------------------------------------------------------------------------
/// All the skills known by a unit of men
class SkillList : public AList
{
public:
	void Readin(Ainfile *f);
	void Writeout(Aoutfile *f);

	///@return string detailing this
	AString Report(int num_men);

	/// foreach skill in this, add corresponding days from 'b'
	///@NOTE: skills in 'b' not in this are not added
	void Combine(SkillList *b);

	///@return new list corresponding to the new unit, adjust this
	SkillList* Split(int total_men, int num_leaving);

	///@return number of days of study for 'skill'
	int GetDays(int skill);

	/// set days of study for 'skill' to 'new_days' (0 to remove)
	void SetDays(int skill, int new_days);
};

#endif


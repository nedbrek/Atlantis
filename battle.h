#ifndef BATTLE_CLASS
#define BATTLE_CLASS
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
class ARegion;
class ARegionList;
class Areport;
class Army;
class AString;
class Faction;
class ItemList;
class Soldier;
class Unit;

//----------------------------------------------------------------------------
/// Result of assassination attempt
enum
{
	ASS_NONE,
	ASS_SUCC,
	ASS_FAIL
};

/// Result of battle
enum
{
	BATTLE_IMPOSSIBLE,
	BATTLE_LOST,
	BATTLE_WON,
	BATTLE_DRAW
};

//----------------------------------------------------------------------------
/// Handles battle mechanics and reporting
class Battle : public AListElem
{
public:
	Battle();
	~Battle();

	/// Send this battle report to 'f' for 'fac'
	void Report(Areport *f, Faction *fac);

	/// Add 's' to this battle report
	void AddLine(const AString &s);

	int Run(ARegion *region, Unit *att, AList *atts, Unit *tar, AList *defs, int ass, ARegionList *pRegs);

	void WriteSides(ARegion *r, Unit *att, Unit *tar, AList *atts, AList *defs, int ass, ARegionList *pRegs);

private: // methods
	void FreeRound(Army *att, Army *def, int ass = 0);
	void NormalRound(int round, Army *a, Army *b);
	void DoAttack(int round, Soldier *a, Army *attackers, Army *def,
	         int behind, bool ass = false);
	void GetSpoils(AList *losers, ItemList *spoils, int ass);

	// These functions should be implemented in specials.cpp
	void UpdateShields(Army *);
	void DoSpecialAttack(int round, Soldier *a, Army *attackers, Army *def, int behind);

public: // data
	int assassination;
	Faction *attacker; ///< Only matters in the case of an assassination
	AString *asstext;
	AList text;
};

class BattlePtr : public AListElem
{
public:
	Battle *ptr;
};

#endif


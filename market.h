#ifndef MARKET_CLASS
#define MARKET_CLASS
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

/// Types of market listings
enum
{
	M_BUY,
	M_SELL
};

/// One item in a market
class Market : public AListElem
{
public:
	Market();

	Market(int type, int item, int price, int amount, int minpop, int maxpop, int minamt, int maxamt);

	void PostTurn(int pop, int wages);
	void Writeout(Aoutfile *f);
	void Readin(Ainfile *f);
	AString Report() const;

	int type; ///< buy or sell
	int item; ///< item index
	int price; ///< in silver
	int amount; ///< max number per turn

	int minpop;
	int maxpop;
	int minamt;
	int maxamt;

	int baseprice;
	int activity;
};

/// All items in a market
class MarketList : public AList
{
public:
	void PostTurn(int pop, int wages);
	void Writeout(Aoutfile *f);
	void Readin(Ainfile *f);
};

#endif

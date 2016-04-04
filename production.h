#ifndef PRODUCTION_CLASS
#define PRODUCTION_CLASS
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

/// One unit of production
class Production : public AListElem
{
public:
	Production(int item_type, int max_amt);
	Production();

	void Writeout(Aoutfile *f);
	void Readin(Ainfile *f);

	///@NOTE does not write anywhere, @return description
	AString WriteReport();

	int itemtype;
	int baseamount;
	int amount;
	int skill;
	int level;
	int productivity;
	int activity;
};

/// A list of production items
class ProductionList : public AList
{
public:
	Production* GetProd(int item_type, int skill);

	/// add 'p' to this, removes any pre-existing production that matches
	void AddProd(Production *p);

	void Writeout(Aoutfile *f);
	void Readin(Ainfile *f);
};

#endif


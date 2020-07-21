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

AListElem::~AListElem()
{
}

AList::AList()
{
	list = 0;
	lastelem = 0;
	num = 0;
}

AList::~AList()
{
	deleteAll();
}

void AList::deleteAll()
{
	while (list)
	{
		AListElem *temp = list->next;
		delete list;
		list = temp;
	}
	lastelem = 0;
	num = 0;
}

void AList::clear()
{
	while (list) {
		AListElem *temp = list->next;
		list->next = 0;
		list = temp;
	}
	lastelem = 0;
	num = 0;
}

void AList::push_front(AListElem *e)
{
	++num;
	e->next = list;
	list = e;
	if (!lastelem)
		lastelem = list;
}

void AList::push_back(AListElem *e)
{
	++num;
	if (list)
	{
		lastelem->next = e;
		e->next = 0;
		lastelem = e;
	}
	else
	{
		list = e;
		e->next = 0;
		lastelem = list;
	}
}

AListElem* AList::next(AListElem *e) const
{
	return e ? e->next : 0;
}

AListElem* AList::front() const
{
	return list;
}

AListElem* AList::get(AListElem *e)
{
	AListElem *temp = list;
	while (temp)
	{
		if (temp == e)
			return temp;
		temp = temp->next;
	}
	return nullptr;
}

bool AList::remove(AListElem *e)
{
	if (!e)
		return false;

	// if e is the last item
	if (!e->next)
		lastelem = 0;

	for (AListElem **pp = &list; *pp; pp = &((*pp)->next))
	{
		if (*pp == e)
		{
			*pp = e->next;
			--num;
			return true;
		}

		if (!e->next)
			lastelem = *pp;
	}
	return false;
}

int AList::size() const
{
	return num;
}

int AList::nextLive(AListElem **copy, int size, int pos) const
{
	while (++pos < size)
	{
		for (AListElem *elem = front(); elem; elem = elem->next)
		{
			if (elem == copy[pos])
				return pos;
		}
	}
	return pos;
}

#ifndef ALIST_CLASS
#define ALIST_CLASS
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

/**
 * Base class for intrusive single-linked list
 */
class AListElem
{
public:
	virtual ~AListElem();

	AListElem *next;
};

/**
 * a collection of AListElem
 */
class AList
{
public:
	AList();
	~AList();

	/// delete all elements
	void DeleteAll() { deleteAll(); }
	void deleteAll();

	/// clear this without deleting members
	void Empty() { clear(); }
	void clear();

	///@return 'e' if it is in this, else NULL
	AListElem* Get(AListElem *e) { return get(e); }
	AListElem* get(AListElem *e);

	///@return the element after 'e'
	AListElem* Next(AListElem *e) const { return next(e); }
	AListElem* next(AListElem *e) const;

	///@return true if 'e' was in this (before being removed)
	bool Remove(AListElem *e) { return remove(e); }
	bool remove(AListElem *e);

	/// into the front
	void Insert(AListElem *e) { push_front(e); }
	void push_front(AListElem *e);

	/// to the back
	void Add(AListElem *e) { push_back(e); }
	void push_back(AListElem *e);

	///@return the first element
	AListElem* First() const { return front(); }
	AListElem* front() const;

	///@return number of elements in this
	int Num() const  { return size(); }
	int size() const;

	// Helper function for forlist_safe
	int NextLive(AListElem **copy, int size, int pos) const
	{
		return nextLive(copy, size, pos);
	}
	int nextLive(AListElem **copy, int size, int pos) const;

private:
	AListElem *list;
	AListElem *lastelem;
	int num;
};

#define forlist(l) \
	AListElem *elem, *_elem2; \
	for (elem=(l)->First(), \
			_elem2 = (elem ? (l)->Next(elem) : 0); \
			elem; \
			elem = _elem2, \
			_elem2 = (_elem2 ? ((l)->Next(_elem2)) : 0))

#define forlist_safe(l) \
	int size = (l)->Num(); \
	AListElem **copy = new AListElem*[size]; \
	AListElem *elem; \
	int pos; \
	for (pos = 0, elem = (l)->First(); elem; elem = elem->next, pos++) { \
		copy[pos] = elem; \
	} \
	for (pos = 0; \
			pos < size ? (elem = copy[pos], 1) : (delete [] copy, 0); \
			pos = (l)->NextLive(copy, size, pos))

#endif

#ifndef ASTRING_CLASS
#define ASTRING_CLASS
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
#include <iostream>

class AString : public AListElem
{
	friend std::ostream& operator<<(std::ostream &os, const AString&);
	friend std::istream& operator>>(std::istream &is, AString&);

public:
	AString();
	AString(const char*);
	AString(int);
	AString(unsigned);
	AString(char);
	AString(const AString&);
	~AString();

	bool operator==(const AString&) const;
	bool operator==(const char*) const;

	AString operator+(const AString&) const;
	AString& operator+=(const AString&);

	AString& operator=(const AString&);
	AString& operator=(const char*);

	///@deprecated
	AString* StripWhite() const { return stripWhite(); }
	///@return a new string without leading whitespace (space and tab)
	AString* stripWhite() const;

	///@deprecated
	char* Str() { return str(); }
	///@return the underlying char array
	char* str();
	const char* str() const;

	///@deprecated
	int Len() const { return len(); }
	///@return the string length
	int len() const;

	///@return the integer representation of this
	int value() const;

	/// get strict integer representation of this. @return true on success
	bool strict_value(int *v) const;

	///@return a new string with only legal chars, or NULL if this is all blanks or illegal
	AString* getlegal() const;

	/**
	 * @NOTE: this is modified to only contain the remainder after consuming the token
	 * @return a new string with the token
	 */
	AString* gettoken();

	/**
	 * @NOTE: changes the first '@' to space
	 * @return 1 if there was an '@' in this, else 0
	 */
	int getat();

	///@deprecated
	AString* Trunc(int a, int back=30) { return trunc(a, back); }
	/*
	 * truncate this in some fashion
	 * @return a new string representing the remainder of the string
	 */
	AString* trunc(int, int back=30);

private:
	bool isEqual(const char*) const;

private:
	int len_;
	char *str_;
};

#endif


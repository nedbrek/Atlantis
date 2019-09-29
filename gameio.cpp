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
#include "gameio.h"
#include "astring.h"
#include <stdlib.h>
#include <time.h>
#include <iostream>

char buf[256];

static void clearToEndl()
{
	char ch = ' ';
	while (!std::cin.eof() && ch != '\n')
	{
		ch = std::cin.get();
	}
}

static void morewait()
{
	std::cout << '\n';
	std::cin.getline(buf, sizeof(buf), '\n');
	std::cout << '\n';
}

void initIO()
{
    // I am unsure of why this is hard-coded.
	seedrandom( 1783 );
}

void doneIO()
{
}

int getrandom(int range)
{
	if (!range)
		return 0;

	const bool neg = (range < 0);
	if (neg)
		range = -range;

	int random = rand() % range;

	if (neg)
		random = (int)(random * -1);

	return random;
}

void seedrandom(int num)
{
    // Now to do this directly.
    srand((unsigned int)num);
}

void seedrandomrandom()
{
	time_t now = clock();
    seedrandom((unsigned int)now);
}

int Agetint()
{
	int x;
	std::cin >> x;
	clearToEndl();
	return x;
}

void Awrite(const AString &s)
{
	std::cout << s << '\n';
}

void Adot()
{
	std::cout << ".";
}

void message(const char *c)
{
	std::cout << c << '\n';
	// wait for user to acknowledge
	morewait();
}

AString* getfilename(const AString &s)
{
	std::cout << s;
	return AGetString();
}

AString* AGetString()
{
	std::cin.getline(buf, sizeof(buf), '\n');
	return new AString(buf);
}

#ifndef GAME_IO
#define GAME_IO
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
class AString;

/// initialize system resources
void initIO();
/// deconstruct system resources
void doneIO();

///@return a random number from 0 to (r-1)
int getrandom(int r);

/// Seed the random number generator
void seedrandom(int);
void seedrandomrandom();

///@return an int from the user
int Agetint();

/// write a string to stdout
void Awrite(const AString &);

/// write a dot ('.') to stdout
void Adot();

/// print a message and wait for user to acknowledge
void message(const char *);

///@return a new string from the user (with prompt)
AString* getfilename(const AString &prompt);

///@return a new string from the user
AString* AGetString();

#endif


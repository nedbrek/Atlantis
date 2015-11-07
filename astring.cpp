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
#include "astring.h"
#include <cstring>
#include <cstdio>
#include <string>

static bool islegal(char c)
{
	return (c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9') ||
	        c=='!' ||  c=='[' || c==']' || c==',' || c=='.' || c==' ' ||
	        c=='{' || c=='}' || c=='@' || c=='#' || c=='$' || c=='%' ||
	        c=='^' || c=='&' || c=='*' || c=='-' || c=='_' || c=='+' ||
	        c=='=' || c==';' || c==':' || c=='<' || c=='>' || c=='?' ||
	        c=='/' || c=='~' || c=='\'' || c== '\\' || c=='`';
}

AString::AString()
{
	len_ = 0;
	str_ = new char[1];
	str_[0] = '\0';
}

AString::AString(const char *s)
{
	len_ = strlen(s);
	str_ = new char[len_ + 1];
	strcpy(str_, s);
}

AString::AString(int l)
{
	char buf[16];
	sprintf(buf, "%d", l);
	len_ = strlen(buf);
	str_ = new char[len_ + 1];
	strcpy(str_, buf);
}

AString::AString(unsigned l)
{
	char buf[16];
	sprintf(buf, "%u", l);
	len_ = strlen(buf);
	str_ = new char[len_ + 1];
	strcpy(str_, buf);
}

AString::AString(char c)
{
	len_ = 1;
	str_ = new char[2];
	str_[0] = c;
	str_[1] = '\0';
}

AString::~AString()
{
	delete str_;
}

AString::AString(const AString &s)
{
	len_ = s.len_;
	str_ = new char[len_ + 1];
	strcpy(str_, s.str_);
}

AString& AString::operator=(const AString &s)
{
	delete str_;
	len_ = s.len_;
	str_ = new char[len_ + 1];
	strcpy(str_, s.str_);
	return *this;
}

AString& AString::operator=(const char *c)
{
	delete str_;
	len_ = strlen(c);
	str_ = new char[len_ + 1];
	strcpy(str_, c);
	return *this;
}

bool AString::operator==(const char *s) const
{
	return isEqual(s);
}

bool AString::operator==(const AString &s) const
{
	return isEqual(s.str_);
}

bool AString::isEqual(const char *temp2) const
{
	const char *temp1 = str_;
	while (*temp1 && *temp2)
	{
		char t1 = *temp1;
		if ((t1 >= 'A') && (t1 <= 'Z'))
			t1 = t1 - 'A' + 'a';
		if (t1 == '_') t1 = ' ';

		char t2 = *temp2;
		if ((t2 >= 'A') && (t2 <= 'Z'))
			t2 = t2 - 'A' + 'a';

		if (t2 == '_') t2 = ' ';

		if (t1 != t2) return false;

		++temp1;
		++temp2;
	}

	return *temp1 == *temp2;
}

AString AString::operator+(const AString &s) const
{
	char *temp = new char[len_ + s.len_ + 1];

	int i; // live-out
	for (i = 0; i < len_; ++i)
	{
		temp[i] = str_[i];
	}

	for (int j = 0; j < s.len_ + 1; ++j)
	{
		temp[i++] = s.str_[j];
	}

	AString temp2 = AString(temp);
	delete temp;
	return temp2;
}

AString& AString::operator+=(const AString &s)
{
	char *temp = new char[len_ + s.len_ + 1];

	int i; // live-out
	for (i = 0; i < len_; ++i)
	{
		temp[i] = str_[i];
	}

	for (int j = 0; j < s.len_ + 1; ++j)
	{
		temp[i++] = s.str_[j];
	}

	delete str_;
	str_ = temp;
	len_ += s.len_;

	return *this;
}

char* AString::str()
{
	return str_;
}

const char* AString::str() const
{
	return str_;
}

int AString::len() const
{
	return len_;
}

AString* AString::gettoken()
{
	// find start of non-whitespace token
	int place = 0;
	while (place < len_ && (str_[place] == ' ' || str_[place] == '\t'))
		++place;

	// all whitespace
	if (place >= len_) return 0;
	// only comment
	if (str_[place] == ';') return 0;

	char buf[1024];
	size_t place2 = 0;

	// handle quoted token
	if (str_[place] == '"')
	{
		++place; // skip start quote

		while (place2 < sizeof(buf)-1 && place < len_ && str_[place] != '"')
		{
			// copy token to buf
			buf[place2++] = str_[place++];
		}

		if (place == len_ || place2 == sizeof(buf)-1)
		{
			// Unmatched "" return 0, truncate self (no more tokens)
			delete str_;
			str_ = new char[1];
			len_ = 0;
			str_[0] = '\0';

			return 0;
		}
		//else

		// skip trailing quote
		++place;
	}
	else // unquoted token
	{
		while (place2 < sizeof(buf)-1 && place < len_ &&
		       str_[place] != ' ' && str_[place] != '\t' && str_[place] != ';')
		{
			buf[place2++] = str_[place++];
		}
	}

	buf[place2] = '\0';

	if (place2 == sizeof(buf)-1 || place == len_ || str_[place] == ';')
	{
		delete str_;
		str_ = new char[1];
		len_ = 0;
		str_[0] = '\0';

		return new AString(buf);
	}

	// adjust self to remainder
	char *buf2 = new char[len_ - place2 + 1];
	int newlen = 0;
	place2 = 0;
	while (place < len_)
	{
		buf2[place2++] = str_[place++];

		++newlen;
	}

	buf2[place2] = '\0';

	len_ = newlen;
	delete str_;
	str_ = buf2;

	// return token
	return new AString(buf);
}

AString* AString::stripWhite() const
{
	int place = 0;
	while (place < len_ && (str_[place] == ' ' || str_[place] == '\t'))
	{
		++place;
	}

	if (place >= len_)
		return 0;

	return new AString(str_ + place);
}

int AString::getat()
{
	int place = 0;
	while (place < len_ && (str_[place] == ' ' || str_[place] == '\t'))
		++place;

	if (place >= len_) return 0;

	if (str_[place] == '@')
	{
		str_[place] = ' ';
		return 1;
	}
	return 0;
}

AString* AString::getlegal() const
{
	char *temp = new char[len_ + 1];
	char *temp2 = temp;
	bool j = false;
	for (int i = 0; i < len_; ++i)
	{
		if (islegal(str_[i]))
		{
			*temp2 = str_[i];
			if (str_[i] != ' ') j = true;

			++temp2;
		}
	}

	if (!j)
	{
		delete temp;
		return 0;
	}

	*temp2 = '\0';
	AString * retval = new AString(temp);
	delete temp;
	return retval;
}

AString* AString::trunc(int val, int back)
{
	if (len() <= val)
		return 0;

	for (int i = val; i > (val - back); --i)
	{
		if (str_[i] == ' ')
		{
			str_[i] = '\0';
			return new AString(&(str_[i+1]));
		}
	}

	AString *temp = new AString(&(str_[val]));
	str_[val] = '\0';
	return temp;
}

int AString::value() const
{
	int place = 0;
	int ret = 0;
	while (str_[place] >= '0' && str_[place] <= '9')
	{
		ret *= 10;
		// Fix integer overflow to negative.
		if (ret < 0) return 0;

		ret += (str_[place++] - '0');
	}
	return ret;
}

std::ostream& operator<<(std::ostream &os, const AString &s)
{
	os << s.str_;
	return os;
}

std::istream& operator>>(std::istream &is, AString &s)
{
	std::string buf;
	is >> buf;

	s.len_ = int(buf.size());
	s.str_ = new char[s.len_ + 1];
	strcpy(s.str_, buf.c_str());

	return is;
}


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
#include "fileio.h"
#include "gameio.h"
#include "astring.h"

static char buf[1024];

static void skipwhite(std::ifstream *f)
{
	if (f->eof())
		return;

	int ch = f->peek();
	while ((ch == ' ') || (ch == '\n') || (ch == '\t') ||
			(ch == '\r') || (ch == '\0'))
	{
		f->get();
		if (f->eof())
			return;

		ch = f->peek();
	}
}

//----------------------------------------------------------------------------
Ainfile::Ainfile()
{
	file = new std::ifstream;
}

Ainfile::~Ainfile()
{
	delete file;
}

void Ainfile::Open(const AString &s)
{
	while (!file->rdbuf()->is_open())
	{
		AString *name = getfilename(s);
		file->open(name->str(), std::ios::in);
		delete name;
	}
}

int Ainfile::OpenByName(const AString &s)
{
	file->open(s.str(), std::ios::in);

	if (!file->rdbuf()->is_open())
		return -1;

	return 0;
}

void Ainfile::Close()
{
	file->close();
}

AString* Ainfile::GetStr()
{
	skipwhite(file);
	return GetStrNoSkip();
}

AString* Ainfile::GetStrNoSkip()
{
	if (file->peek() == -1 || file->eof())
		return 0;

	file->getline(buf, sizeof(buf)-1, '\n');
	return new AString(&buf[0]);
}

int Ainfile::GetInt()
{
	int x = 0;
	*file >> x;
	return x;
}

//----------------------------------------------------------------------------
Aoutfile::Aoutfile()
{
	file = new std::ofstream;
}

Aoutfile::~Aoutfile()
{
	delete file;
}

void Aoutfile::Open(const AString &s)
{
	while (!file->rdbuf()->is_open())
	{
		AString *name = getfilename(s);
		file->open(name->str(), std::ios::out|std::ios::ate);
		delete name;

		// Handle a broke ios::ate implementation on some boxes
		file->seekp(0, std::ios::end);

		if ((int)file->tellp() != 0)
			file->close();
	}
}

int Aoutfile::OpenByName(const AString &s)
{
	file->open(s.str(), std::ios::out|std::ios::ate);
	if (!file->rdbuf()->is_open())
		return -1;

	// Handle a broke ios::ate implementation on some boxes
	file->seekp(0, std::ios::end);

	if ((int)file->tellp() != 0)
	{
		file->close();
		return -1;
	}
	return 0;
}

void Aoutfile::Close()
{
	file->close();
}

void Aoutfile::PutInt(int x)
{
	*file << x << '\n';
}

void Aoutfile::PutStr(const char *s)
{
	*file << s << '\n';
}

void Aoutfile::PutStr(const AString &s)
{
	*file << s << '\n';
}

void Aoutfile::putNoNewline(const char *s)
{
	*file << s;
}

//----------------------------------------------------------------------------
Areport::Areport()
: tabs(0)
{
}

void Areport::Open(const AString &s)
{
	Aoutfile::Open(s);
	tabs = 0;
}

int Areport::OpenByName(const AString &s)
{
	tabs = 0;
	return Aoutfile::OpenByName(s);
}

void Areport::AddTab()
{
	++tabs;
}

void Areport::DropTab()
{
	if (tabs > 0)
		--tabs;
}

void Areport::ClearTab()
{
	tabs = 0;
}

void Areport::PutStr(const AString &s, int comment)
{
	AString temp;
	for (int i = 0; i < tabs; ++i)
		temp += "  ";

	temp += s;
	AString *temp2 = temp.Trunc(70);

	if (comment)
		Aoutfile::putNoNewline(";");

	Aoutfile::PutStr(temp);

	while (temp2)
	{
		temp = "  ";
		for (int i = 0; i < tabs; ++i)
			temp += "  ";

		temp += *temp2;

		delete temp2;
		temp2 = temp.Trunc(70);

		if (comment)
			Aoutfile::putNoNewline(";");

		Aoutfile::PutStr(temp);
	}
}

void Areport::PutNoFormat(const AString &s)
{
	Aoutfile::PutStr(s);
}

void Areport::EndLine()
{
	Aoutfile::PutStr("");
}

//----------------------------------------------------------------------------
Arules::Arules()
: tabs(0)
, wraptab(0)
{
}

void Arules::Open(const AString &s)
{
	Aoutfile::Open(s);
	tabs = 0;
	wraptab = 0;
}

int Arules::OpenByName(const AString &s)
{
	//NOTE: parent method uses ios::ate
	file->open(s.str(), std::ios::out|std::ios::trunc);
	if (!file->rdbuf()->is_open())
		return -1;

	// Handle a broke ios::ate implementation on some boxes
	file->seekp(0, std::ios::end);

	if ((int)file->tellp() != 0)
	{
		file->close();
		return -1;
	}

	tabs = 0;
	wraptab = 0;
	return 0;
}

void Arules::AddTab()
{
	++tabs;
}

void Arules::DropTab()
{
	if (tabs > 0)
		--tabs;
}

void Arules::ClearTab()
{
	tabs = 0;
}

void Arules::AddWrapTab()
{
	++wraptab;
}

void Arules::DropWrapTab()
{
	if (wraptab > 0)
		--wraptab;
}

void Arules::ClearWrapTab()
{
	wraptab = 0;
}

void Arules::PutStr(const AString &s)
{
	AString temp;
	for (int i = 0; i < tabs; ++i)
		temp += "  ";

	temp += s;
	AString *temp2 = temp.Trunc(78, 70);

	Aoutfile::PutStr(temp);

	while (temp2)
	{
		temp = "";
		for (int i = 0; i < tabs; ++i)
			temp += "  ";

		temp += *temp2;

		delete temp2;
		temp2 = temp.Trunc(78, 70);

		Aoutfile::PutStr(temp);
	}
}

void Arules::WrapStr(const AString &s)
{
	AString temp;
	for (int i = 0; i < wraptab; ++i)
		temp += "  ";

	temp += s;
	AString *temp2 = temp.Trunc(70);

	Aoutfile::PutStr(temp);

	while (temp2)
	{
		temp = "  ";
		for (int i = 0; i < wraptab; ++i)
			temp += "  ";

		temp += *temp2;

		delete temp2;
		temp2 = temp.Trunc(70);

		Aoutfile::PutStr(temp);
	}
}

void Arules::PutNoFormat(const AString &s)
{
	Aoutfile::PutStr(s);
}

void Arules::EndLine()
{
	Aoutfile::PutStr("");
}

void Arules::Enclose(int flag, const AString &tag)
{
	if (flag)
	{
		PutStr(AString("<") + tag + ">");
		AddTab();
	}
	else
	{
		DropTab();
		PutStr(AString("</")+ tag + ">");
	}
}

void Arules::TagText(const AString &tag, const AString &text)
{
	Enclose(1, tag);
	PutStr(text);
	Enclose(0, tag);
}

void Arules::ClassTagText(const AString &tag, const AString &cls, const AString &text)
{
	AString temp = tag;
	temp +=  " CLASS=\"";
	temp += cls;
	temp += "\"";
	Enclose(1, temp);
	PutStr(text);
	Enclose(0, tag);
}

void Arules::Paragraph(const AString &text)
{
	Enclose(1, "P");
	PutStr(text);
	Enclose(0, "P");
}

void Arules::CommandExample(const AString &header, const AString &examp)
{
	Paragraph(header);
	Paragraph("");
	Enclose(1, "PRE");
	PutNoFormat(examp);
	Enclose(0, "PRE");
}

AString Arules::Link(const AString &href, const AString &text)
{
	return AString("<A HREF=\"")+href+"\">"+text+"</A>";
}

void Arules::LinkRef(const AString &name)
{
	PutStr(AString("<A NAME=\"")+name+"\"></A>");
}


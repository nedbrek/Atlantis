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
#include "orders.h"
#include "unit.h"
#include "astring.h"

const char *const od[] =
{
	"#atlantis",
	"#end",
	"unit",
	"address",
	"advance",
	"armor",
	"assassinate",
	"attack",
	"autotax",
	"avoid",
	"behind",
	"build",
	"buy",
	"cast",
	"claim",
	"combat",
	"consume",
	"declare",
	"describe",
	"destroy",
	"end",
	"endturn",
	"enter",
	"entertain",
	"evict",
	"exchange",
	"faction",
	"find",
	"forget",
	"form",
	"give",
	"guard",
	"hold",
	"leave",
	"move",
	"name",
	"noaid",
	"nocross",
	"nospoils",
	"option",
	"password",
	"pillage",
	"prepare",
	"produce",
	"promote",
	"quit",
	"restart",
	"reveal",
	"sail",
	"sell",
	"share",
	"show",
	"spoils",
	"steal",
	"study",
	"tax",
	"teach",
	"turn",
	"weapon",
	"withdraw",
	"work",
};

const char* const *OrderStrs = od;

int Parse1Order(AString *token)
{
	for (int i = 0; i < NORDERS; ++i)
		if (*token == OrderStrs[i])
			return i;

	return -1;
}

Order::Order(int o)
: type(o)
{
}

Order::~Order()
{
}

ExchangeOrder::ExchangeOrder()
: Order(O_EXCHANGE)
{
	exchangeStatus = -1;
	target = NULL;
}

ExchangeOrder::~ExchangeOrder()
{
	delete target;
}

TurnOrder::TurnOrder()
: Order(O_TURN)
{
	repeating = 0;
}

TurnOrder::~TurnOrder()
{
}

MoveOrder::MoveOrder(int t)
: Order(t)
{
}

MoveOrder::~MoveOrder()
{
}

ForgetOrder::ForgetOrder()
: Order(O_FORGET)
{
}

ForgetOrder::~ForgetOrder()
{
}

WithdrawOrder::WithdrawOrder()
: Order(O_WITHDRAW)
{
}

WithdrawOrder::~WithdrawOrder()
{
}

GiveOrder::GiveOrder()
: Order(O_GIVE)
{
	target = NULL;
}

GiveOrder::~GiveOrder()
{
	delete target;
}

StudyOrder::StudyOrder()
: Order(O_STUDY)
{
}

StudyOrder::~StudyOrder()
{
}

TeachOrder::TeachOrder()
: Order(O_TEACH)
{
}

TeachOrder::~TeachOrder()
{
}

ProduceOrder::ProduceOrder()
: Order(O_PRODUCE)
{
}

ProduceOrder::~ProduceOrder()
{
}

ProduceQueue::ProduceQueue()
: Order(O_PRODUCE)
{
}

ProduceQueue::~ProduceQueue()
{
}

BuyOrder::BuyOrder()
: Order(O_BUY)
{
}

BuyOrder::~BuyOrder()
{
}

SellOrder::SellOrder()
: Order(O_SELL)
{
}

SellOrder::~SellOrder()
{
}

AttackOrder::AttackOrder()
: Order(O_ATTACK)
{
}

AttackOrder::~AttackOrder()
{
}

BuildOrder::BuildOrder()
: Order(O_BUILD)
{
	target = NULL;
}

BuildOrder::~BuildOrder()
{
	delete target;
}

SailOrder::SailOrder()
: Order(O_SAIL)
{
}

SailOrder::~SailOrder()
{
}

FindOrder::FindOrder()
: Order(O_FIND)
{
}

FindOrder::~FindOrder()
{
}

StealOrder::StealOrder()
: Order(O_STEAL)
{
	target = NULL;
}

StealOrder::~StealOrder()
{
	delete target;
}

AssassinateOrder::AssassinateOrder()
: Order(O_ASSASSINATE)
{
	target = NULL;
}

AssassinateOrder::~AssassinateOrder()
{
	delete target;
}

CastOrder::CastOrder()
: Order(O_CAST)
{
}

CastOrder::~CastOrder()
{
}

CastEnchantOrder::CastEnchantOrder()
: output_item(-1)
{
}

CastMindOrder::CastMindOrder()
{
	id = NULL;
}

CastMindOrder::~CastMindOrder()
{
	delete id;
}

TeleportOrder::TeleportOrder()
{
}

TeleportOrder::~TeleportOrder()
{
}

CastRegionOrder::CastRegionOrder()
{
}

CastRegionOrder::~CastRegionOrder()
{
}

CastIntOrder::CastIntOrder()
{
}

CastIntOrder::~CastIntOrder()
{
}

CastUnitsOrder::CastUnitsOrder()
{
}

CastUnitsOrder::~CastUnitsOrder()
{
}

EvictOrder::EvictOrder()
: Order(O_EVICT)
{
}

EvictOrder::~EvictOrder()
{
}

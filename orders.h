#ifndef ORDERS_CLASS
#define ORDERS_CLASS
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
#include <deque>
class AString;
class UnitId;

//----------------------------------------------------------------------------
enum
{
	O_ATLANTIS,
	O_END,
	O_UNIT,
	O_ADDRESS,
	O_ADVANCE,
	O_ARMOR,
	O_ASSASSINATE,
	O_ATTACK,
	O_AUTOTAX,
	O_AVOID,
	O_BEHIND,
	O_BUILD,
	O_BUY,
	O_CAST,
	O_CLAIM,
	O_COMBAT,
	O_CONSUME,
	O_DECLARE,
	O_DESCRIBE,
	O_DESTROY,
	O_ENDFORM,
	O_ENDTURN,
	O_ENTER,
	O_ENTERTAIN,
	O_EVICT,
	O_EXCHANGE,
	O_FACTION,
	O_FIND,
	O_FORGET,
	O_FORM,
	O_GIVE,
	O_GUARD,
	O_HOLD,
	O_LEAVE,
	O_MOVE,
	O_NAME,
	O_NOAID,
	O_NOCROSS,
	O_NOSPOILS,
	O_OPTION,
	O_PASSWORD,
	O_PILLAGE,
	O_PREPARE,
	O_PRODUCE,
	O_PROMOTE,
	O_QUIT,
	O_RESTART,
	O_REVEAL,
	O_SAIL,
	O_SELL,
	O_SHARE,
	O_SHOW,
	O_SPOILS,
	O_STEAL,
	O_STUDY,
	O_TAX,
	O_TEACH,
	O_TURN,
	O_WEAPON,
	O_WITHDRAW,
	O_WORK,
	NORDERS
};

//----------------------------------------------------------------------------
enum
{
	M_NONE,
	M_WALK,
	M_RIDE,
	M_FLY,
	M_SAIL
};

//----------------------------------------------------------------------------
#define MOVE_IN  98
#define MOVE_OUT 99
// Enter is MOVE_ENTER + num of object
#define MOVE_ENTER 100

//----------------------------------------------------------------------------
extern const char* const *OrderStrs;

//----------------------------------------------------------------------------
int Parse1Order(AString *);

//----------------------------------------------------------------------------
/// One segment of movement
class MoveDir : public AListElem
{
public:
	int dir;
};

//----------------------------------------------------------------------------
/// Base class for all orders
class Order : public AListElem
{
public:
	explicit Order(int t);
	virtual ~Order();

	const int type;
};

/// Move to adjacent hex or enter/leave an object
class MoveOrder : public Order
{
public:
	MoveOrder(int t = O_MOVE);
	~MoveOrder();

	int advancing;
	AList dirs;
};

/// Take funds or items from the faction bank
class WithdrawOrder : public Order
{
public:
	WithdrawOrder();
	~WithdrawOrder();

	int item;
	int amount;
};

/// Transfer items to another unit
class GiveOrder : public Order
{
public:
	GiveOrder();
	~GiveOrder();

	int item;
	int amount; ///< if amount == -1, transfer whole unit, -2 means all of item
	int except;

	UnitId *target;
};

/// Increase skill
class StudyOrder : public Order
{
public:
	StudyOrder();
	~StudyOrder();

	int skill;
	int days;
};

/// Help another unit to study
class TeachOrder : public Order
{
public:
	TeachOrder();
	~TeachOrder();

	AList targets;
};

/// Create items
class ProduceOrder : public Order
{
public:
	ProduceOrder();
	~ProduceOrder();

	int item;
	int skill; ///< -1 for none
	int productivity;
	int limit;
};

/// Produce multiple items
class ProduceQueue : public Order
{
public:
	ProduceQueue();
	~ProduceQueue();

	std::deque<ProduceOrder> orders_;
};

/// Buy an item from a market
class BuyOrder : public Order
{
public:
	BuyOrder();
	~BuyOrder();

	int item;
	int num;
};

/// Sell an item in market
class SellOrder : public Order
{
public:
	SellOrder();
	~SellOrder();

	int item;
	int num;
};

/// Attack a unit
class AttackOrder : public Order
{
public:
	AttackOrder();
	~AttackOrder();

	AList targets;
};

/// Construct an object
class BuildOrder : public Order
{
public:
	BuildOrder();
	~BuildOrder();

	UnitId *target;
};

/// Operate a ship
class SailOrder : public Order
{
public:
	SailOrder();
	~SailOrder();

	AList dirs;
};

/// Look for a player
class FindOrder : public Order
{
public:
	FindOrder();
	~FindOrder();

	int find;
};

/// Steal an item
class StealOrder : public Order
{
public:
	StealOrder();
	~StealOrder();

	UnitId *target;
	int item;
};

/// Try to secretly attack a unit
class AssassinateOrder : public Order
{
public:
	AssassinateOrder();
	~AssassinateOrder();

	UnitId *target;
};

/// Lose a skill
class ForgetOrder : public Order
{
public:
	ForgetOrder();
	~ForgetOrder();

	int skill;
};

/// Atomically swap items
class ExchangeOrder : public Order
{
public:
	ExchangeOrder();
	~ExchangeOrder();

	int giveItem;
	int giveAmount;
	int expectItem;
	int expectAmount;

	int exchangeStatus;

	UnitId *target;
};

/// Delay orders for 1 turn
class TurnOrder : public Order
{
public:
	TurnOrder();
	~TurnOrder();

	int repeating;
	AList turnOrders;
};

/// Cast a spell
class CastOrder : public Order
{
public:
	CastOrder();
	~CastOrder();

	int spell;
	int level;
};

/// Cast enchant (armor or weapon)
class CastEnchantOrder : public CastOrder
{
public:
	CastEnchantOrder();

	int output_item;
};

/// Cast mind reading
class CastMindOrder : public CastOrder
{
public:
	CastMindOrder();
	~CastMindOrder();

	UnitId *id;
};

/// Cast a spell targeting a region
class CastRegionOrder : public CastOrder
{
public:
	CastRegionOrder();
	~CastRegionOrder();

	int xloc, yloc, zloc;
};

/// Cast teleport
class TeleportOrder : public CastRegionOrder
{
public:
	TeleportOrder();
	~TeleportOrder();

	int gate;
	AList units;
};

/// Cast helper
class CastIntOrder : public CastOrder
{
public:
	CastIntOrder();
	~CastIntOrder();

	int target;
};

/// Cast helper
class CastUnitsOrder : public CastOrder
{
public:
	CastUnitsOrder();
	~CastUnitsOrder();

	AList units;
};

/// Force units to leave an object
class EvictOrder : public Order
{
public:
	EvictOrder();
	~EvictOrder();

	AList targets;
};

#endif


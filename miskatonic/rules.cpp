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
// MODIFICATIONS
// Date			Person			Comments
// ----			------			--------
// 2001/Feb/22	Joseph Traub	Moved the Item and such definitions out
//								into gamedata.cpp
//
#include "gamedata.h"
#include "gamedefs.h"

//
// Define the various globals for this game.
//
// If you change any of these, it is incumbent on you, the GM to change
// the html file containing the rules to correctly reflect the changes!
//

static int am[] = { 3, 3, 3, 3, 3, 3, 3 };
int *allowedMages = am;
int allowedMagesSize = sizeof(am) / sizeof(am[0]);

static int aa[] = { 3, 3, 3, 3, 3, 3, 3 };
int *allowedApprentices = aa;
int allowedApprenticesSize = sizeof(aa) / sizeof(aa[0]);

static int aw[] = { 100, 100, 100, 100, 100, 100 };
int *allowedTaxes = aw;
int allowedTaxesSize = sizeof(aw) / sizeof(aw[0]);

static int at[] = { 100, 100, 100, 100, 100, 100 };
int *allowedTrades = at;
int allowedTradesSize = sizeof(at) / sizeof(at[0]);

static GameDefs g = {
	"Miskatonic",				// RULESET_NAME
	MAKE_ATL_VER( 1, 0, 1 ),    // RULESET_VERSION

	2, /* FOOT_SPEED */
	4, /* HORSE_SPEED */
	4, /* SHIP_SPEED */
	6, /* FLY_SPEED */
	8, /* MAX_SPEED */

	10, /* STUDENTS_PER_TEACHER */
	5, /* MAINTENANCE_COST */
	0, /* LEADER_COST */
	1, /* MAINT_COST_PER_HIT */

	5,  /* MAINTAINENCE_MULTIPLIER */
	GameDefs::MULT_NONE, /* MULTIPLIER_USE */

	20, /* STARVE_PERCENT */
	GameDefs::STARVE_NONE, /* SKILL_STARVATION */

	7500, /* START_MONEY */
	5, /* WORK_FRACTION */
	20, /* ENTERTAIN_FRACTION */
	20, /* ENTERTAIN_INCOME */

	50, /* TAX_INCOME */

	10, /* HEALS_PER_MAN */

	75, /* GUARD_REGEN */ /* percent */
	100, /* CITY_GUARD */
	50, /* GUARD_MONEY */
	4000, /* CITY_POP */

	20, /* WMON_FREQUENCY */
	10, /* LAIR_FREQUENCY */

	0, /* FACTION_POINTS */

	100, /* TIMES_REWARD */

	1, // TOWNS_EXIST
	0, // LEADERS_EXIST
	1, // SKILL_LIMIT_NONLEADERS
	1, // MAGE_NONLEADERS
	1, // RACES_EXIST
	1, // GATES_EXIST
	1, // FOOD_ITEMS_EXIST
	1, // CITY_MONSTERS_EXIST
	1, // WANDERING_MONSTERS_EXIST
	1, // LAIR_MONSTERS_EXIST
	0, // WEATHER_EXISTS
	1, // OPEN_ENDED
	0, // NEXUS_EXISTS
	0, // CONQUEST_GAME

	1, // RANDOM_ECONOMY
	1, // VARIABLE_ECONOMY

	2, // CITY_MARKET_NORMAL_AMT
	0, // CITY_MARKET_ADVANCED_AMT
	20, // CITY_MARKET_TRADE_AMT
	0, // CITY_MARKET_MAGIC_AMT
	1,  // MORE_PROFITABLE_TRADE_GOODS

	50,	// BASE_MAN_COST
	0, // LASTORDERS_MAINTAINED_BY_SCRIPTS
	10, // MAX_INACTIVE_TURNS

	0, // EASIER_UNDERWORLD

	1, // DEFAULT_WORK_ORDER

	GameDefs::FACLIM_MAGE_COUNT, // FACTION_LIMIT_TYPE

	GameDefs::WFLIGHT_MUST_LAND,	// FLIGHT_OVER_WATER

	0,   // SAFE_START_CITIES
	500, // AMT_START_CITY_GUARDS
	1,   // START_CITY_GUARDS_PLATE
	3,   // START_CITY_MAGES
	3,   // START_CITY_TACTICS
	1,   // APPRENTICES_EXIST

	"Cykranosh", // WORLD_NAME

	1, // NEXUS_GATE_OUT
	0, // NEXUS_IS_CITY
	1, // NEXUS_NO_EXITS
	1,	// BATTLE_FACTION_INFO
	0,	// ALLOW_WITHDRAW
	2500,	// CITY_RENAME_COST
	1,	// TAX_PILLAGE_MONTH_LONG
	0,	// MULTI_HEX_NEXUS
	1,	// UNDERWORLD_LEVELS
	1,	// UNDERDEEP_LEVELS
	0,	// ABYSS_LEVEL
	80,	// TOWN_PROBABILITY
	35,	// UNDERWORLD_TOWN_PROBABILITY
	75,	// TOWN_SPREAD
	100,	// TOWNS_NOT_ADJACENT
	25,	// LESS_ARCTIC_TOWNS
	1,	// ARCHIPELAGO
	0,	// LAKES_EXIST
	GameDefs::NO_EFFECT, // LAKE_WAGE_EFFECT
	0,	// LAKESIDE_IS_COASTAL
	50,	// ODD_TERRAIN
	0,	// IMPROVED_FARSIGHT
	1,	// GM_REPORT
	0,	// DECAY
	0,	// LIMITED_MAGES_PER_BUILDING
	GameDefs::REPORT_NOTHING, // TRANSIT_REPORT
	0,  //FACTION_SKILLS_REVEAL_RESOURCES
	0,  // MARKETS_SHOW_ADVANCED_ITEMS
	GameDefs::PREPARE_NONE,	// USE_PREPARE_COMMAND
	0,	// MONSTER_ADVANCE_MIN_PERCENT
	25,	// MONSTER_ADVANCE_HOSTILE_PERCENT (% of MON HOSTILE VALUE), DRAGON=50 Hostile
	0,	// HAVE_EMAIL_SPECIAL_COMMANDS
	0,	// START_CITIES_START_UNLIMITED
	1,	// PROPORTIONAL_AMTS_USAGE
	0,  // USE_WEAPON_ARMOR_COMMAND
	3,  // MONSTER_NO_SPOILS
	9,  // MONSTER_SPOILS_RECOVERY
	2,  // MAX_ASSASSIN_FREE_ATTACKS
	1,  // RELEASE_MONSTERS
	1,  // CHECK_MONSTER_CONTROL_MID_TURN
	0,  // DETECT_GATE_NUMBERS
	GameDefs::ARMY_ROUT_HITS_FIGURE,  // ARMY_ROUT
	0,	// FULL_TRUESEEING_BONUS
	0,	// IMPROVED_AMTS
	0,	// FULL_INVIS_ON_SELF
	1,	// MONSTER_BATTLE_REGEN
	GameDefs::TAX_NORMAL | GameDefs::TAX_ANY_MAGE, // WHO_CAN_TAX
	5,	// SKILL_PRACTISE_AMOUNT
	0,	// UPKEEP_MINIMUM_FOOD
	-1,	// UPKEEP_MAXIMUM_FOOD
	30,	// UPKEEP_FOOD_VALUE
	1,	// PREVENT_SAIL_THROUGH
	0,	// ALLOW_TRIVIAL_PORTAGE
};

GameDefs * Globals = &g;

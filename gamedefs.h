#ifndef GAME_DEFS
#define GAME_DEFS
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
#include "helper.h"

/// Movement Directions
enum
{
	D_NORTH,
	D_NORTHEAST,
	D_SOUTHEAST,
	D_SOUTH,
	D_SOUTHWEST,
	D_NORTHWEST,
	NDIRS
};

extern const char* *DirectionStrs; ///< full names for directions
extern const char* *DirectionAbrs; ///< short names for directions

extern const char* *MonthNames; ///< normal months (0-11)

extern const char* *SeasonNames; ///< four seasons (clear, winter, monsoon, blizzard)

// Data for faction points
extern int *allowedMages;
extern int allowedMagesSize;
extern int *allowedApprentices;
extern int allowedApprenticesSize;
extern int *allowedTaxes;
extern int allowedTaxesSize;
extern int *allowedTrades;
extern int allowedTradesSize;

/// Global settings (set at compile time in rules.cpp)
class GameDefs
{
public:
	const char *RULESET_NAME; ///< name of the rules
	ATL_VER RULESET_VERSION; ///< numeric version

	int FOOT_SPEED;  ///< speed on land (without a mount)
	int HORSE_SPEED; ///< speed on land (with any mount)
	int SHIP_SPEED;  ///< speed on water (includes swimming)
	int FLY_SPEED;   ///< speed when flying
	int MAX_SPEED;   ///< speed which cannot be exceeded

	int STUDENTS_PER_TEACHER; ///< number of students per man in the teaching unit

	//---maintenance
	int MAINTENANCE_COST; ///< cost in silver per man per month
	int LEADER_COST; ///< base maintenance cost of leaders

	int MAINT_COST_PER_HIT;  ///< pay MAINTENANCE_COST per hit per man per month

	// Settings for using skill level multiplier (units pay X per level of skill they have per man)
	int MAINTENANCE_MULTIPLIER; ///< cost (in silver) per level of skill

	enum
	{
		MULT_NONE,    ///< no one pays extra per skill level
		MULT_MAGES,   ///< mages pay extra
		MULT_LEADERS, ///< leaders pay extra
		MULT_ALL      ///< everyone pays
	};
	int MULTIPLIER_USE; ///< who uses skill multiplier

	//---starvation (failure to pay maintenance)
	int STARVE_PERCENT; ///< chance for a man to be lost

	enum
	{
		STARVE_NONE,    ///< disable skill starvation
		STARVE_MAGES,   ///< apply it to mage units
		STARVE_LEADERS, ///< apply it to leaders
		STARVE_ALL      ///< apply it to all
	};
	int SKILL_STARVATION; ///< who loses skill levels instead of dying

	//---other settings
	int START_MONEY; ///< amount of silver given to new factions

	int WORK_FRACTION; ///< factor in determining max amount of work in the region
	int ENTERTAIN_FRACTION; ///< factor in determining max amount of entertainment in the region

	int ENTERTAIN_INCOME; ///< amount of silver per entertainer
	int TAX_INCOME; ///< amount of silver per taxer

	int HEALS_PER_MAN; ///< number of men healed by one healer

	int GUARD_REGEN; ///< percent chance per turn to regenerate 10% of the guards
	int CITY_GUARD; ///< number of men in city guard units
	int GUARD_MONEY; ///< amount of silver per guard
	int CITY_POP; ///< max pop (city) - town is 1/2, village is 1/4

	int WMON_FREQUENCY; ///< percent chance of wandering monsters appearing
	int LAIR_FREQUENCY; ///< percent chance of a lair appearing

	int FACTION_POINTS; ///< number of faction points (set max tax and production regions, and mages)

	int TIMES_REWARD; ///< amount of silver given to the faction winning the times reward

	int TOWNS_EXIST; ///< generate towns (includes cities and villages)
	int LEADERS_EXIST; ///< allow leaders to be recruited
	int SKILL_LIMIT_NONLEADERS; ///< apply racial skill limits to non-leaders
	int MAGE_NONLEADERS; ///< allow non-leaders to study magic
	int RACES_EXIST; ///< allow non-leaders to be recruited
	int GATES_EXIST; ///< generate teleportation gates
	int FOOD_ITEMS_EXIST; ///< allow grain and livestock to be produced
	int CITY_MONSTERS_EXIST; ///< actually city guards
	int WANDERING_MONSTERS_EXIST; ///< create wandering monsters
	int LAIR_MONSTERS_EXIST; ///< create lairs
	int WEATHER_EXISTS; ///< allow weather to vary
	int OPEN_ENDED; ///< 0: check victory conditions, 1: game ends when players quit
	int NEXUS_EXISTS; ///< start players in the nexus
	int CONQUEST_GAME; ///< special combat oriented rules

	//---some economic controls
	int RANDOM_ECONOMY; ///< economic settings in towns are randomized
	int VARIABLE_ECONOMY; ///< economic settings are altered each turn

	int CITY_MARKET_NORMAL_AMT; ///< base number of items for sale
	int CITY_MARKET_ADVANCED_AMT; ///< base number of items for sale (for those marked advanced)
	int CITY_MARKET_TRADE_AMT; ///< base number of items that are desired to buy in town
	int CITY_MARKET_MAGIC_AMT; ///< base number of magic items (not set NOMARKET) for sale
	int MORE_PROFITABLE_TRADE_GOODS; ///< raise prices on trade goods

	int BASE_MAN_COST; ///< base cost of a man (racial cost scales this up)

	//---other settings
	int LASTORDERS_MAINTAINED_BY_SCRIPTS; ///< lastorders values maintained by external scripts
	int MAX_INACTIVE_TURNS; ///< number of turns to allow a faction to be inactive (-1 disables)

	int EASIER_UNDERWORLD; ///< give explicit information about z level

	int DEFAULT_WORK_ORDER; ///< units with no orders 'work' by default

	enum
	{
		FACLIM_MAGE_COUNT,    ///< only mages (and apprentices) are limited
		FACLIM_FACTION_TYPES, ///< limit taxes and production (and mages/apprentices)
		FACLIM_UNLIMITED      ///< no limits
	};
	int FACTION_LIMIT_TYPE; ///< type of faction limits that are in effect in this game

	enum
	{
		WFLIGHT_NONE,      ///< no flying over water
		WFLIGHT_MUST_LAND, ///< flying units must reach land by end of turn
		WFLIGHT_UNLIMITED  ///< flying units can end turn over water
	};
	int FLIGHT_OVER_WATER; ///< how to handle flight over water

	int SAFE_START_CITIES; ///< guards in starting cities get amulets of invulnerability
	int AMT_START_CITY_GUARDS; ///< number of guards in starting cities
	int START_CITY_GUARDS_PLATE; ///< guards in starting cities get plate armor
	int START_CITY_MAGES; ///< fire skill level for mages in starting cities (0 for no mages)
	int START_CITY_TACTICS; ///< tactician skill for guards in starting cities (0 for none)

	int APPRENTICES_EXIST; ///< enable apprentices (users of magic items)

	const char *WORLD_NAME; ///< name of the world

	int NEXUS_GATE_OUT; ///< can you cast gate from the nexus
	int NEXUS_IS_CITY; ///< make the nexus a city
	int NEXUS_NO_EXITS; ///< don't connect the nexus to starting cities

	int BATTLE_FACTION_INFO; ///< battle reports show faction

	int ALLOW_WITHDRAW; ///< allow units to withdraw items (not just silver)

	int CITY_RENAME_COST; ///< cost is the city size (1, 2, 3) * this value (0 to disable)

	int TAX_PILLAGE_MONTH_LONG; ///< make taxing and pillaging month-long actions

	int MULTI_HEX_NEXUS; ///< enable multi-hex nexus

	int UNDERWORLD_LEVELS; ///< number of levels of underworld (1/4 size)
	int UNDERDEEP_LEVELS; ///< number of levels of underdeep (1/16 size)
	int ABYSS_LEVEL; ///< 0: no abyss, else: one level of abyss (1/64 size)

	int TOWN_PROBABILITY; ///< town probability: 100 = default
	int UNDERWORLD_TOWN_PROBABILITY; ///< adjusts TOWN_PROBABILITY for underworld: 100 = default

	// Default = 0. For high values the town probability will need to be adjusted
	// downwards to get comparable number of towns
	int TOWN_SPREAD; ///< lessen effects of economy on chance for towns

	int TOWNS_NOT_ADJACENT; ///< discourage adjacent towns (except near starting cities, 100 disallows)
	int LESS_ARCTIC_TOWNS; ///< make settlements near the arctic less likely and smaller. suggested: 0-5

	/// Enable Archipelago Creation
	// = chance of creating archipelagos vs continents
	// (note that archipelagos are smaller so that
	//  the overall contribution of archipelagos to
	//  land mass will be much lower than this percentage)
	// Setting ARCHIPELAGO means that smaller inland seas will be
	// converted into continent mass.
	// suggested value: 10, 25, 50+ it's really a matter of taste
	int ARCHIPELAGO;

	/// Chance for Lake Creation
	// Setting LAKES_EXIST means that smaller inland seas will be
	// converted into continent mass - this is the chance that
	// such regions will end up as lakes.
	// suggested value: around 12.
	int LAKES_EXIST;

	// Lakes will add one to adjacent regions wages if set
	enum
	{
		NO_EFFECT = 0x00,
		ALL = 0x01,
		TOWNS = 0x02,
		NONPLAINS = 0x04,
		NONPLAINS_TOWNS_ONLY=0x08,
		DESERT_ONLY = 0x10
	};
	int LAKE_WAGE_EFFECT; ///< effect on surrounding wages

	int LAKESIDE_IS_COASTAL; ///< count lakeside regions as coastal for all purposes (races and such)

	int ODD_TERRAIN; ///< chance (x 0.1%) for single-hex terrain oddities. suggested: 5-40

	int IMPROVED_FARSIGHT; ///< farsight makes use of a mage's other skills

	int GM_REPORT; ///< generate report.1 every turn

	int DECAY; ///< objects decay according to the parameters in the object definition table

	int LIMITED_MAGES_PER_BUILDING; ///< limit number of mages which can study inside of certain buildings

	// Transit report options
	enum
	{
		REPORT_NOTHING = 0x0000,
		// Various things which can be shown
		REPORT_SHOW_PEASANTS = 0x0001,
		REPORT_SHOW_REGION_MONEY = 0x0002,
		REPORT_SHOW_WAGES = 0x0004,
		REPORT_SHOW_MARKETS = 0x0008,
		REPORT_SHOW_RESOURCES = 0x0010,
		REPORT_SHOW_ENTERTAINMENT = 0x0020,
		// Collection of the the above
		REPORT_SHOW_ECONOMY = (REPORT_SHOW_PEASANTS |
		                  REPORT_SHOW_REGION_MONEY |
		                  REPORT_SHOW_WAGES |
		                  REPORT_SHOW_MARKETS |
		                  REPORT_SHOW_RESOURCES |
		                  REPORT_SHOW_ENTERTAINMENT),
		// Which type of exits to show
		REPORT_SHOW_USED_EXITS = 0x0040,
		REPORT_SHOW_ALL_EXITS = 0x0080,
		// Which types of units to show
		REPORT_SHOW_GUARDS = 0x0100,
		REPORT_SHOW_INDOOR_UNITS = 0x0200,
		REPORT_SHOW_OUTDOOR_UNITS = 0x0400,
		// Collection of the above
		REPORT_SHOW_UNITS = (REPORT_SHOW_GUARDS | REPORT_SHOW_INDOOR_UNITS |
		                     REPORT_SHOW_OUTDOOR_UNITS),
		// Various types of buildings
		REPORT_SHOW_BUILDINGS = 0x0800,
		REPORT_SHOW_ROADS = 0x1000,
		REPORT_SHOW_SHIPS = 0x2000,
		// Collection of the above
		REPORT_SHOW_STRUCTURES = (REPORT_SHOW_BUILDINGS |
		                    REPORT_SHOW_ROADS |
		                    REPORT_SHOW_SHIPS),
		// Should the unit get to use their advanced skills?
		REPORT_USE_UNIT_SKILLS = 0x8000,

		// Some common collections
		REPORT_SHOW_REGION = (REPORT_SHOW_ECONOMY | REPORT_SHOW_ALL_EXITS),
		REPORT_SHOW_EVERYTHING = (REPORT_SHOW_REGION |
		                    REPORT_SHOW_UNITS |
		                    REPORT_SHOW_STRUCTURES)
	};
	int TRANSIT_REPORT; ///< information shown to a unit just passing through a hex

	int FACTION_SKILLS_REVEAL_RESOURCES;

	int MARKETS_SHOW_ADVANCED_ITEMS; ///< show advanced items in markets at all times

	enum
	{
		PREPARE_NONE = 0,   ///< PREPARE disabled
		PREPARE_NORMAL = 1, ///< PREPARE enabled
		PREPARE_STRICT = 2  ///< PREPARE must be used (no automatic selection)
	};
	int USE_PREPARE_COMMAND; ///< treatment of PREPARE command

	// Monsters have the option of occasionally advancing instead of just using
	// move. There are two values which control this.
	//
	// If you don't want monsters to EVER advance, use 0 for both.
	// If you want a flat percent, use MIN_PERCENT and set HOSTILE_PERCENT
	// to 0.  If you only want it based on the HOSTILE value, set MIN_PERCENT
	// to 0 and HOSTILE_PERCENT to what you want
	int MONSTER_ADVANCE_MIN_PERCENT; ///< minimum amount which monsters should advance
	int MONSTER_ADVANCE_HOSTILE_PERCENT; ///< percent of hostile rating used to determine if they advance

	/// Set this to 1 if your scripts can handle the following commands
	/// #create, #resend, #times, #rumor, #remind, #email
	int HAVE_EMAIL_SPECIAL_COMMANDS;

	int START_CITIES_START_UNLIMITED; ///< starting cities begin with unlimited markets at world generation time

	/// If enabled, then when a unit has more men than Amulets of True Seeing,
	/// and it is targetted by an assassin, then there is a chance that the
	/// target picked will be one of the men carrying the amulet and the
	/// attempt will be foiled. If not enabled, the attempt always succeeds
	int PROPORTIONAL_AMTS_USAGE;

	/// If enabled, then the ARMOR and WEAPON commands for selecting armor/weapon
	/// priorities for a unit are enabled. If a preferred weapon or armor
	/// isn't available, then the code will fall back to the internal list
	int USE_WEAPON_ARMOR_COMMAND;

	int WMONSTER_SPOILS_RECOVERY; ///< apply spoils recovery to wandering monsters
	int MONSTER_NO_SPOILS; ///< >0: disable spoils from released monsters for that many months

	// (has no effect unles MONSTER_NO_SPOILS is also set)
	int MONSTER_SPOILS_RECOVERY; ///< time in months over which monster spoils are slowly regained (0 disables)

	int MAX_ASSASSIN_FREE_ATTACKS; ///< limit number of attacks assassin gets in free round

	int RELEASE_MONSTERS; ///< allow 'GIVE 0 <num> <monster>' for summoned monsters (dragons, wolves, eagles currently)

	int CHECK_MONSTER_CONTROL_MID_TURN; ///< check for demons breaking free and undead decaying before movement, rather than at the end of the turn. This prevents 'balrog missiles'

	int DETECT_GATE_NUMBERS; ///< should 'CAST GATE DETECT' show actual gate numbers

	int LINEAR_COMBAT;  ///< should combat be based on linear combat scale (or the default power of 2)

	enum
	{
		ARMY_ROUT_FIGURES = 0, ///< rout if half of the total figures die. All figures are treated equally

		/// Rout if half the total hits die, all hits are counted independently.
		/// This means you could rout even if none of your men are dead but
		/// you have just taken a lot of hits and are getting clobbered
		ARMY_ROUT_HITS_INDIVIDUAL,

		/// Rout if half of the total hits die, but figures hits only stop
		/// counting toward the total when the figure is fully dead
		ARMY_ROUT_HITS_FIGURE
	};
	int ARMY_ROUT; ///< panic based on number of hits lost rather than the number of figures lost

	int FULL_TRUESEEING_BONUS; ///< If 0 then mages get TRUE_SEEING skill/2 rounded up as a bonus to observation

	int IMPROVED_AMTS; ///< If 1 then an AMTS will give 3 bonus to OBSE rather than 2

	/// If 1 then mages get INVISIBILITY skill added to stealth.
	/// recommended only if you also set FULL_TRUESEEING_BONUS
	int FULL_INVIS_ON_SELF;

	int MONSTER_BATTLE_REGEN; ///< monsters regenerate during battle

	enum
	{
		TAX_ANYONE = 0x00001,
		TAX_COMBAT_SKILL = 0x00002,
		TAX_BOW_SKILL = 0x00004,
		TAX_RIDING_SKILL = 0x00008,
		TAX_USABLE_WEAPON = 0x00010, // no requirement or has required skill
		TAX_ANY_WEAPON = 0x00020, // Pay up lest I beat thee with my longbow!
		TAX_HORSE = 0x00040, // someone who HAS a horse, not the horse itself
		// Usually a weapon that requires no skill, plus COMB, but also
		// LANC (which requires RIDI) and RIDI
		TAX_MELEE_WEAPON_AND_MATCHING_SKILL = 0x00080,
		TAX_BOW_SKILL_AND_MATCHING_WEAPON = 0x00100,
		TAX_HORSE_AND_RIDING_SKILL = 0x00200,
		// Probably more petty theft than tax, but then there are those
		// who argue that taxation IS theft ;->
		TAX_STEALTH_SKILL = 0x00400,
		// Should mages be able to tax?  I'd give my tax to someone who
		// was aiming the black wind at me...
		TAX_MAGE_DAMAGE = 0x00800,
		TAX_MAGE_FEAR = 0x01000, // or able to flatten my barn?
		TAX_MAGE_OTHER = 0x02000, // or able to create magical armour?
		TAX_ANY_MAGE = 0x04000, // or who just has an impressive pointy hat?
		// Wasn't sure whether mages should be judged on whether they
		// know the spell, or their current combat spell (he's ordered
		// to cast shield in combat, but still knows how to fireball
		// unruly peasants).  Setting this uses only the current
		// spell set with the COMBAT order.
		TAX_MAGE_COMBAT_SPELL = 0x08000,
		TAX_BATTLE_ITEM = 0x10000, // Has a staff of lightning?
		TAX_USABLE_BATTLE_ITEM = 0x20000,
		TAX_CREATURES = 0x40000, // Do magical critters help to tax?
		TAX_ILLUSIONS = 0x80000, // What if they're not really there?

		// Abbreviation for "the usual"
		TAX_NORMAL = TAX_COMBAT_SKILL | TAX_USABLE_WEAPON
	};
	int WHO_CAN_TAX; ///< who is able to tax

	int SKILL_PRACTISE_AMOUNT; ///< amount of skill improvement when a skill is used

	//---Options on using food for upkeep. Note that all these values are in silver equivalents!
	int UPKEEP_MINIMUM_FOOD; ///< amount of actual food required for upkeep

	/// MAXIMUM_FOOD is the maximum contribution of food to upkeep
	/// per man, and therefore CAN be lower than MINIMUM_FOOD
	/// (although this seems a silly thing to do).
	/// A negative value for this def indicates no maximum
	int UPKEEP_MAXIMUM_FOOD;
	int UPKEEP_FOOD_VALUE; ///< conversion factor from food to silver

	/// prevent ships from sailing through single hex landmasses during the same turn
	/// (does not prevent them from stopping in a single hex one turn and sailing on through the next turn)
	int PREVENT_SAIL_THROUGH;

	/// If we are preventing sail through, should we also prevent the 'easy portage' that the above allows by default?
	int ALLOW_TRIVIAL_PORTAGE;
};

extern GameDefs *Globals;

#endif

#ifndef GAME_CLASS
#define GAME_CLASS
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
#include "aregion.h"

#define CURRENT_ATL_VER MAKE_ATL_VER( 4, 2, 82 )

class Aorders;
class ExchangeOrder;
class Faction;
class GiveOrder;
class Order;
class OrdersCheck;
class ProduceOrder;
class WithdrawOrder;

/// All the state for running the game
class Game
{
public:
	/// constructor
	Game();

	/// destructor
	~Game();

	// used in main
	void ModifyTablesPerRuleset(); // enable/disable parts of the data tables
	int NewGame(int seed);
	int OpenGame();
	int RunGame();
	int EditGame(int *pSaveGame);
	void DummyGame();
	int DoOrdersCheck(const AString &strOrders, const AString &strCheck);
	int SaveGame();
	int WritePlayers();
	int ViewMap(const AString &, const AString &);
	void UnitFactionMap();
	int GenRules(const AString &, const AString &, const AString &);
	void ViewFactions(); // not used

	///@return faction with id 'n'
	Faction* getFaction(int n);

	// Setup the array of units
	void SetupUnitSeq();
	void SetupUnitNums();

	// Get a unit by its number
	Unit* GetUnit(int num);

	int currentMonth() const { return month; }
	int currentYear() const { return year; }
	int TurnNumber();

	// Get average maintenance per man for use in economy
	static
	int GetAvgMaintPerMan();

	// Faction limit functions
	// Game specific and found in extra.cpp
	// They may return -1 to indicate no limit
	int AllowedMages(Faction *pFac);
	int AllowedApprentices(Faction *pFact);
	int AllowedTaxes(Faction *pFac);
	int AllowedTrades(Faction *pFac);

	ARegionList& getRegions() { return regions; }
	const ARegionList& getRegions() const { return regions; }

private: // methods
	// Game editing functions
	ARegion* EditGameFindRegion();
	void EditGameFindUnit();
	void EditGameRegion(ARegion *pReg);
	void EditGameUnit(Unit *pUnit);
	void EditGameUnitItems(Unit *pUnit);
	void EditGameUnitSkills(Unit *pUnit);
	void EditGameUnitMove(Unit *pUnit);

	void CreateOceanLairs();
	void FixBoatNums(); // Fix broken boat numbers
	void FixGateNums(); // Fix broken/missing gates

	int ReadPlayers();
	int ReadPlayersLine(AString *pToken, AString *pLine, Faction *pFac, int newPlayer);

	// Handle special gm unit modification functions
	Unit* ParseGMUnit(AString *tag, Faction *pFac);

	// extra set up for faction. (in extra.cpp)
	int SetupFaction(Faction *pFac);

	// get a new unit, with its number assigned
	Unit* GetNewUnit(Faction *fac, int an = 0);

	void PreProcessTurn();
	void ReadOrders();
	void DefaultWorkOrder();
	void RunOrders();
	void ClearOrders(Faction *);
	void MakeFactionReportLists();
	void CountAllMages();
	void CountAllApprentices();
	void WriteReport();
	void DeleteDeadFactions();
	Faction* AddFaction(int setup);

	// Standard creation functions
	void CreateCityMons();
	void CreateWMons();
	void CreateLMons();
	void CreateVMons();

	// Game-specific creation functions (see world.cpp)
	void CreateWorld();
	void CreateNPCFactions();
	void CreateCityMon(ARegion *pReg, int percent, int needmage);
	int MakeWMon(ARegion *pReg);
	void MakeLMon(Object *pObj);

	void WriteSurfaceMap(Aoutfile *f, ARegionArray *pArr, int type);
	void WriteUnderworldMap(Aoutfile *f, ARegionArray *pArr, int type);
	char GetRChar(ARegion *r);
	AString GetXtraMap(ARegion *, int);

	// Functions to do upgrades to the ruleset -- should be in extras.cpp
	int UpgradeMajorVersion(int savedVersion);
	int UpgradeMinorVersion(int savedVersion);
	int UpgradePatchLevel(int savedVersion);

	// Functions to allow enabling/disabling parts of the data tables
	void EnableSkill(int sk); // Enabled a disabled skill
	void DisableSkill(int sk);  // Prevents skill being studied or used

	void EnableItem(int it); // Enables a disabled item
	void DisableItem(int it); // Prevents item being generated/produced

	void EnableObject(int ob); // Enables a disabled object
	void DisableObject(int ob); // Prevents object being built
	void ModifyObjectFlags(int ob, int flags);
	void ModifyObjectConstruction(int ob, int it, int num, int sk, int lev);
	void ModifyObjectManpower(int ob, int prot, int cap, int sail, int mages);

	void ClearTerrainRaces(int t);
	void ModifyTerrainRace(int t, int i, int r);
	void ModifyTerrainCoastRace(int t, int i, int r);
	void ClearTerrainItems(int t);
	void ModifyTerrainItems(int t, int i, int p, int c, int a);
	void ModifyTerrainEconomy(int t, int pop, int wages, int econ, int move);

	void ModifyRangeFlags(int range, int flags);

	// Parsing functions
	void ParseError(OrdersCheck *pCheck, Unit *pUnit, Faction *pFac, const AString &strError);
	int ParseDir(AString * token);

	void ParseOrders(int faction, Aorders *ordersFile, OrdersCheck *pCheck);
	Order* ProcessOrder(int orderNum, Unit *unit, AString *order, OrdersCheck *pCheck, int at_value);
	void ProcessMoveOrder(Unit *,AString *, OrdersCheck *pCheck);
	void ProcessAdvanceOrder(Unit *,AString *, OrdersCheck *pCheck);
	Unit *ProcessFormOrder(Unit *former, AString *order, OrdersCheck *pCheck);
	void ProcessAddressOrder(Unit *,AString *, OrdersCheck *pCheck);
	void ProcessAvoidOrder(Unit *,AString *, OrdersCheck *pCheck);
	void ProcessGuardOrder(Unit *,AString *, OrdersCheck *pCheck);
	void ProcessNameOrder(Unit *,AString *, OrdersCheck *pCheck);
	void ProcessDescribeOrder(Unit *,AString *, OrdersCheck *pCheck);
	void ProcessBehindOrder(Unit *,AString *, OrdersCheck *pCheck);
	int parseExcept(const int item, const int amt, AString *o, Unit *unit, OrdersCheck *pCheck);
	void ProcessGiveOrder(Unit *,AString *, OrdersCheck *pCheck);
	void ProcessWithdrawOrder(Unit *,AString *, OrdersCheck *pCheck);
	void ProcessDeclareOrder(Faction *,AString *, OrdersCheck *pCheck);
	void ProcessStudyOrder(Unit *,AString *, OrdersCheck *pCheck);
	Order* ProcessTeachOrder(Unit *,AString *, OrdersCheck *pCheck, int at_value);
	void ProcessWorkOrder(Unit *, OrdersCheck *pCheck);
	void ProcessProduceOrder(Unit *,AString *, OrdersCheck *pCheck);
	void ProcessBuyOrder(Unit *, AString *, OrdersCheck *pCheck);
	void ProcessSellOrder(Unit *,AString *, OrdersCheck *pCheck);
	void ProcessAttackOrder(Unit *,AString *, OrdersCheck *pCheck);
	void ProcessBuildOrder(Unit *, AString *, OrdersCheck *pCheck);
	void ProcessSailOrder(Unit *,AString *, OrdersCheck *pCheck);
	void ProcessEnterOrder(Unit *,AString *, OrdersCheck *pCheck);
	void ProcessLeaveOrder(Unit *, OrdersCheck *pCheck);
	void ProcessPromoteOrder(Unit *,AString *, OrdersCheck *pCheck);
	void ProcessEvictOrder(Unit *,AString *, OrdersCheck *pCheck);
	void ProcessTaxOrder(Unit *, OrdersCheck *pCheck);
	void ProcessPillageOrder(Unit *, OrdersCheck *pCheck);
	void ProcessConsumeOrder(Unit *, AString *, OrdersCheck *pCheck);
	void ProcessRevealOrder(Unit *,AString *, OrdersCheck *pCheck);
	void ProcessFindOrder(Unit *,AString *, OrdersCheck *pCheck);
	void ProcessDestroyOrder(Unit *, OrdersCheck *pCheck);
	void ProcessQuitOrder(Unit *,AString *, OrdersCheck *pCheck);
	void ProcessRestartOrder(Unit *,AString *, OrdersCheck *pCheck);
	void ProcessAssassinateOrder(Unit *,AString *, OrdersCheck *pCheck);
	void ProcessStealOrder(Unit *,AString *, OrdersCheck *pCheck);
	void ProcessFactionOrder(Unit *,AString *, OrdersCheck *pCheck);
	void ProcessClaimOrder(Unit *,AString *, OrdersCheck *pCheck);
	void ProcessCombatOrder(Unit *,AString *, OrdersCheck *pCheck);
	void ProcessPrepareOrder(Unit *, AString *, OrdersCheck *pCheck);
	void ProcessWeaponOrder(Unit *u, AString *o, OrdersCheck *pCheck);
	void ProcessArmorOrder(Unit *u, AString *o, OrdersCheck *pCheck);
	void ProcessCastOrder(Unit *,AString *, OrdersCheck *pCheck);
	void ProcessEnchantSpell(Unit *u, AString *o, int spell, OrdersCheck *pCheck);
	void ProcessEntertainOrder(Unit *, OrdersCheck *pCheck);
	void ProcessForgetOrder(Unit *,AString *, OrdersCheck *pCheck);
	void ProcessReshowOrder(Unit *,AString *, OrdersCheck *pCheck);
	void ProcessHoldOrder(Unit *,AString *, OrdersCheck *pCheck);
	void ProcessNoaidOrder(Unit *,AString *, OrdersCheck *pCheck);
	void ProcessNocrossOrder(Unit *, AString *, OrdersCheck *pCheck);
	void ProcessNospoilsOrder(Unit *, AString *, OrdersCheck *pCheck);
	void ProcessSpoilsOrder(Unit *, AString *, OrdersCheck *pCheck);
	void ProcessFlagOrder(unsigned flag, Unit *,AString *, OrdersCheck *pCheck);
	void ProcessOptionOrder(Unit *,AString *, OrdersCheck *pCheck);
	void ProcessPasswordOrder(Unit *, AString *, OrdersCheck *pCheck);
	void ProcessExchangeOrder(Unit *, AString *, OrdersCheck *pCheck);
	AString *ProcessTurnOrder(Unit *, Aorders *, OrdersCheck *pCheck, int);

	void RemoveInactiveFactions();

	// Game running functions

	// This can be called by parse functions
	int CountMages(Faction *);
	int CountApprentices(Faction *);
	void FindDeadFactions();
	void DeleteEmptyUnits();
	void DeleteEmptyInRegion(ARegion *);
	void EmptyHell();
	void DoGuard1Orders();
	void DoGiveOrders();
	void DoWithdrawOrders();

	void DoExchangeOrders();
	void DoExchangeOrder(ARegion *, Unit *, ExchangeOrder *);

	// Faction limit functions
	int TaxCheck(ARegion *pReg, Faction *pFac);
	int TradeCheck(ARegion *pReg, Faction *pFac);

	// returns 0 normally, or 1 if no more GIVE orders should be allowed
	int DoGiveOrder(ARegion *, Unit *, GiveOrder *);

	// returns 0 normally, or 1 if no more WITHDRAW orders should be allowed
	int DoWithdrawOrder(ARegion *, Unit *, WithdrawOrder *);

	// These are game specific, and can be found in extra.cpp
	void CheckUnitMaintenance(int consume);
	void CheckFactionMaintenance(int consume);
	void CheckAllyMaintenance();

	// Similar to the above, but for minimum food requirements
	void CheckUnitHunger();
	void CheckFactionHunger();
	void CheckAllyHunger();

	void CheckUnitMaintenanceItem(int item, int value, int consume);
	void CheckFactionMaintenanceItem(int item, int value, int consume);
	void CheckAllyMaintenanceItem(int item, int value);

	// Hunger again
	void CheckUnitHungerItem(int item, int value);
	void CheckFactionHungerItem(int item, int value);
	void CheckAllyHungerItem(int item, int value);

	void AssessMaintenance();

	void GrowWMons(int);
	void GrowLMons(int);
	void GrowVMons();
	void PostProcessUnit(ARegion *,Unit *);
	void MidProcessUnit(ARegion *, Unit *);

	// Mid and PostProcessUnitExtra can be used to provide game-specific
	// unit processing at the approrpriate times
	void MidProcessUnitExtra(ARegion *, Unit *);
	void MidProcessTurn();
	void PostProcessUnitExtra(ARegion *,Unit *);
	void PostProcessTurn();

	// Handle escaped monster check
	void MonsterCheck(ARegion *r, Unit *u);

	// CheckVictory is used to see if the game is over
	Faction* CheckVictory();

	void EndGame(Faction *pVictor);

	void RunBuyOrders();
	void DoBuy(ARegion *,Market *);
	int GetBuyAmount(ARegion *,Market *);
	void RunSellOrders();
	void DoSell(ARegion *,Market *);
	int GetSellAmount(ARegion *,Market *);
	void DoAttackOrders();
	void CheckWMonAttack(ARegion *,Unit *);
	Unit *GetWMonTar(ARegion *,int,Unit *);
	int CountWMonTars(ARegion *,Unit *);
	void AttemptAttack(ARegion *,Unit *,Unit *,int,int=0);
	void DoAutoAttacks();
	void DoAutoAttacksRegion(ARegion *);
	void DoAdvanceAttack(ARegion *,Unit *);
	void DoAutoAttack(ARegion *,Unit *);
	void DoAdvanceAttacks(AList *);
	void DoAutoAttackOn(ARegion *,Unit *);
	void RemoveEmptyObjects();
	void RunEnterOrders();
	void Do1EnterOrder(ARegion *,Object *,Unit *);
	void RunPromoteOrders();
	void Do1PromoteOrder(Object *,Unit *);
	void Do1EvictOrder(Object *,Unit *);
	void RunPillageOrders();
	int CountPillagers(ARegion *);
	void ClearPillagers(ARegion *);
	void RunPillageRegion(ARegion *);
	void RunTaxOrders();
	void RunTaxRegion(ARegion *);
	int CountTaxers(ARegion *);
	void RunFindOrders();
	void RunFindUnit(Unit *);
	void RunDestroyOrders();
	void Do1Destroy(ARegion *,Object *,Unit *);
	void RunQuitOrders();
	void RunForgetOrders();
	void Do1Quit(Faction *);
	void SinkUncrewedShips();
	void DrownUnits();
	void RunStealOrders();
	AList* CanSeeSteal(ARegion *,Unit *);
	void Do1Steal(ARegion *,Object *,Unit *);
	void Do1Assassinate(ARegion *,Object *,Unit *);
	void AdjustCityMons(ARegion *pReg);
	void AdjustCityMon(ARegion *pReg, Unit *u);

	// Month long orders
	void RunMoveOrders();
	Location* DoAMoveOrder(Unit *, ARegion *, Object *);
	void DoMoveEnter(Unit *, ARegion *, Object **);
	void RunMonthOrders();
	void RunStudyOrders(ARegion *);
	void Do1StudyOrder(Unit *,Object *);
	void RunTeachOrders();
	void Do1TeachOrder(ARegion *,Unit *);
	void RunProduceOrders(ARegion *);
	int ValidProd(Unit *,ARegion *,Production *);
	int FindAttemptedProd(ARegion *,Production *);
	void RunAProduction(ARegion *,Production *);
	bool RunUnitProduce(ARegion *,Unit *, ProduceOrder *o, struct ProduceIntermediates *pi);
	void Run1BuildOrder(ARegion *,Object *,Unit *);
	void RunBuildHelpers(ARegion *);
	void RunSailOrders();
	ARegion* Do1SailOrder(ARegion *, Object *, Unit *);
	void ClearCastEffects();
	void RunCastOrders();
	void RunACastOrder(ARegion *,Object *,Unit *);
	void RunTeleportOrders();

	// include spells.h for spell function declarations
#define GAME_SPELLS
#include "spells.h"
#undef GAME_SPELLS

	// Battle function
	void KillDead(Location *);
	int RunBattle(ARegion *,Unit *,Unit *,int = 0,int = 0);
	void GetSides(ARegion *,AList &,AList &,AList &,AList &,Unit *,Unit *,
	     int = 0,int = 0);
	int CanAttack(ARegion *,AList *,Unit *);
	void GetAFacs(ARegion *,Unit *,Unit *,AList &,AList &,AList &);
	void GetDFacs(ARegion *,Unit *,AList &);

private: // data
	AList factions;
	AList battles;
	ARegionList regions;
	int factionseq = 1;
	unsigned unitseq = 1;
	Unit **ppUnits = nullptr;
	unsigned maxppunits = 0;
	int shipseq = 100;
	int year = 1;
	int month = -1;

	enum Status
	{
		GAME_STATUS_UNINIT,
		GAME_STATUS_NEW,
		GAME_STATUS_RUNNING,
		GAME_STATUS_FINISHED,
	};
	Status gameStatus = GAME_STATUS_UNINIT;

	int guardfaction = 0;
	int monfaction = 0;
	int doExtraInit = 0;
};

#endif

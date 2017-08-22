#include "game.h"
#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <vector>

class PyAtlantis;
struct PyBattle;
struct PyStructure;

extern
void usage();

struct PyFaction
{
	PyFaction()
	: f_(NULL)
	{
	}

	explicit PyFaction(Faction *f)
	: f_(f)
	{
	}

	bool operator==(const PyFaction &rhs)
	{
		return f_ == rhs.f_;
	}

	bool isNpc() const { return f_->IsNPC() != 0; }

	int num() const { return f_->num; }

	std::string name() const
	{
		if (!f_->name)
			return "null";

		return f_->name->str();
	}

	void addBattle(const PyBattle &b);

	Faction *f_;
};
typedef std::vector<PyFaction> PyFactionList;

struct PyStructure
{
	PyStructure()
	: o_(NULL)
	{
	}

	explicit PyStructure(Object *o)
	: o_(o)
	{
	}

	bool operator==(const PyStructure &rhs)
	{
		return o_ == rhs.o_;
	}

	std::string name()
	{
		return o_->name->str();
	}

	Object *o_;
};
typedef std::vector<PyStructure> PyStructureList;

struct PyRegion
{
	PyRegion()
	: r_(NULL)
	{
	}

	explicit PyRegion(ARegion *r)
	: r_(r)
	{
	}

	bool operator==(const PyRegion &rhs)
	{
		return r_ == rhs.r_;
	}

	std::string name()
	{
		return r_->name->str();
	}

	bool isSafeRegion() const
	{
		return r_->IsSafeRegion() != 0;
	}

	PyStructure getDummy() { return PyStructure(r_->GetDummy()); }

	bool isPresent(const PyFaction &f) const { return r_->Present(f.f_); }

	ARegion *r_;
};
typedef std::vector<PyRegion> PyRegionList;

struct PyUnit
{
	PyUnit()
	: u_(NULL)
	{
	}

	explicit PyUnit(Unit *u)
	: u_(u)
	{
	}

	bool operator==(const PyUnit &rhs)
	{
		return u_ == rhs.u_;
	}

	std::string name()
	{
		return u_->name->str();
	}

	void addError(const std::string &msg) { u_->Error(msg.c_str()); }

	bool isAlive() { return u_->IsAlive() != 0; }
	bool canAttack() { return u_->canattack != 0; }
	Guard guard() { return Guard(u_->guard); }
	int canSee(const PyRegion &r, const PyUnit &u) { return u_->CanSee(r.r_, u.u_); }
	bool canCatch(const PyRegion &r, const PyUnit &u) { return u_->CanCatch(r.r_, u.u_) != 0; }

	EAttitude attitude(const PyRegion &r, const PyUnit &u) const
	{
		return EAttitude(u_->GetAttitude(r.r_, u.u_));
	}

	PyFaction faction() { return PyFaction(u_->faction); }

	Unit *u_;
};
typedef std::vector<PyUnit> PyUnitList;

struct PyLocation
{
	PyLocation(const PyUnit &u, const PyStructure &o, const PyRegion &r)
	{
		l_.unit = u.u_;
		l_.obj = o.o_;
		l_.region = r.r_;
	}

	explicit PyLocation(Location &l)
	: l_(l)
	{
	}

	bool operator==(const PyLocation &rhs) const
	{
		return l_.unit == rhs.l_.unit &&
		       l_.obj  == rhs.l_.obj  &&
		       l_.region == rhs.l_.region;
	}

	Location* makeLocation() const
	{
		return new Location(l_);
	}

	//PyUnit unit() { return PyUnit(l_.unit); };
	//PyStructure obj() { return PyStructure(l_.obj); }
	//PyRegion region() { return PyRegion(l_.region); }
	std::string unitName() const { return l_.unit->name->str(); }

	Location l_;
};
typedef std::vector<PyLocation> PyLocationList;

struct PyBattle
{
	PyBattle()
	: b_(new Battle)
	{
	}

	void writeSides(PyAtlantis &game, const PyRegion &r, const PyUnit &att, const PyUnit &tar,
	                const PyLocationList &att_list, const PyLocationList &def_list, bool asn);

	int run(const PyRegion &r, const PyUnit &att, const PyLocationList &att_list,
	        const PyUnit &tar, const PyLocationList &def_list, int asn, PyAtlantis &game);

	Battle *b_;
};

class PyAtlantis
{
public:
	PyAtlantis()
	{
		game_.ModifyTablesPerRuleset();
	}

	~PyAtlantis()
	{
		doneIO();
	}

	void newGame(int seed)
	{
		game_.NewGame(seed);
	}

	bool save()
	{
		return game_.SaveGame() == 0;
	}

	bool open()
	{
		return game_.OpenGame() == 0;
	}

	bool readPlayers()
	{
		game_.PreProcessTurn();

		Awrite("Reading the Gamemaster File...");
		return game_.ReadPlayers() == 0;
	}

	bool isFinished()
	{
		return game_.gameStatus == Game::GAME_STATUS_FINISHED;
	}

	void parseOrders(int fac_num, const std::string &fname)
	{
		Aorders file;

		if (file.OpenByName(fname.c_str()) != -1)
			game_.ParseOrders(fac_num, &file, NULL);

		file.Close();
	}

	void defaultWorkOrder() { game_.DefaultWorkOrder(); }
	void removeInactiveFactions() { game_.RemoveInactiveFactions(); }
	void runFindOrders() { game_.RunFindOrders(); }
	void runEnterOrders() { game_.RunEnterOrders(); }
	void runPromoteOrders() { game_.RunPromoteOrders(); }

	void doAttackOrders()
	{
		forlist(&game_.regions) {
			ARegion * r = (ARegion *) elem;
			forlist(&r->objects) {
				Object * o = (Object *) elem;
				forlist(&o->units) {
					Unit * u = (Unit *) elem;
					if (u->type == U_WMON) {
						if (u->canattack && u->IsAlive()) {
							game_.CheckWMonAttack(r,u);
						}
					} else {
						if (u->IsAlive() && u->attackorders) {
							AttackOrder * ord = u->attackorders;
							while (ord->targets.Num()) {
								UnitId * id = (UnitId *) ord->targets.First();
								ord->targets.Remove(id);
								Unit * t = r->GetUnitId(id,u->faction->num);
								delete id;
								if (u->canattack && u->IsAlive()) {
									if (t) {
										game_.AttemptAttack(r,u,t,0);
									} else {
										u->Error("ATTACK: Non-existent unit.");
									}
								}
							}
							delete ord;
							u->attackorders = 0;
						}
					}
				}
			}
		}
	}

	PyFactionList getDefendingFactions(const PyRegion &r, const PyUnit &victim_unit)
	{
		AList defenders;
		game_.GetDFacs(r.r_, victim_unit.u_, defenders);

		PyFactionList ret;
		forlist(&defenders)
		{
			FactionPtr *e = (FactionPtr*)elem;
			ret.push_back(PyFaction(e->ptr));
		}
		return ret;
	}

	PyLocationList getAttackingFactions(const PyRegion &r, const PyUnit &attacker, const PyUnit &victim_unit,
	    const PyFactionList &def_facs, PyFactionList &att_facs)
	{
		AList afacs;
		for (PyFactionList::const_iterator i = att_facs.begin(); i != att_facs.end(); ++i)
			afacs.push_back(new FactionPtr(i->f_));

		AList dfacs;
		for (PyFactionList::const_iterator i = def_facs.begin(); i != def_facs.end(); ++i)
			dfacs.push_back(new FactionPtr(i->f_));

		AList attackers;
		game_.GetAFacs(r.r_, attacker.u_, victim_unit.u_, dfacs, afacs, attackers);

		// copy back updated attacking factions
		att_facs.clear();
		{
			forlist(&afacs)
			{
				FactionPtr *e = (FactionPtr*)elem;
				att_facs.push_back(PyFaction(e->ptr));
			}
		}

		// return output argument
		PyLocationList ret;
		forlist(&attackers)
		{
			Location *e = (Location*)elem;
			ret.push_back(PyLocation(*e));
		}
		return ret;
	}

	void getSides(const PyRegion &battle_region, const PyFactionList &att_facs, const PyFactionList &def_facs,
	    PyLocationList &att_locs, PyLocationList &def_locs, PyUnit &attacker, PyUnit &tar_unit, int adv)
	{
		AList afacs;
		for (PyFactionList::const_iterator i = att_facs.begin(); i != att_facs.end(); ++i)
			afacs.push_back(new FactionPtr(i->f_));

		AList dfacs;
		for (PyFactionList::const_iterator i = def_facs.begin(); i != def_facs.end(); ++i)
			dfacs.push_back(new FactionPtr(i->f_));

		AList atts, defs; // output args
		game_.GetSides(battle_region.r_, afacs, dfacs, atts, defs, attacker.u_, tar_unit.u_, adv);

		forlist(&atts)
		{
			Location *e = (Location*)elem;
			// don't duplicate attackers
			if (std::find(att_locs.begin(), att_locs.end(), PyLocation(*e)) == att_locs.end())
			{
				att_locs.push_back(PyLocation(*e));
			}
		}

		{ // macro protection
		forlist(&defs)
		{
			Location *e = (Location*)elem;
			def_locs.push_back(PyLocation(*e));
		}
		}
	}

	void addBattle(PyBattle &battle) { game_.battles.Add(battle.b_); }
	void killDead(const PyLocation &l) { game_.KillDead(&l.l_); }
	void runStealOrders() { game_.RunStealOrders(); }
	void doGiveOrders() { game_.DoGiveOrders(); }
	void doExchangeOrders() { game_.DoExchangeOrders(); }
	void runDestroyOrders() { game_.RunDestroyOrders(); }
	void runPillageOrders() { game_.RunPillageOrders(); }
	void runTaxOrders() { game_.RunTaxOrders(); }
	void doGuard1Orders() { game_.DoGuard1Orders(); }
	void runCastOrders() { game_.RunCastOrders(); }
	void runSellOrders() { game_.RunSellOrders(); }
	void runBuyOrders() { game_.RunBuyOrders(); }
	void runForgetOrders() { game_.RunForgetOrders(); }
	void midProcessTurn() { game_.MidProcessTurn(); }
	void runQuitOrders() { game_.RunQuitOrders(); }
	void deleteEmptyUnits() { game_.DeleteEmptyUnits(); }
	void sinkUncrewedShips() { game_.SinkUncrewedShips(); }
	void drownUnits() { game_.DrownUnits(); }
	void doWithdrawOrders() { game_.DoWithdrawOrders(); }
	void runSailOrders() { game_.RunSailOrders(); }
	void runMoveOrders() { game_.RunMoveOrders();}
	void findDeadFactions() { game_.FindDeadFactions(); }
	void runTeachOrders() { game_.RunTeachOrders(); }
	void runMonthOrders() { game_.RunMonthOrders();}
	void runTeleportOrders() { game_.RunTeleportOrders(); }
	void assessMaintenance() { game_.AssessMaintenance(); }
	void postProcessTurn() { game_.PostProcessTurn(); }

	void writeReports() { game_.WriteReport(); }
	void writePlayers() { game_.WritePlayers(); }

	void cleanup()
	{
		game_.battles.DeleteAll();
		game_.EmptyHell(); 
		game_.DeleteDeadFactions();
	}

	void dummy()
	{
		game_.DummyGame();
	}

	bool checkOrders()
	{
		return game_.DoOrdersCheck("", "") == 0;
	}

	bool genRules()
	{
		return game_.GenRules("", "", "") == 0;
	}

	void enableItem(const std::string &item_name, bool enable)
	{
		AString tmp(item_name.c_str());
		const int item = ParseAllItems(&tmp);
		if (item == -1)
		{
			std::cout << "Enable bad item " << item_name << std::endl;
			return;
		}

		if (enable)
			game_.EnableItem(item);
		else
			game_.DisableItem(item);
	}

	void enableItem(const std::string &item_name)
	{
		enableItem(item_name, true);
	}

	PyFactionList factions()
	{
		if (factionMap_.empty())
		{
			forlist(&game_.factions)
			{
				Faction *f = (Faction*)elem;
				factionMap_.insert(std::make_pair(f, PyFaction(f)));
			}
		}

		PyFactionList ret;
		for(std::map<Faction*, PyFaction>::iterator i = factionMap_.begin(); i != factionMap_.end(); ++i)
		{
			ret.push_back(i->second);
		}
		return ret;
	}

	PyRegionList regions()
	{
		if (regionMap_.empty())
		{
			forlist(&game_.regions)
			{
				ARegion *r = (ARegion*)elem;
				regionMap_.insert(std::make_pair(r, PyRegion(r)));
			}
		}

		PyRegionList ret;
		for(std::map<ARegion*, PyRegion>::iterator i = regionMap_.begin(); i != regionMap_.end(); ++i)
		{
			ret.push_back(i->second);
		}
		return ret;
	}

	PyStructureList structures(const PyRegion &r)
	{
		PyStructureList ret;
		forlist(&r.r_->objects)
		{
			ret.push_back(PyStructure((Object*)elem));
		}
		return ret;
	}

	PyUnitList units(const PyStructure &o)
	{
		PyUnitList ret;
		forlist(&o.o_->units)
		{
			ret.push_back(PyUnit((Unit*)elem));
		}
		return ret;
	}

	ARegionList* region_list() { return &game_.regions; }

private:
	Game game_;
	std::map<Faction*, PyFaction> factionMap_;
	std::map<ARegion*, PyRegion> regionMap_;
};

void PyFaction::addBattle(const PyBattle &b)
{
	BattlePtr *p = new BattlePtr;
	p->ptr = b.b_;
	f_->battles.Add(p);
}

void PyBattle::writeSides(PyAtlantis &game, const PyRegion &r, const PyUnit &att, const PyUnit &tar,
                const PyLocationList &att_list, const PyLocationList &def_list, bool asn)
{
	// atts and defs are Location lists
	AList atts, defs;
	for (PyLocationList::const_iterator i = att_list.begin(); i != att_list.end(); ++i)
		atts.Add(i->makeLocation());
	for (PyLocationList::const_iterator i = def_list.begin(); i != def_list.end(); ++i)
		defs.Add(i->makeLocation());

	b_->WriteSides(r.r_, att.u_, tar.u_, &atts, &defs, asn, game.region_list());
}

int PyBattle::run(const PyRegion &r, const PyUnit &att, const PyLocationList &att_list,
     const PyUnit &tar, const PyLocationList &def_list, int asn, PyAtlantis &game)
{
	AList atts, defs;
	for (PyLocationList::const_iterator i = att_list.begin(); i != att_list.end(); ++i)
		atts.Add(i->makeLocation());
	for (PyLocationList::const_iterator i = def_list.begin(); i != def_list.end(); ++i)
		defs.Add(i->makeLocation());

	// int Run(ARegion *region, Unit *att, AList *atts, Unit *tar, AList *defs, int ass, ARegionList *pRegs)
	return b_->Run(r.r_, att.u_, &atts, tar.u_, &defs, asn, game.region_list());
}

//----------------------------------------------------------------------------
BOOST_PYTHON_MODULE(Atlantis)
{
	using namespace boost::python;
	void (PyAtlantis::*enItem1)(const std::string &) = &PyAtlantis::enableItem;
	void (PyAtlantis::*enItem2)(const std::string &, bool) = &PyAtlantis::enableItem;

	def("usage", usage);

	class_<PyFactionList>("FactionList")
	    .def(vector_indexing_suite<PyFactionList>());

	class_<PyFaction>("Faction")
	    .def("isNpc", &PyFaction::isNpc)
	    .def("num", &PyFaction::num)
	    .def("name", &PyFaction::name)
	    .def("addBattle", &PyFaction::addBattle)
	    ;

	class_<PyRegionList>("RegionList")
	    .def(vector_indexing_suite<PyRegionList>());

	class_<PyRegion>("Region")
	    .def("name", &PyRegion::name)
	    .def("isSafeRegion", &PyRegion::isSafeRegion)
	    .def("getDummy", &PyRegion::getDummy)
	    .def("isPresent", &PyRegion::isPresent)
	    ;

	class_<PyStructureList>("StructureList")
	    .def(vector_indexing_suite<PyStructureList>());

	class_<PyStructure>("Structure")
	    .def("name", &PyStructure::name)
	    ;

	class_<PyUnitList>("UnitList")
	    .def(vector_indexing_suite<PyUnitList>());

	class_<PyUnit>("Unit")
	    .def("name", &PyUnit::name)
	    .def("faction", &PyUnit::faction)
	    .def("addError", &PyUnit::addError)
	    .def("isAlive", &PyUnit::isAlive)
	    .def("canAttack", &PyUnit::canAttack)
	    .def("guard", &PyUnit::guard)
	    .def("attitude", &PyUnit::attitude)
	    .def("canSee", &PyUnit::canSee)
	    .def("canCatch", &PyUnit::canCatch)
	    ;

	enum_<Guard>("Guard")
	    .value("none", GUARD_NONE)
	    .value("guard", GUARD_GUARD)
	    .value("avoid", GUARD_AVOID)
	    .value("set", GUARD_SET)
	    .value("advance", GUARD_ADVANCE)
	    ;

	 enum_<EAttitude>("EAttitude")
	    .value("hostile", A_HOSTILE)
	    .value("unfriendly", A_UNFRIENDLY)
	    .value("neutral", A_NEUTRAL)
	    .value("friendly", A_FRIENDLY)
	    .value("ally", A_ALLY)
	    ;

	class_<PyLocationList>("LocationList")
	    .def(vector_indexing_suite<PyLocationList>());

	class_<PyLocation>("Location", init<const PyUnit&, const PyStructure&, const PyRegion&>())
	    .def("unitName", &PyLocation::unitName)
	    ;

	class_<PyBattle>("Battle")
	    .def("writeSides", &PyBattle::writeSides)
	    .def("run", &PyBattle::run)
	    ;

	class_<PyAtlantis>("PyAtlantis")
	    .def("new", &PyAtlantis::newGame)
	    .def("save", &PyAtlantis::save)
	    .def("open", &PyAtlantis::open)
	    .def("dummy", &PyAtlantis::dummy)
	    .def("readPlayers", &PyAtlantis::readPlayers)
	    .def("isFinished", &PyAtlantis::isFinished)
	    .def("parseOrders", &PyAtlantis::parseOrders)
	    .def("defaultWorkOrder", &PyAtlantis::defaultWorkOrder)
	    .def("removeInactiveFactions", &PyAtlantis::removeInactiveFactions)
	    .def("runFindOrders", &PyAtlantis::runFindOrders)
	    .def("runEnterOrders", &PyAtlantis::runEnterOrders)
	    .def("runPromoteOrders", &PyAtlantis::runPromoteOrders)
	    .def("doAttackOrders", &PyAtlantis::doAttackOrders)
	    .def("getDefendingFactions", &PyAtlantis::getDefendingFactions)
	    .def("getAttackingFactions", &PyAtlantis::getAttackingFactions)
	    .def("addBattle", &PyAtlantis::addBattle)
	    .def("getSides", &PyAtlantis::getSides)
	    .def("killDead", &PyAtlantis::killDead)
	    .def("runStealOrders", &PyAtlantis::runStealOrders)
	    .def("doGiveOrders", &PyAtlantis::doGiveOrders)
	    .def("doExchangeOrders", &PyAtlantis::doExchangeOrders)
	    .def("runDestroyOrders", &PyAtlantis::runDestroyOrders)
	    .def("runPillageOrders", &PyAtlantis::runPillageOrders)
	    .def("runTaxOrders", &PyAtlantis::runTaxOrders)
	    .def("doGuard1Orders", &PyAtlantis::doGuard1Orders)
	    .def("runCastOrders", &PyAtlantis::runCastOrders)
	    .def("runSellOrders", &PyAtlantis::runSellOrders)
	    .def("runBuyOrders", &PyAtlantis::runBuyOrders)
	    .def("runForgetOrders", &PyAtlantis::runForgetOrders)
	    .def("midProcessTurn", &PyAtlantis::midProcessTurn)
	    .def("runQuitOrders", &PyAtlantis::runQuitOrders)
	    .def("deleteEmptyUnits", &PyAtlantis::deleteEmptyUnits)
	    .def("sinkUncrewedShips", &PyAtlantis::sinkUncrewedShips)
	    .def("drownUnits", &PyAtlantis::drownUnits)
	    .def("doWithdrawOrders", &PyAtlantis::doWithdrawOrders)
	    .def("runSailOrders", &PyAtlantis::runSailOrders)
	    .def("runMoveOrders", &PyAtlantis::runMoveOrders)
	    .def("findDeadFactions", &PyAtlantis::findDeadFactions)
	    .def("runTeachOrders", &PyAtlantis::runTeachOrders)
	    .def("runMonthOrders", &PyAtlantis::runMonthOrders)
	    .def("runTeleportOrders", &PyAtlantis::runTeleportOrders)
	    .def("assessMaintenance", &PyAtlantis::assessMaintenance)
	    .def("postProcessTurn", &PyAtlantis::postProcessTurn)
	    .def("writeReports", &PyAtlantis::writeReports)
	    .def("writePlayers", &PyAtlantis::writePlayers)
	    .def("cleanup", &PyAtlantis::cleanup)
	    .def("checkOrders", &PyAtlantis::checkOrders)
	    .def("genRules", &PyAtlantis::genRules)
	    .def("enableItem", enItem1)
	    .def("enableItem", enItem2)
	    .def("factions", &PyAtlantis::factions)
	    .def("regions", &PyAtlantis::regions)
	    .def("structures", &PyAtlantis::structures)
	    .def("units", &PyAtlantis::units)
	    ;

	initIO();
}


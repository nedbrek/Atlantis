#include "game.h"
#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <vector>

extern
void usage();

class PyFaction
{
public:
	PyFaction()
	: f_(NULL)
	{
	}

	PyFaction(Faction *f)
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

private:
	Faction *f_;
};
typedef std::vector<PyFaction> PyFactionList;

struct PyRegion
{
	PyRegion()
	: r_(NULL)
	{
	}

	PyRegion(ARegion *r)
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

	ARegion *r_;
};
typedef std::vector<PyRegion> PyRegionList;

struct PyStructure
{
	PyStructure()
	: o_(NULL)
	{
	}

	PyStructure(Object *o)
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

struct PyUnit
{
	PyUnit()
	: u_(NULL)
	{
	}

	PyUnit(Unit *u)
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

	bool isAlive() { return u_->IsAlive() != 0; }
	bool canAttack() { return u_->canattack != 0; }
	Guard guard() { return Guard(u_->guard); }
	EAttitude attitude(const PyRegion &r, const PyUnit &u)
	{
		return EAttitude(u_->GetAttitude(r.r_, u.u_));
	}

	Unit *u_;
};
typedef std::vector<PyUnit> PyUnitList;

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
	void doAttackOrders() { game_.DoAttackOrders(); }

	void attemptAttack(const PyRegion &r, const PyUnit &u, const PyUnit &t, int val)
	{
		game_.AttemptAttack(r.r_, u.u_, t.u_, val);
	}

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

private:
	Game game_;
	std::map<Faction*, PyFaction> factionMap_;
	std::map<ARegion*, PyRegion> regionMap_;
};

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
	    ;

	class_<PyRegionList>("RegionList")
	    .def(vector_indexing_suite<PyRegionList>());

	class_<PyRegion>("Region")
	    .def("name", &PyRegion::name)
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
	    .def("isAlive", &PyUnit::isAlive)
	    .def("canAttack", &PyUnit::canAttack)
	    .def("guard", &PyUnit::guard)
	    .def("attitude", &PyUnit::attitude)
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
	    .def("attemptAttack", &PyAtlantis::attemptAttack)
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


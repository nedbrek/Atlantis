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
	void doAutoAttacks() { game_.DoAutoAttacks(); }
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
		PyFactionList ret;
		forlist(&game_.factions)
		{
			Faction *f = (Faction*)elem;
			ret.push_back(f);
		}
		return ret;
	}

private:
	Game game_;
};

BOOST_PYTHON_MODULE(Atlantis)
{
	using namespace boost::python;
	void (PyAtlantis::*enItem1)(const std::string &) = &PyAtlantis::enableItem;
	void (PyAtlantis::*enItem2)(const std::string &, bool) = &PyAtlantis::enableItem;

	def("usage", usage);

	class_<PyFactionList>("FactionList")
	    .def(vector_indexing_suite<PyFactionList>());

	class_<PyFaction>("PyFaction")
	    .def("isNpc", &PyFaction::isNpc)
	    .def("num", &PyFaction::num)
	    .def("name", &PyFaction::name)
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
	    .def("doAutoAttacks", &PyAtlantis::doAutoAttacks)
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
	    ;

	initIO();
}


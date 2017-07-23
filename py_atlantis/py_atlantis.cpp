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

	bool newGame(int seed)
	{
		if (game_.NewGame(seed) == 0)
			return false;
		return game_.WritePlayers() == 0;
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

	bool run()
	{
		Awrite("Running the Turn...");

		game_.DefaultWorkOrder();
		game_.RemoveInactiveFactions();

		// Form and instant orders are handled during parsing
		Awrite("Running FIND Orders...");
		game_.RunFindOrders();

		Awrite("Running ENTER/LEAVE Orders...");
		game_.RunEnterOrders();
		Awrite("Running PROMOTE/EVICT Orders...");
		game_.RunPromoteOrders();
		Awrite("Running Combat...");
		game_.DoAttackOrders();
		game_.DoAutoAttacks();
		Awrite("Running STEAL/ASSASSINATE Orders...");
		game_.RunStealOrders();
		Awrite("Running GIVE/PAY/TRANSFER Orders...");
		game_.DoGiveOrders();
		Awrite("Running EXCHANGE Orders...");
		game_.DoExchangeOrders();
		Awrite("Running DESTROY Orders...");
		game_.RunDestroyOrders();
		Awrite("Running PILLAGE Orders...");
		game_.RunPillageOrders();
		Awrite("Running TAX Orders...");
		game_.RunTaxOrders();
		Awrite("Running GUARD 1 Orders...");
		game_.DoGuard1Orders();
		Awrite("Running Magic Orders...");
		game_.ClearCastEffects();
		game_.RunCastOrders();
		Awrite("Running SELL Orders...");
		game_.RunSellOrders();
		Awrite("Running BUY Orders...");
		game_.RunBuyOrders();
		Awrite("Running FORGET Orders...");
		game_.RunForgetOrders();
		Awrite("Mid-Turn Processing...");
		game_.MidProcessTurn();
		Awrite("Running QUIT Orders...");
		game_.RunQuitOrders();
		Awrite("Removing Empty Units...");
		game_.DeleteEmptyUnits();
		game_.SinkUncrewedShips();
		game_.DrownUnits();
		if (Globals->ALLOW_WITHDRAW)
		{
			Awrite("Running WITHDRAW Orders...");
			game_.DoWithdrawOrders();
		}
		Awrite("Running Sail Orders...");
		game_.RunSailOrders();
		Awrite("Running Move Orders...");
		game_.RunMoveOrders();
		game_.SinkUncrewedShips();
		game_.DrownUnits();
		game_.FindDeadFactions();
		Awrite("Running Teach Orders...");
		game_.RunTeachOrders();
		Awrite("Running Month-long Orders...");
		game_.RunMonthOrders();
		game_.RunTeleportOrders();
		Awrite("Assessing Maintenance costs...");
		game_.AssessMaintenance();

		Awrite("Post-Turn Processing...");
		game_.gameStatus = Game::GAME_STATUS_RUNNING;
		game_.PostProcessTurn(); // status can be END

		game_.DeleteEmptyUnits();
		game_.RemoveEmptyObjects();

		Awrite("Writing the Report File...");
		game_.WriteReport();
		Awrite("");
		game_.battles.DeleteAll();

		Awrite("Writing Playerinfo File...");
		game_.WritePlayers();
		game_.EmptyHell(); 

		Awrite("Removing Dead Factions...");
		game_.DeleteDeadFactions();
		Awrite("done");

		return false;
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
	    .def("run", &PyAtlantis::run)
	    .def("checkOrders", &PyAtlantis::checkOrders)
	    .def("genRules", &PyAtlantis::genRules)
	    .def("enableItem", enItem1)
	    .def("enableItem", enItem2)
	    .def("factions", &PyAtlantis::factions)
	    ;

	initIO();
}


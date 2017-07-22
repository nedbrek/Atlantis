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

	bool run()
	{
		game_.gameStatus = Game::GAME_STATUS_RUNNING;

		Awrite("Reading the Orders File...");
		game_.ReadOrders();

		if (Globals->MAX_INACTIVE_TURNS != -1)
		{
			Awrite("QUITting Inactive Factions...");
			game_.RemoveInactiveFactions();
		}

		Awrite("Running the Turn...");
		game_.RunOrders();

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
	    .def("run", &PyAtlantis::run)
	    .def("checkOrders", &PyAtlantis::checkOrders)
	    .def("genRules", &PyAtlantis::genRules)
	    .def("enableItem", enItem1)
	    .def("enableItem", enItem2)
	    .def("factions", &PyAtlantis::factions)
	    ;

	initIO();
}


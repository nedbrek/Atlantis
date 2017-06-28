#include "game.h"
#include <boost/python.hpp>

extern
void usage();

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

	bool run()
	{
		return game_.RunGame() == 0;
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

private:
	Game game_;
};

BOOST_PYTHON_MODULE(Atlantis)
{
	using namespace boost::python;
	def("usage", usage);
	class_<PyAtlantis>("PyAtlantis")
	    .def("new", &PyAtlantis::newGame)
	    .def("save", &PyAtlantis::save)
	    .def("open", &PyAtlantis::open)
	    .def("dummy", &PyAtlantis::dummy)
	    .def("run", &PyAtlantis::run)
	    .def("checkOrders", &PyAtlantis::checkOrders)
	    .def("genRules", &PyAtlantis::genRules)
	    ;

	initIO();
}


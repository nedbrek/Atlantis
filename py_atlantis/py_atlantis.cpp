#include <boost/python.hpp>

extern
void usage();

class PyAtlantis
{
};

BOOST_PYTHON_MODULE(Atlantis)
{
	using namespace boost::python;
	def("usage", usage);
}


//
// This is a **build** test - it does nothing else except ensure that all the C++ wrapper classes are included and that
// the program builds.
//
// Once there are demos which exercise all the C++ classes this "test" can be removed
//
#include <cstdlib>
#include <clocale>
#include <iostream>
#include <ncpp/NotCurses.hh>
#include <ncpp/Menu.hh>
#include <ncpp/Plane.hh>
#include <ncpp/Reel.hh>
#include <ncpp/MultiSelector.hh>
#include <ncpp/Selector.hh>
#include <ncpp/Visual.hh>
#include <ncpp/Direct.hh>
#include <ncpp/Plot.hh>
#include <ncpp/FDPlane.hh>
#include <ncpp/Subproc.hh>

using namespace ncpp;

int run ()
{
	NotCurses nc;

	const char *ncver = nc.version ();
	Plane plane (1, 1, 0, 0);
	Plot plot1 (plane);
	PlotU plot2 (plane);
	PlotD plot3 (plane);

	nc.stop ();

	Direct direct (getenv ("TERM"));
	direct.set_fg (0xb5, 0x0d, 0xff);
	std::cout << "notcurses version: ";
	direct.set_bg (0x05, 0x6e, 0xee);
	direct.set_fg (0xe2, 0xbf, 0x00);
	std::cout << ncver << std::endl;

	return 0;
}

int main ()
{
	if (!setlocale (LC_ALL, "")){
		std::cerr << "Couldn't set locale based on user preferences" << std::endl;
		return EXIT_FAILURE;
	}
	
	try {
		return run ();
	} catch (ncpp::init_error &e) {
		std::cerr << "Initialization error: " << e.what () << std::endl;
	} catch (ncpp::invalid_state_error &e) {
		std::cerr << "Invalid state error: " << e.what () << std::endl;
	} catch (ncpp::invalid_argument &e) {
		std::cerr << "Invalid argument error: " << e.what () << std::endl;
	}

	return 1;
}

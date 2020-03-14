#include <ncpp/NotCurses.hh>
#include <ncpp/NCLogLevel.hh>

using namespace ncpp;

notcurses_options NotCurses::default_notcurses_options = {
	/* termtype */                 nullptr,
	/* inhibit_alternate_screen */ false,
	/* retain_cursor */            false,
	/* suppress_bannner */         false,
	/* no_quit_sighandlers */      false,
	/* no_winch_sighandler */      false,
	/* renderfp */                 nullptr,
	/* loglevel */                 NCLogLevel::Silent,
	/* margin_t */                 0,
	/* margin_r */                 0,
	/* margin_b */                 0,
	/* margin_l */                 0,
};

NotCurses *NotCurses::_instance = nullptr;
std::mutex NotCurses::init_mutex;

NotCurses::~NotCurses ()
{
	const std::lock_guard<std::mutex> lock (init_mutex);

	if (nc == nullptr)
		return;

	notcurses_stop (nc);
	_instance = nullptr;
}

NotCurses::NotCurses (const notcurses_options &nc_opts, FILE *fp)
{
	const std::lock_guard<std::mutex> lock (init_mutex);

	if (_instance != nullptr)
		throw new init_error ("There can be only one instance of the NotCurses class. Use NotCurses::get_instance() to access the existing instance.");

	nc = notcurses_init (&nc_opts, fp == nullptr ? stdout : fp);
	if (nc == nullptr)
		throw new init_error ("notcurses failed to initialize");
	_instance = this;
}

Plane* NotCurses::get_top () noexcept
{
	ncplane *top = notcurses_top (nc);
	if (top == nullptr)
		return nullptr;

	return Plane::map_plane (top);
}

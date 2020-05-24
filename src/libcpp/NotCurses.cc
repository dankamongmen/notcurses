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
	/* flags */                    0,
};

NotCurses *NotCurses::_instance = nullptr;
std::mutex NotCurses::init_mutex;

NotCurses::~NotCurses ()
{
	const std::lock_guard<std::mutex> lock (init_mutex);

	if (nc == nullptr)
		return;

	notcurses_stop (nc);
	if (_instance == this)
		_instance = nullptr;
}

NotCurses::NotCurses (const notcurses_options &nc_opts, FILE *fp)
	: Root (nullptr)
{
	const std::lock_guard<std::mutex> lock (init_mutex);

	nc = notcurses_init (&nc_opts, fp);
	if (nc == nullptr)
		throw new init_error ("notcurses failed to initialize");
	if (_instance == nullptr)
		_instance = this;
}

Plane* NotCurses::get_top () noexcept
{
	ncplane *top = notcurses_top (nc);
	if (top == nullptr)
		return nullptr;

	return Plane::map_plane (top);
}

// This is potentially dangerous, but alas necessary. It can cause other calls
// here to fail in a bad way, but we need a way to report errors to
// std{out,err} in case of failure and that will work only if notcurses is
// stopped, so...
bool NotCurses::stop ()
{
  if (nc == nullptr)
    throw invalid_state_error (ncpp_invalid_state_message);

  bool ret = !notcurses_stop (nc);
  nc = nullptr;

  const std::lock_guard<std::mutex> lock (init_mutex);
  if (_instance == this)
	  _instance = nullptr;

  return ret;
}

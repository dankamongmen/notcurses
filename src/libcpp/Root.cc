#include <ncpp/Root.hh>
#include <ncpp/NotCurses.hh>

using namespace ncpp;

notcurses* Root::get_notcurses () const
{
	notcurses *ret = NotCurses::get_instance ();
	if (ret == nullptr)
		throw new invalid_state_error (ncpp_invalid_state_message);
	return ret;
}

bool Root::is_notcurses_stopped () const noexcept
{
	return NotCurses::is_notcurses_stopped ();
}

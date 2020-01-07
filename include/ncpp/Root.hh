#ifndef __NCPP_ROOT_HH
#define __NCPP_ROOT_HH

#include <notcurses.h>

#include "_helpers.hh"
#include "_exceptions.hh"

namespace ncpp {
	class NCPP_API_EXPORT Root
	{
	protected:
		static constexpr char ncpp_invalid_state_message[] = "notcurses++ is in an invalid state (already stopped?)";

	protected:
		notcurses* get_notcurses () const;

		// All the objects which need to destroy notcurses entities (planes, panelreel etc etc) **have to** call this
		// function before calling to notcurses from their destructor. This is to prevent a segfault when
		// NotCurses::stop has been called and the app uses smart pointers holding NotCurses objects which may be
		// destructed **after** notcurses is stopped.
		bool is_notcurses_stopped () const noexcept;
	};
}
#endif

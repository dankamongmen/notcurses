#ifndef __NCPP_UTILITIES_HH
#define __NCPP_UTILITIES_HH

#include <notcurses/notcurses.h>

#include "_helpers.hh"

namespace ncpp
{
	class NotCurses;
	class Plane;
	class Root;

	class NCPP_API_EXPORT Utilities
	{
	public:
		static ncplane* to_ncplane (const Plane *plane) noexcept;

		static ncplane* to_ncplane (const Plane &plane) noexcept
		{
			return to_ncplane (&plane);
		}

		static NotCurses* get_notcurses_cpp (const Root *o) noexcept;

		static NotCurses* get_notcurses_cpp (const Root &o) noexcept
		{
			return get_notcurses_cpp (&o);
		}

		static NotCurses* get_notcurses_cpp (const Plane *plane) noexcept;

		static NotCurses* get_notcurses_cpp (const Plane &plane) noexcept
		{
			return get_notcurses_cpp (&plane);
		}
	};
}
#endif

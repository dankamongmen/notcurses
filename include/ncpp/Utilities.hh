#ifndef __NCPP_UTILITIES_HH
#define __NCPP_UTILITIES_HH

#include <notcurses/notcurses.h>

#include "_helpers.hh"

namespace ncpp
{
	class Plane;

	class NCPP_API_EXPORT Utilities
	{
	public:
		static ncplane* to_ncplane (const Plane *plane) noexcept;

		static ncplane* to_ncplane (const Plane &plane) noexcept
		{
			return to_ncplane (&plane);
		}
	};
}
#endif

#ifndef __NCPP_VISUAL_HH
#define __NCPP_VISUAL_HH

#include <notcurses/notcurses.h>

#include "Root.hh"
#include "Plane.hh"
#include "Utilities.hh"

namespace ncpp
{
	class NotCurses;
	class Plane;

	class NCPP_API_EXPORT Visual : public Root
	{
	public:
		explicit Visual (const char *file, nc_err_e *ncerr)
		{
			visual = ncvisual_from_file (file, ncerr);
			if (visual == nullptr)
				throw init_error ("Notcurses failed to create a new visual");
		}

		~Visual () noexcept
		{
			if (!is_notcurses_stopped ())
				ncvisual_destroy (visual);
		}

		operator ncvisual* () const noexcept
		{
			return visual;
		}

		operator ncvisual const* () const noexcept
		{
			return visual;
		}

		nc_err_e decode () const noexcept
		{
			return ncvisual_decode (visual);
		}

		ncplane* render (const ncvisual_options* vopts) const NOEXCEPT_MAYBE
		{
			return ncvisual_render (get_notcurses (), visual, vopts); // FIXME error_guard
		}

		int stream (Plane& p, nc_err_e* ncerr, float timescale, streamcb streamer, void *curry = nullptr) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncvisual_stream (p.to_ncplane (), visual, ncerr, timescale, streamer, curry), -1);
		}

		char* subtitle () const noexcept
		{
			return ncvisual_subtitle (visual);
		}

		bool rotate (double rads) const NOEXCEPT_MAYBE
		{
			return error_guard (ncvisual_rotate (visual, rads), -1);
		}

	private:
		void common_init (ncplane *plane, const char *file, nc_err_e* ncerr)
		{
			if (plane == nullptr)
				throw invalid_argument ("'plane' must be a valid pointer");

			visual = ncplane_visual_open (plane, file, ncerr);
			if (visual == nullptr)
				throw init_error ("Notcurses failed to create a new visual");
		}

	private:
		ncvisual *visual = nullptr;
	};
}
#endif

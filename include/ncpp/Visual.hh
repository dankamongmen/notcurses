#ifndef __NCPP_VISUAL_HH
#define __NCPP_VISUAL_HH

#include <notcurses/notcurses.h>

#include "Root.hh"
#include "NCScale.hh"

namespace ncpp
{
	class Plane;

	class NCPP_API_EXPORT Visual : public Root
	{
	public:
		explicit Visual (Plane *plane, const char *file, nc_err_e* ncerr)
			: Visual (reinterpret_cast<ncplane*>(plane), file, ncerr)
		{}

		explicit Visual (Plane const* plane, const char *file, nc_err_e* ncerr)
			: Visual (const_cast<Plane*>(plane), file, ncerr)
		{}

		explicit Visual (Plane &plane, const char *file, nc_err_e* ncerr)
			: Visual (reinterpret_cast<ncplane*>(&plane), file, ncerr)
		{}

		explicit Visual (Plane const& plane, const char *file, nc_err_e* ncerr)
			: Visual (const_cast<Plane&>(plane), file, ncerr)
		{}

		explicit Visual (ncplane *plane, const char *file, nc_err_e* ncerr)
		{
			if (plane == nullptr)
				throw invalid_argument ("'plane' must be a valid pointer");

			visual = ncplane_visual_open (reinterpret_cast<ncplane*>(plane), file, ncerr);
			if (visual == nullptr)
				throw init_error ("notcurses failed to create a new visual");
		}

		explicit Visual (const char *file, nc_err_e* ncerr, int y, int x, NCScale scale)
		{
			visual = ncvisual_open_plane (get_notcurses (), file, ncerr, y, x, static_cast<ncscale_e>(scale));
			if (visual == nullptr)
				throw init_error ("notcurses failed to create a new visual");
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

		AVFrame* decode (nc_err_e* ncerr) const noexcept
		{
			return ncvisual_decode (visual, ncerr);
		}

		bool render (int begy, int begx, int leny, int lenx) const NOEXCEPT_MAYBE
		{
			return error_guard (ncvisual_render (visual, begy, begx, leny, lenx), -1);
		}

		int stream (nc_err_e* ncerr, float timescale, streamcb streamer, void *curry = nullptr) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncvisual_stream (get_notcurses (), visual, ncerr, timescale, streamer, curry), -1);
		}

		char* subtitle () const noexcept
		{
			return ncvisual_subtitle (visual);
		}

		Plane* get_plane () const noexcept;

	private:
		ncvisual *visual = nullptr;
	};
}
#endif

#ifndef __NCPP_VISUAL_HH
#define __NCPP_VISUAL_HH

#include <notcurses/notcurses.h>

#include "Root.hh"
#include "Plane.hh"
#include "Utilities.hh"
#include "NotCurses.hh"

namespace ncpp
{
	class NotCurses;
	class Plane;

	class NCPP_API_EXPORT Visual : public Root
	{
	public:
		explicit Visual (const char *file)
			: Root (NotCurses::get_instance ())
		{
			visual = ncvisual_from_file (file);
			if (visual == nullptr)
				throw init_error ("Notcurses failed to create a new visual");
		}

		explicit Visual (const uint32_t* rgba, int rows, int rowstride, int cols)
			: Root (NotCurses::get_instance ())
		{
			visual = ncvisual_from_rgba (rgba, rows, rowstride, cols);
			if (visual == nullptr)
				throw init_error ("Notcurses failed to create a new visual");
		}

		explicit Visual (const Plane& p, ncblitter_e blit, unsigned begy, unsigned begx, unsigned leny, unsigned lenx)
			: Root (NotCurses::get_instance ())
		{
			visual = ncvisual_from_plane (p, blit, begy, begx, leny, lenx);
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

		int decode () const noexcept
		{
			return ncvisual_decode (visual);
		}

		int decode_loop () const noexcept
		{
			return ncvisual_decode_loop (visual);
		}

		ncplane* blit (const ncvisual_options* vopts) const NOEXCEPT_MAYBE
		{
			return error_guard<ncplane*, ncplane*> (ncvisual_blit (get_notcurses (), visual, vopts), nullptr);
		}

		int stream (const ncvisual_options* vopts, float timescale, ncstreamcb streamer, void *curry = nullptr) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncvisual_stream (get_notcurses (), visual, timescale, streamer, vopts, curry), -1);
		}

		ncplane* subtitle (Plane& parent) noexcept
		{
			return ncvisual_subtitle_plane (parent, visual);
		}

		bool rotate (double rads) const NOEXCEPT_MAYBE
		{
			return error_guard (ncvisual_rotate (visual, rads), -1);
		}

		bool simple_streamer (ncvisual_options* vopts, const timespec* tspec, void* curry = nullptr) const NOEXCEPT_MAYBE
		{
			return error_guard (ncvisual_simple_streamer (visual, vopts, tspec, curry), -1);
		}

		int polyfill (int y, int x, uint32_t rgba) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncvisual_polyfill_yx (visual, y, x, rgba), -1);
		}

		bool geom (const struct ncvisual_options *vopts, ncvgeom* geom) const NOEXCEPT_MAYBE
		{
			return error_guard (ncvisual_geom (get_notcurses (), visual, vopts, geom), -1);
		}

		bool at (int y, int x, uint32_t* pixel) const
		{
			if (pixel == nullptr)
				throw invalid_argument ("'pixel' must be a valid pointer");

			return error_guard (ncvisual_at_yx (visual, y, x, pixel), -1);
		}

		bool set (int y, int x, uint32_t pixel) const NOEXCEPT_MAYBE
		{
			return error_guard (ncvisual_set_yx (visual, y, x, pixel), -1);
		}

	private:
		void common_init (ncplane *plane, const char *file)
		{
			if (plane == nullptr)
				throw invalid_argument ("'plane' must be a valid pointer");

			visual = ncvisual_from_file (file);
			if (visual == nullptr)
				throw init_error ("Notcurses failed to create a new visual");
		}

	private:
		ncvisual *visual = nullptr;
	};
}
#endif

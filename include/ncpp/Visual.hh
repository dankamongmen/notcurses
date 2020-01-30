#ifndef __NCPP_VISUAL_HH
#define __NCPP_VISUAL_HH

#include <notcurses.h>

#include "Root.hh"
#include "NCScale.hh"

namespace ncpp
{
	class Plane;

	class NCPP_API_EXPORT Visual : public Root
	{
	public:
		explicit Visual (Plane *plane, const char *file, int *averr)
			: Visual (reinterpret_cast<ncplane*>(plane), file, averr)
		{}

		explicit Visual (ncplane *plane, const char *file, int *averr)
		{
			if (plane == nullptr)
				throw new invalid_argument ("'plane' must be a valid pointer");

			visual = ncplane_visual_open (reinterpret_cast<ncplane*>(plane), file, averr);
			if (visual == nullptr)
				throw new init_error ("notcurses failed to create a new visual");
		}

		explicit Visual (const char *file, int *averr, int y, int x, NCScale scale)
		{
			visual = ncvisual_open_plane (get_notcurses (), file, averr, y, x, static_cast<ncscale_e>(scale));
			if (visual == nullptr)
				throw new init_error ("notcurses failed to create a new visual");
		}

		~Visual () noexcept
		{
			destroy_plane (get_plane ());
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

		AVFrame* decode (int *averr) const noexcept
		{
			return ncvisual_decode (visual, averr);
		}

		bool render (int begy, int begx, int leny, int lenx) const noexcept
		{
			return ncvisual_render (visual, begy, begx, leny, lenx) != -1;
		}

		int stream (int *averr, float timescale, streamcb streamer, void *curry = nullptr) const noexcept
		{
			return ncvisual_stream (get_notcurses (), visual, averr, timescale, streamer, curry);
		}

    char* subtitle () const noexcept
    {
      return ncvisual_subtitle (visual);
    }

		Plane* get_plane () const noexcept;

	private:
		static void destroy_plane (Plane *plane) noexcept;

	private:
		ncvisual *visual = nullptr;
	};
}
#endif

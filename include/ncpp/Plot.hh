#ifndef __NCPP_PLOT_HH
#define __NCPP_PLOT_HH

#include <notcurses/notcurses.h>

#include "Root.hh"
#include "NCAlign.hh"

namespace ncpp
{
	class Plane;

	class NCPP_API_EXPORT Plot : public Root
	{
	public:
		static ncplot_options default_options;

	public:
		explicit Plot (Plane *plane, const ncplot_options *opts = nullptr)
			: Plot (reinterpret_cast<ncplane*>(plane), opts)
		{}

		explicit Plot (Plane const* plane, const ncplot_options *opts = nullptr)
			: Plot (const_cast<Plane*>(plane), opts)
		{}

		explicit Plot (Plane &plane, const ncplot_options *opts = nullptr)
			: Plot (reinterpret_cast<ncplane*>(&plane), opts)
		{}

		explicit Plot (Plane const& plane, const ncplot_options *opts = nullptr)
			: Plot (const_cast<Plane*>(&plane), opts)
		{}

		explicit Plot (ncplane *plane, const ncplot_options *opts = nullptr,
                   uint64_t miny = 0, uint64_t maxy = 0)
		{
			if (plane == nullptr)
				throw invalid_argument ("'plane' must be a valid pointer");

			plot = ncuplot_create (plane, opts == nullptr ? &default_options : opts, miny, maxy);
			if (plot == nullptr)
				throw init_error ("notcurses failed to create a new plot");
		}

		~Plot ()
		{
			if (!is_notcurses_stopped ())
				ncuplot_destroy (plot);
		}

		bool add_sample(uint64_t x, uint64_t y) const NOEXCEPT_MAYBE
		{
			return error_guard (ncuplot_add_sample (plot, x, y), -1);
		}

		bool set_sample(uint64_t x, uint64_t y) const NOEXCEPT_MAYBE
		{
			return error_guard (ncuplot_set_sample (plot, x, y), -1);
		}

		Plane* get_plane () const noexcept;

	private:
		ncuplot *plot;
	};
}
#endif

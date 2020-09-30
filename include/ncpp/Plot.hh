#ifndef __NCPP_PLOT_HH
#define __NCPP_PLOT_HH

#include <type_traits>

#include <notcurses/notcurses.h>

#include "NCAlign.hh"
#include "Plane.hh"
#include "Utilities.hh"
#include "Widget.hh"

namespace ncpp
{
	template<typename TPlot, typename TCoord>
	class NCPP_API_EXPORT PlotBase : public Widget
	{
		static constexpr bool is_double = std::is_same_v<TCoord,double>;
		static constexpr bool is_uint64 = std::is_same_v<TCoord,uint64_t>;

	public:
		bool add_sample (uint64_t x, TCoord y) const NOEXCEPT_MAYBE
		{
			int ret;

			if constexpr (is_double) {
				ret = ncdplot_add_sample (plot, x, y);
			} else {
				ret = nduplot_add_sample (plot, x, y);
			}

			return error_guard (ret, -1);
		}

		bool set_sample (uint64_t x, TCoord y) const NOEXCEPT_MAYBE
		{
			int ret;

			if constexpr (is_double) {
				ret = ncdplot_set_sample (plot, x, y);
			} else {
				ret = nduplot_set_sample (plot, x, y);
			}
			return error_guard (ret, -1);
		}

		bool sample (uint64_t x, TCoord* y) const NOEXCEPT_MAYBE
		{
			int ret;

			if constexpr (is_double) {
				ret = ncdplot_sample (plot, x, y);
			} else {
				ret = ncuplot_sample (plot, x, y);
			}

			return error_guard (ret, -1);
		}

	protected:
		explicit PlotBase (Plane *plane, const ncplot_options *opts, TCoord miny = 0, TCoord maxy = 0)
			: Widget (Utilities::get_notcurses_cpp (plane))
		{
			static_assert (is_double || is_uint64, "PlotBase must be parameterized with either 'double' or 'uint64_t' types");
			if constexpr (is_double) {
				static_assert (std::is_same_v<TPlot, ncdplot>, "ncdplot must be used for a plot using double coordinates");
			} else {
				static_assert (std::is_same_v<TPlot, ncuplot>, "ncuplot must be used for a plot using uint64_t coordinates");
			}

			ensure_valid_plane (plane);

			if (!plane->is_valid ())
				throw invalid_argument ("Invalid Plane object passed in 'plane'. Widgets must not reuse the same plane.");

			if (opts == nullptr)
				throw invalid_argument ("'opts' must be a valid pointer");

			if constexpr (is_uint64) {
				plot = ncuplot_create (Utilities::to_ncplane (plane), opts, miny, maxy);
			} else {
				plot = ncdplot_create (Utilities::to_ncplane (plane), opts, miny, maxy);
			}

			if (plot == nullptr)
				throw init_error ("Notcurses failed to create a new plot");

			take_plane_ownership (plane);
		}

		~PlotBase ()
		{
			if (!is_notcurses_stopped ()) {
				if constexpr (is_double) {
					ncdplot_destroy (plot);
				} else {
					ncuplot_destroy (plot);
				}
			}
		}

		TPlot *get_plot () const noexcept
		{
			return plot;
		}

	private:
		TPlot *plot;
	};

	class NCPP_API_EXPORT PlotU : public PlotBase<ncuplot, uint64_t>
	{
	public:
		static ncplot_options default_options;

	public:
		explicit PlotU (Plane *plane, const ncplot_options *opts = nullptr)
			: PlotU (static_cast<const Plane*>(plane), opts)
		{}

		explicit PlotU (Plane const* plane, const ncplot_options *opts = nullptr)
			: PlotBase (const_cast<Plane*>(plane), opts == nullptr ? &default_options : opts)
		{}

		explicit PlotU (Plane &plane, const ncplot_options *opts = nullptr)
			: PlotU (static_cast<Plane const&>(plane), opts)
		{}

		explicit PlotU (Plane const& plane, const ncplot_options *opts = nullptr)
			: PlotBase (const_cast<Plane*>(&plane), opts == nullptr ? &default_options : opts)
		{}

		Plane* get_plane () const noexcept;
	};

	class NCPP_API_EXPORT PlotD : public PlotBase<ncdplot, double>
	{
	public:
		static ncplot_options default_options;

	public:
		explicit PlotD (Plane *plane, const ncplot_options *opts = nullptr)
			: PlotD (static_cast<const Plane*>(plane), opts)
		{}

		explicit PlotD (Plane const* plane, const ncplot_options *opts = nullptr)
			: PlotBase (const_cast<Plane*>(plane), opts == nullptr ? &default_options : opts)
		{}

		explicit PlotD (Plane &plane, const ncplot_options *opts = nullptr)
			: PlotD (static_cast<Plane const&>(plane), opts)
		{}

		explicit PlotD (Plane const& plane, const ncplot_options *opts = nullptr)
			: PlotBase (const_cast<Plane*>(&plane), opts == nullptr ? &default_options : opts)
		{}

		Plane* get_plane () const noexcept;
	};

	using Plot = PlotU;
}
#endif

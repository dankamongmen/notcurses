#ifndef __NCPP_PLOT_HH
#define __NCPP_PLOT_HH

#include <type_traits>

#include <notcurses/notcurses.h>

#include "Root.hh"
#include "NCAlign.hh"

namespace ncpp
{
	class Plane;

	template<typename TPlot, typename TCoord>
	class NCPP_API_EXPORT PlotBase : public Root
	{
		static constexpr bool is_double = std::is_same_v<TCoord,double>;
		static constexpr bool is_uint64 = std::is_same_v<TCoord,uint64_t>;

	public:
		bool add_sample(TCoord x, TCoord y) const NOEXCEPT_MAYBE
		{
			int ret;

			if constexpr (is_double) {
				ret = ncdplot_add_sample (plot, x, y);
			} else {
				ret = nduplot_add_sample (plot, x, y);
			}

			return error_guard (ret, -1);
		}

		bool set_sample(TCoord x, TCoord y) const NOEXCEPT_MAYBE
		{
			int ret;

			if constexpr (is_double) {
				ret = ncdplot_set_sample (plot, x, y);
			} else {
				ret = nduplot_set_sample (plot, x, y);
			}
			return error_guard (ret, -1);
		}

	protected:
		explicit PlotBase (ncplane *plane, const ncplot_options *opts, TCoord miny = 0, TCoord maxy = 0)
		{
			static_assert (is_double || is_uint64, "PlotBase must be parameterized with either 'double' or 'uint64_t' types");
			if constexpr (is_double) {
				static_assert (std::is_same_v<TPlot, ncdplot>, "ncdplot must be used for a plot using double coordinates");
			} else {
				static_assert (std::is_same_v<TPlot, ncuplot>, "ncuplot must be used for a plot using uint64_t coordinates");
			}

			if (plane == nullptr)
				throw invalid_argument ("'plane' must be a valid pointer");

			if (opts == nullptr)
				throw invalid_argument ("'opts' must be a valid pointer");

			if constexpr (is_uint64) {
				plot = ncuplot_create (plane, opts, miny, maxy);
			} else {
				plot = ncdplot_create (plane, opts, miny, maxy);
			}

			if (plot == nullptr)
				throw init_error ("notcurses failed to create a new plot");
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
			: PlotU (reinterpret_cast<ncplane*>(plane), opts)
		{}

		explicit PlotU (Plane const* plane, const ncplot_options *opts = nullptr)
			: PlotU (const_cast<Plane*>(plane), opts)
		{}

		explicit PlotU (Plane &plane, const ncplot_options *opts = nullptr)
			: PlotU (reinterpret_cast<ncplane*>(&plane), opts)
		{}

		explicit PlotU (Plane const& plane, const ncplot_options *opts = nullptr)
			: PlotU (const_cast<Plane*>(&plane), opts)
		{}

		explicit PlotU (ncplane *plane, const ncplot_options *opts = nullptr,
		                uint64_t miny = 0, uint64_t maxy = 0)
			: PlotBase (plane, opts == nullptr ? &default_options : opts, miny, maxy)
		{}

		Plane* get_plane () const noexcept;
	};

	class NCPP_API_EXPORT PlotD : public PlotBase<ncdplot, double>
	{
	public:
		static ncplot_options default_options;

	public:
		explicit PlotD (Plane *plane, const ncplot_options *opts = nullptr)
			: PlotD (reinterpret_cast<ncplane*>(plane), opts)
		{}

		explicit PlotD (Plane const* plane, const ncplot_options *opts = nullptr)
			: PlotD (const_cast<Plane*>(plane), opts)
		{}

		explicit PlotD (Plane &plane, const ncplot_options *opts = nullptr)
			: PlotD (reinterpret_cast<ncplane*>(&plane), opts)
		{}

		explicit PlotD (Plane const& plane, const ncplot_options *opts = nullptr)
			: PlotD (const_cast<Plane*>(&plane), opts)
		{}

		explicit PlotD (ncplane *plane, const ncplot_options *opts = nullptr,
		                double miny = 0, double maxy = 0)
			: PlotBase (plane, opts == nullptr ? &default_options : opts, miny, maxy)
		{}

		Plane* get_plane () const noexcept;
	};

	using Plot = PlotU;
}
#endif

#ifndef __NCPP_MULTI_SELECTOR_HH
#define __NCPP_MULTI_SELECTOR_HH

#include <notcurses/notcurses.h>

#include "NCAlign.hh"
#include "Plane.hh"
#include "Utilities.hh"
#include "Widget.hh"

namespace ncpp
{
	class NCPP_API_EXPORT MultiSelector : public Widget
	{
	public:
		static ncmultiselector_options default_options;

	public:
		explicit MultiSelector (Plane *plane, const ncmultiselector_options *opts = nullptr)
			: Widget (Utilities::get_notcurses_cpp (plane))
		{
			ensure_valid_plane (plane);
			common_init (Utilities::to_ncplane (plane), opts);
			take_plane_ownership (plane);
		}

		explicit MultiSelector (Plane &plane, const ncmultiselector_options *opts = nullptr)
			: Widget (Utilities::get_notcurses_cpp (plane))
		{
			ensure_valid_plane (plane);
			common_init (Utilities::to_ncplane (plane), opts);
			take_plane_ownership (plane);
		}

		~MultiSelector ()
		{
			if (!is_notcurses_stopped ())
				ncmultiselector_destroy (multiselector);
		}

		bool offer_input (const struct ncinput *ni) const noexcept
		{
			return ncmultiselector_offer_input (multiselector, ni);
		}

		int get_selected (bool *selected, unsigned count) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncmultiselector_selected (multiselector, selected, count), -1);
		}

		Plane* get_plane () const noexcept;

	private:
		void common_init (ncplane *plane, const ncmultiselector_options *opts)
		{
			if (plane == nullptr)
				throw invalid_argument ("'plane' must be a valid pointer");

			multiselector = ncmultiselector_create (plane, opts == nullptr ? &default_options : opts);
			if (multiselector == nullptr)
				throw init_error ("Notcurses failed to create a new multiselector");
		}

	private:
		ncmultiselector *multiselector;
	};
}
#endif

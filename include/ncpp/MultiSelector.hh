#ifndef __NCPP_MULTI_SELECTOR_HH
#define __NCPP_MULTI_SELECTOR_HH

#include <notcurses/notcurses.h>

#include "Root.hh"
#include "NCAlign.hh"

namespace ncpp
{
	class Plane;

	class NCPP_API_EXPORT MultiSelector : public Root
	{
	public:
		static ncmultiselector_options default_options;

	public:
		explicit MultiSelector (Plane *plane, int y, int x, const ncmultiselector_options *opts = nullptr)
			: MultiSelector (reinterpret_cast<ncplane*>(plane), y, x, opts)
		{}

		explicit MultiSelector (Plane const* plane, int y, int x, const ncmultiselector_options *opts = nullptr)
			: MultiSelector (const_cast<Plane*>(plane), y, x, opts)
		{}

		explicit MultiSelector (Plane &plane, int y, int x, const ncmultiselector_options *opts = nullptr)
			: MultiSelector (reinterpret_cast<ncplane*>(&plane), y, x, opts)
		{}

		explicit MultiSelector (Plane const& plane, int y, int x, const ncmultiselector_options *opts = nullptr)
			: MultiSelector (const_cast<Plane*>(&plane), y, x, opts)
		{}

		explicit MultiSelector (ncplane *plane, int y, int x, const ncmultiselector_options *opts = nullptr)
		{
			if (plane == nullptr)
				throw invalid_argument ("'plane' must be a valid pointer");

			multiselector = ncmultiselector_create (plane, y, x, opts == nullptr ? &default_options : opts);
			if (multiselector == nullptr)
				throw init_error ("Notcurses failed to create a new multiselector");
		}

		~MultiSelector ()
		{
			if (!is_notcurses_stopped ())
				ncmultiselector_destroy (multiselector, nullptr);
		}

		bool offer_input (const struct ncinput *nc) const noexcept
		{
			return ncmultiselector_offer_input (multiselector, nc);
		}

		int get_selected (bool *selected, unsigned count) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncmultiselector_selected (multiselector, selected, count), -1);
		}

		Plane* get_plane () const noexcept;

	private:
		ncmultiselector *multiselector;
	};
}
#endif

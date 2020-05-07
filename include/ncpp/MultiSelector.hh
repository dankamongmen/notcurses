#ifndef __NCPP_MULTI_SELECTOR_HH
#define __NCPP_MULTI_SELECTOR_HH

#include <notcurses/notcurses.h>

#include "Root.hh"
#include "NCAlign.hh"
#include "NotCurses.hh"

namespace ncpp
{
	class Plane;

	class NCPP_API_EXPORT MultiSelector : public Root
	{
	public:
		static multiselector_options default_options;

	public:
		explicit MultiSelector (NotCurses *nc, int y, int x, const multiselector_options *opts = nullptr)
			: MultiSelector (reinterpret_cast<notcurses*>(nc), y, x, opts)
		{}

		explicit MultiSelector (NotCurses const* nc, int y, int x, const multiselector_options *opts = nullptr)
			: MultiSelector (const_cast<NotCurses*>(nc), y, x, opts)
		{}

		explicit MultiSelector (NotCurses &nc, int y, int x, const multiselector_options *opts = nullptr)
			: MultiSelector (reinterpret_cast<notcurses*>(&nc), y, x, opts)
		{}

		explicit MultiSelector (NotCurses const& nc, int y, int x, const multiselector_options *opts = nullptr)
			: MultiSelector (const_cast<NotCurses*>(&nc), y, x, opts)
		{}

		explicit MultiSelector (notcurses *nc, int y, int x, const multiselector_options *opts = nullptr)
		{
			multiselector = ncmultiselector_create (nc, y, x, opts == nullptr ? &default_options : opts);
			if (multiselector == nullptr)
				throw init_error ("notcurses failed to create a new multiselector");
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

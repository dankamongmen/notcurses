#ifndef __NCPP_MENU_HH
#define __NCPP_MENU_HH

#include <notcurses.h>

#include "Root.hh"

namespace ncpp
{
	class Plane;

	class NCPP_API_EXPORT Menu : public Root
	{
	public:
		static ncmenu_options default_options;

	public:
		explicit Menu (const ncmenu_options *opts = nullptr)
		{
			menu = ncmenu_create (get_notcurses (), opts == nullptr ? &default_options : opts);
			if (menu == nullptr)
				throw new init_error ("notcurses failed to create a new menu");
		}

		~Menu ()
		{
			if (!is_notcurses_stopped ())
				ncmenu_destroy (menu);
		}

		bool unroll (int sectionidx) const noexcept
		{
			return ncmenu_unroll (menu, sectionidx) >= 0;
		}

		bool rollup () const noexcept
		{
			return ncmenu_rollup (menu) >= 0;
		}

		bool nextsection () const noexcept
		{
			return ncmenu_nextsection (menu) >= 0;
		}

		bool prevsection () const noexcept
		{
			return ncmenu_prevsection (menu) >= 0;
		}

		bool nextitem () const noexcept
		{
			return ncmenu_nextitem (menu) >= 0;
		}

		bool previtem () const noexcept
		{
			return ncmenu_previtem (menu) >= 0;
		}

		const char* get_selected (ncinput *ni = nullptr) const noexcept
		{
			return ncmenu_selected (menu, ni);
		}

		bool offer_input (const struct ncinput* nc) const noexcept
		{
			return ncmenu_offer_input (menu, nc);
		}

		Plane* get_plane () const noexcept;

	private:
		ncmenu *menu;
	};
}
#endif

#ifndef __NCPP_MENU_HH
#define __NCPP_MENU_HH

#include <notcurses/notcurses.h>

#include "Root.hh"

namespace ncpp
{
	class NotCurses;
	class Plane;

	class NCPP_API_EXPORT Menu : public Root
	{
	public:
		static ncmenu_options default_options;

	public:
		explicit Menu (const ncmenu_options *opts = nullptr, NotCurses *ncinst = nullptr)
			: Root (ncinst)
		{
			menu = ncmenu_create (notcurses_stdplane (get_notcurses ()), opts == nullptr ? &default_options : opts);
			if (menu == nullptr)
				throw init_error ("Notcurses failed to create a new menu");
		}

		~Menu ()
		{
			if (!is_notcurses_stopped ())
				ncmenu_destroy (menu);
		}

		bool unroll (int sectionidx) const NOEXCEPT_MAYBE
		{
			return error_guard (ncmenu_unroll (menu, sectionidx), -1);
		}

		bool rollup () const NOEXCEPT_MAYBE
		{
			return error_guard (ncmenu_rollup (menu), -1);
		}

		bool nextsection () const NOEXCEPT_MAYBE
		{
			return error_guard (ncmenu_nextsection (menu), -1);
		}

		bool prevsection () const NOEXCEPT_MAYBE
		{
			return error_guard (ncmenu_prevsection (menu), -1);
		}

		bool nextitem () const NOEXCEPT_MAYBE
		{
			return error_guard (ncmenu_nextitem (menu), -1);
		}

		bool previtem () const NOEXCEPT_MAYBE
		{
			return error_guard (ncmenu_previtem (menu), -1);
		}

		bool item_set_status (const char* section, const char* item, bool status) const NOEXCEPT_MAYBE
		{
			return error_guard (ncmenu_item_set_status (menu, section, item, status), -1);
		}

		const char* get_selected (ncinput *ni = nullptr) const noexcept
		{
			return ncmenu_selected (menu, ni);
		}

		const char* get_mouse_selected (const struct ncinput* click, struct ncinput* ni) const noexcept
		{
			return ncmenu_mouse_selected (menu, click, ni);
		}

		bool offer_input (const struct ncinput* ni) const noexcept
		{
			return ncmenu_offer_input (menu, ni);
		}

		Plane* get_plane () const noexcept;

	private:
		ncmenu *menu;
	};
}
#endif

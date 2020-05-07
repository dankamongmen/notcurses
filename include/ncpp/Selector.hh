#ifndef __NCPP_SELECTOR_HH
#define __NCPP_SELECTOR_HH

#include <notcurses/notcurses.h>

#include "Root.hh"
#include "NCAlign.hh"
#include "NotCurses.hh"

namespace ncpp
{
	class Plane;

	class NCPP_API_EXPORT Selector : public Root
	{
	public:
		static selector_options default_options;

	public:
		explicit Selector (NotCurses *nc, int y, int x, const selector_options *opts = nullptr)
			: Selector (reinterpret_cast<notcurses*>(nc), y, x, opts)
		{}

		explicit Selector (NotCurses const* nc, int y, int x, const selector_options *opts = nullptr)
			: Selector (const_cast<NotCurses*>(nc), y, x, opts)
		{}

		explicit Selector (NotCurses &nc, int y, int x, const selector_options *opts = nullptr)
			: Selector (reinterpret_cast<NotCurses*>(&nc), y, x, opts)
		{}

		explicit Selector (NotCurses const& nc, int y, int x, const selector_options *opts = nullptr)
			: Selector (const_cast<NotCurses*>(&nc), y, x, opts)
		{}

		explicit Selector (notcurses* nc, int y, int x, const selector_options *opts = nullptr)
		{
			selector = ncselector_create (nc, y, x, opts == nullptr ? &default_options : opts);
			if (selector == nullptr)
				throw init_error ("notcurses failed to create a new selector");
		}

		~Selector ()
		{
			if (!is_notcurses_stopped ())
				ncselector_destroy (selector, nullptr);
		}

		int additem (const selector_item *item) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncselector_additem (selector, item), -1);
		}

		int delitem (const char *item) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncselector_delitem (selector, item), -1);
		}

		const char* previtem () const noexcept
		{
			return ncselector_previtem (selector);
		}

		const char* nextitem () const noexcept
		{
			return ncselector_nextitem (selector);
		}

		const char* get_selected () const noexcept
		{
			return ncselector_selected (selector);
		}

		bool offer_input (const struct ncinput* nc) const noexcept
		{
			return ncselector_offer_input (selector, nc);
		}

		Plane* get_plane () const noexcept;

	private:
		ncselector *selector;
	};
}
#endif

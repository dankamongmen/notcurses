#ifndef __NCPP_SELECTOR_HH
#define __NCPP_SELECTOR_HH

#include <notcurses.h>

#include "Root.hh"
#include "NCAlign.hh"

namespace ncpp
{
	class Plane;

	class NCPP_API_EXPORT Selector : public Root
	{
	public:
		static selector_options default_options;

	public:
		explicit Selector (Plane *plane, int y, int x, const selector_options *opts = nullptr)
			: Selector (reinterpret_cast<ncplane*>(plane), y, x, opts)
		{}

		explicit Selector (Plane const* plane, int y, int x, const selector_options *opts = nullptr)
			: Selector (const_cast<Plane*>(plane), y, x, opts)
		{}

		explicit Selector (Plane &plane, int y, int x, const selector_options *opts = nullptr)
			: Selector (reinterpret_cast<ncplane*>(&plane), y, x, opts)
		{}

		explicit Selector (Plane const& plane, int y, int x, const selector_options *opts = nullptr)
			: Selector (const_cast<Plane*>(&plane), y, x, opts)
		{}

		explicit Selector (ncplane *plane, int y, int x, const selector_options *opts = nullptr)
		{
			if (plane == nullptr)
				throw new invalid_argument ("'plane' must be a valid pointer");

			selector = ncselector_create (plane, y, x, opts == nullptr ? &default_options : opts);
			if (selector == nullptr)
				throw new init_error ("notcurses failed to create a new selector");
		}

		~Selector ()
		{
			if (!is_notcurses_stopped ())
				ncselector_destroy (selector, nullptr);
		}

		bool additem (const selector_item *item) const noexcept
		{
			return ncselector_additem (selector, item) >= 0;
		}

		bool delitem (const char *item) const noexcept
		{
			return ncselector_delitem (selector, item) >= 0;
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

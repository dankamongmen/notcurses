#ifndef __NCPP_REEL_HH
#define __NCPP_REEL_HH

#include <memory>
#include <notcurses/notcurses.h>

#include "Tablet.hh"
#include "Plane.hh"
#include "Utilities.hh"
#include "Widget.hh"

namespace ncpp
{
	class NCPP_API_EXPORT NcReel : public Widget
	{
	public:
		static ncreel_options default_options;

		explicit NcReel (Plane &plane, const ncreel_options *popts = nullptr)
			: Widget (Utilities::get_notcurses_cpp (plane))
		{
			ensure_valid_plane (plane);
			common_init (Utilities::to_ncplane (plane), popts);
			take_plane_ownership (plane);
		}

		explicit NcReel (Plane *plane, const ncreel_options *popts = nullptr)
			: Widget (Utilities::get_notcurses_cpp (plane))
		{
			if (plane == nullptr)
				throw invalid_argument ("'plane' must be a valid pointer");

			ensure_valid_plane (plane);
			common_init (Utilities::to_ncplane (plane), popts);
			take_plane_ownership (plane);
		}

		~NcReel ()
		{
			if (!is_notcurses_stopped ())
				ncreel_destroy (reel);
		}

		operator ncreel* () const noexcept
		{
			return reel;
		}

		operator ncreel const* () const noexcept
		{
			return reel;
		}

		// TODO: add an overload using callback that takes NcTablet instance instead of struct tablet
		NcTablet* add (NcTablet *after, NcTablet *before, tabletcb cb, void *opaque = nullptr) const
		{
			nctablet *t = ncreel_add (reel, get_tablet (after), get_tablet (before), cb, opaque);
			if (t == nullptr)
				throw init_error ("Notcurses failed to create a new tablet");

			return NcTablet::map_tablet (t, get_notcurses_cpp ());
		}

		NcTablet* add (NcTablet &after, NcTablet &before, tabletcb cb, void *opaque = nullptr) const noexcept
		{
			return add (&after, &before, cb, opaque);
		}

		int get_tabletcount () const noexcept
		{
			return ncreel_tabletcount (reel);
		}

		bool del (NcTablet *t) const NOEXCEPT_MAYBE
		{
			return error_guard (ncreel_del (reel, get_tablet (t)), -1);
		}

		bool del (NcTablet &t) const NOEXCEPT_MAYBE
		{
			return del (&t);
		}

		bool redraw () const NOEXCEPT_MAYBE
		{
			return error_guard (ncreel_redraw (reel), -1);
		}

		NcTablet* get_focused () const noexcept
		{
			nctablet *t = ncreel_focused (reel);
			if (t == nullptr)
				return nullptr;

			return NcTablet::map_tablet (t, get_notcurses_cpp ());
		}

		NcTablet* next () const noexcept
		{
			nctablet *t = ncreel_next (reel);
			if (t == nullptr)
				return nullptr;

			return NcTablet::map_tablet (t, get_notcurses_cpp ());
		}

		NcTablet* prev () const noexcept
		{
			nctablet *t = ncreel_prev (reel);
			if (t == nullptr)
				return nullptr;

			return NcTablet::map_tablet (t, get_notcurses_cpp ());
		}

		bool offer_input (const struct ncinput* nci) const NOEXCEPT_MAYBE
		{
			return error_guard<bool, bool> (ncreel_offer_input (reel, nci), false);
		}

		Plane* get_plane () const noexcept;

	private:
		nctablet* get_tablet (NcTablet *t) const noexcept
		{
			if (t == nullptr)
				return nullptr;

			return t->get_tablet ();
		}

		void common_init (ncplane *plane, const ncreel_options *popts = nullptr)
		{
			reel = ncreel_create (plane, popts == nullptr ? &default_options : popts);
			if (reel == nullptr)
				throw init_error ("Notcurses failed to create a new ncreel");
		}

	private:
		ncreel *reel = nullptr;

		friend class Plane;
	};
}
#endif

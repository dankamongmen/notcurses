#ifndef __NCPP_REEL_HH
#define __NCPP_REEL_HH

#include <memory>
#include <notcurses/notcurses.h>

#include "Tablet.hh"
#include "Root.hh"

namespace ncpp
{
	class Plane;

	class NCPP_API_EXPORT NcReel : public Root
	{
	public:
		static ncreel_options default_options;

		explicit NcReel (Plane &plane, const ncreel_options *popts = nullptr, int efd = -1)
			: NcReel (&plane, popts, efd)
		{}

		explicit NcReel (Plane *plane, const ncreel_options *popts = nullptr, int efd = -1)
		{
			if (plane == nullptr)
				throw invalid_argument ("'plane' must be a valid pointer");

			create_reel (reinterpret_cast<ncplane*>(plane), popts, efd);
		}

		explicit NcReel (ncplane *plane, const ncreel_options *popts = nullptr, int efd = -1)
		{
			if (plane == nullptr)
				throw invalid_argument ("'plane' must be a valid pointer");

			create_reel (plane, popts, efd);
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
				throw init_error ("notcurses failed to create a new tablet");

			return NcTablet::map_tablet (t);
		}

		NcTablet* add (NcTablet &after, NcTablet &before, tabletcb cb, void *opaque = nullptr) const noexcept
		{
			return add (&after, &before, cb, opaque);
		}

		int get_tabletcount () const noexcept
		{
			return ncreel_tabletcount (reel);
		}

		bool touch (NcTablet *t) const NOEXCEPT_MAYBE
		{
			return error_guard (ncreel_touch (reel, get_tablet (t)), -1);
		}

		bool touch (NcTablet &t) const NOEXCEPT_MAYBE
		{
			return touch (&t);
		}

		bool del (NcTablet *t) const NOEXCEPT_MAYBE
		{
			return error_guard (ncreel_del (reel, get_tablet (t)), -1);
		}

		bool del (NcTablet &t) const NOEXCEPT_MAYBE
		{
			return del (&t);
		}

		bool del_focused () const NOEXCEPT_MAYBE
		{
			return error_guard (ncreel_del_focused (reel), -1);
		}

		bool move (int x, int y) const NOEXCEPT_MAYBE
		{
			return error_guard (ncreel_move (reel, x, y), -1);
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

			return NcTablet::map_tablet (t);
		}

		NcTablet* next () const noexcept
		{
			nctablet *t = ncreel_next (reel);
			if (t == nullptr)
				return nullptr;

			return NcTablet::map_tablet (t);
		}

		NcTablet* prev () const noexcept
		{
			nctablet *t = ncreel_prev (reel);
			if (t == nullptr)
				return nullptr;

			return NcTablet::map_tablet (t);
		}

		Plane* get_plane () const noexcept;

	private:
		nctablet* get_tablet (NcTablet *t) const noexcept
		{
			if (t == nullptr)
				return nullptr;

			return t->get_tablet ();
		}

		void create_reel (ncplane *plane, const ncreel_options *popts, int efd)
		{
			reel = ncreel_create (plane, popts == nullptr ? &default_options : popts, efd);
			if (reel == nullptr)
				throw init_error ("notcurses failed to create a new ncreel");
		}

	private:
		ncreel *reel = nullptr;

		friend class Plane;
	};
}
#endif

#ifndef __NCPP_PANEL_REEL_HH
#define __NCPP_PANEL_REEL_HH

#include <memory>
#include <notcurses.h>

#include "Tablet.hh"
#include "Root.hh"

namespace ncpp
{
	class Plane;

	class NCPP_API_EXPORT PanelReel : public Root
	{
	public:
		static panelreel_options default_options;

		explicit PanelReel (Plane *plane, const panelreel_options *popts, int efd)
		{
			if (plane == nullptr)
				throw new invalid_argument ("'plane' must be a valid pointer");

			create_reel (reinterpret_cast<ncplane*>(plane), popts, efd);
		}

		explicit PanelReel (ncplane *plane, const panelreel_options *popts, int efd)
		{
			if (plane == nullptr)
				throw new invalid_argument ("'plane' must be a valid pointer");

			create_reel (plane, popts, efd);
		}

		~PanelReel ()
		{
			if (!is_notcurses_stopped ())
				panelreel_destroy (reel);
		}

		operator panelreel* () const noexcept
		{
			return reel;
		}

		operator panelreel const* () const noexcept
		{
			return reel;
		}

		// TODO: add an overload using callback that takes Tablet instance instead of struct tablet
		Tablet* add (Tablet *after, Tablet *before, tabletcb cb, void *opaque = nullptr) const
		{
			tablet *t = panelreel_add (reel, get_tablet (after), get_tablet (before), cb, opaque);
			if (t == nullptr)
				throw new init_error ("notcurses failed to create a new tablet");

			return Tablet::map_tablet (t);
		}

		Tablet* add (Tablet &after, Tablet &before, tabletcb cb, void *opaque = nullptr) const noexcept
		{
			return add (&after, &before, cb, opaque);
		}

		int get_tabletcount () const noexcept
		{
			return panelreel_tabletcount (reel);
		}

		bool touch (Tablet *t) const noexcept
		{
			return panelreel_touch (reel, get_tablet (t)) != -1;
		}

		bool touch (Tablet &t) const noexcept
		{
			return touch (&t);
		}

		bool del (Tablet *t) const noexcept
		{
			return panelreel_del (reel, get_tablet (t)) != -1;
		}

		bool del (Tablet &t) const noexcept
		{
			return del (&t);
		}

		bool del_focused () const noexcept
		{
			return panelreel_del_focused (reel) != -1;
		}

		bool move (int x, int y) const noexcept
		{
			return panelreel_move (reel, x, y) != -1;
		}

		bool redraw () const noexcept
		{
			return panelreel_redraw (reel) != -1;
		}

		Tablet* get_focused () const noexcept
		{
			tablet *t = panelreel_focused (reel);
			if (t == nullptr)
				return nullptr;

			return Tablet::map_tablet (t);
		}

		Tablet* next () const noexcept
		{
			tablet *t = panelreel_next (reel);
			if (t == nullptr)
				return nullptr;

			return Tablet::map_tablet (t);
		}

		Tablet* prev () const noexcept
		{
			tablet *t = panelreel_prev (reel);
			if (t == nullptr)
				return nullptr;

			return Tablet::map_tablet (t);
		}

		Plane* get_plane () const noexcept;

	private:
		tablet* get_tablet (Tablet *t) const noexcept
		{
			if (t == nullptr)
				return nullptr;

			return t->get_tablet ();
		}

		void create_reel (ncplane *plane, const panelreel_options *popts, int efd)
		{
			reel = panelreel_create (plane, popts == nullptr ? &default_options : popts, efd);
			if (reel == nullptr)
				throw new init_error ("notcurses failed to create a new panelreel");
		}

	private:
		panelreel *reel = nullptr;

		friend class Plane;
	};
}
#endif

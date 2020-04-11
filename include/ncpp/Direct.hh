#ifndef __NCPP_DIRECT_HH
#define __NCPP_DIRECT_HH

#include <cstdio>
#include <notcurses/notcurses.h>

#include "Root.hh"
#include "Cell.hh"

namespace ncpp
{
	class NCPP_API_EXPORT Direct : public Root
	{
	public:
		explicit Direct (const char *termtype = nullptr, FILE *fp = nullptr)
		{
			direct = ncdirect_init (termtype, fp == nullptr ? stdout : fp);
			if (direct == nullptr)
				throw init_error ("notcurses failed to initialize direct mode");
		}

		~Direct ()
		{
			ncdirect_stop (direct);
		}

		bool clear () const noexcept
		{
			return ncdirect_clear (direct) >= 0;
		}

		bool set_fg_default () const noexcept
		{
			return ncdirect_fg_default (direct) >= 0;
		}

		bool set_fg (unsigned rgb) const noexcept
		{
			return ncdirect_fg (direct, rgb) >= 0;
		}

		bool set_fg (unsigned r, unsigned g, unsigned b) const noexcept
		{
			return ncdirect_fg_rgb8 (direct, r, g, b) >= 0;
		}

		bool set_bg_default () const noexcept
		{
			return ncdirect_bg_default (direct) >= 0;
		}

		bool set_bg (unsigned rgb) const noexcept
		{
			return ncdirect_bg (direct, rgb) >= 0;
		}

		bool set_bg (unsigned r, unsigned g, unsigned b) const noexcept
		{
			return ncdirect_bg_rgb8 (direct, r, g, b) >= 0;
		}

		int get_dim_x () const noexcept
		{
			return ncdirect_dim_x (direct);
		}

		int get_dim_y () const noexcept
		{
			return ncdirect_dim_y (direct);
		}

		void styles_set (CellStyle stylebits) const noexcept
		{
			ncdirect_styles_set (direct, static_cast<unsigned>(stylebits));
		}

		void styles_on (CellStyle stylebits) const noexcept
		{
			ncdirect_styles_on (direct, static_cast<unsigned>(stylebits));
		}

		void styles_off (CellStyle stylebits) const noexcept
		{
			ncdirect_styles_off (direct, static_cast<unsigned>(stylebits));
		}

		int cursor_move_yx (int y, int x) const noexcept
		{
			return ncdirect_cursor_move_yx (direct, y, x);
		}

		int cursor_up (int num) const noexcept
		{
			return ncdirect_cursor_up (direct, num);
		}

		int cursor_left (int num) const noexcept
		{
			return ncdirect_cursor_left (direct, num);
		}

		int cursor_right (int num) const noexcept
		{
			return ncdirect_cursor_right (direct, num);
		}

		int cursor_down (int num) const noexcept
		{
			return ncdirect_cursor_down (direct, num);
		}

		bool cursor_enable () const noexcept
		{
			return ncdirect_cursor_enable (direct) != -1;
		}

		bool cursor_disable () const noexcept
		{
			return ncdirect_cursor_disable (direct) != -1;
		}

	private:
		ncdirect *direct;
	};
}
#endif

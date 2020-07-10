#ifndef __NCPP_DIRECT_HH
#define __NCPP_DIRECT_HH

#include <cstdio>
#include <notcurses/direct.h>
#include <notcurses/notcurses.h>

#include "Root.hh"
#include "Cell.hh"

namespace ncpp
{
	class NotCurses;

	class NCPP_API_EXPORT Direct : public Root
	{
	public:
		explicit Direct (const char *termtype = nullptr, FILE *fp = nullptr, NotCurses *ncinst = nullptr)
			: Root (ncinst)
		{
			direct = ncdirect_init (termtype, fp == nullptr ? stdout : fp);
			if (direct == nullptr)
				throw init_error ("Notcurses failed to initialize direct mode");
		}

		~Direct ()
		{
			ncdirect_stop (direct);
		}

		bool clear () const NOEXCEPT_MAYBE
		{
			return error_guard (ncdirect_clear (direct), -1);
		}

		bool set_fg_default () const NOEXCEPT_MAYBE
		{
			return error_guard (ncdirect_fg_default (direct), -1);
		}

		bool set_fg (unsigned rgb) const NOEXCEPT_MAYBE
		{
			return error_guard (ncdirect_fg (direct, rgb), -1);
		}

		bool set_fg (unsigned r, unsigned g, unsigned b) const NOEXCEPT_MAYBE
		{
			return error_guard (ncdirect_fg_rgb (direct, r, g, b), -1);
		}

		bool set_bg_default () const NOEXCEPT_MAYBE
		{
			return error_guard (ncdirect_bg_default (direct), -1);
		}

		bool set_bg (unsigned rgb) const NOEXCEPT_MAYBE
		{
			return error_guard (ncdirect_bg (direct, rgb), -1);
		}

		bool set_bg (unsigned r, unsigned g, unsigned b) const NOEXCEPT_MAYBE
		{
			return error_guard (ncdirect_bg_rgb (direct, r, g, b), -1);
		}

		int get_dim_x () const NOEXCEPT_MAYBE
		{
			return error_guard (ncdirect_dim_x (direct), -1);
		}

		int get_dim_y () const NOEXCEPT_MAYBE
		{
			return error_guard (ncdirect_dim_y (direct), -1);
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

		bool cursor_move_yx (int y, int x) const NOEXCEPT_MAYBE
		{
			return error_guard (ncdirect_cursor_move_yx (direct, y, x), -1);
		}

		bool cursor_up (int num) const NOEXCEPT_MAYBE
		{
			return error_guard (ncdirect_cursor_up (direct, num), -1);
		}

		bool cursor_left (int num) const NOEXCEPT_MAYBE
		{
			return error_guard (ncdirect_cursor_left (direct, num), -1);
		}

		bool cursor_right (int num) const NOEXCEPT_MAYBE
		{
			return error_guard (ncdirect_cursor_right (direct, num), -1);
		}

		bool cursor_down (int num) const NOEXCEPT_MAYBE
		{
			return error_guard (ncdirect_cursor_down (direct, num), -1);
		}

		bool cursor_enable () const NOEXCEPT_MAYBE
		{
			return error_guard (ncdirect_cursor_enable (direct), -1);
		}

		bool cursor_disable () const NOEXCEPT_MAYBE
		{
			return error_guard (ncdirect_cursor_disable (direct), -1);
		}

		nc_err_e render_image (const char* file, ncalign_e align, ncblitter_e blitter, ncscale_e scale) const noexcept
		{
			return ncdirect_render_image (direct, file, align, blitter, scale);
		}

	private:
		ncdirect *direct;
	};
}
#endif

#ifndef __NCPP_DIRECT_HH
#define __NCPP_DIRECT_HH

#include <cstdio>
#include <notcurses/direct.h>
#include <notcurses/notcurses.h>

#include "Root.hh"
#include "Cell.hh"
#include "NCAlign.hh"

namespace ncpp
{
	class NotCurses;

	class NCPP_API_EXPORT Direct : public Root
	{
	public:
		explicit Direct (const char *termtype = nullptr, FILE *fp = nullptr,
				             uint64_t flags = 0, NotCurses *ncinst = nullptr)
			: Root (ncinst)
		{
			direct = ncdirect_init (termtype, fp == nullptr ? stdout : fp, flags);
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
			return error_guard (ncdirect_set_fg_default (direct), -1);
		}

		bool set_fg_rgb (unsigned rgb) const NOEXCEPT_MAYBE
		{
			return error_guard (ncdirect_set_fg_rgb (direct, rgb), -1);
		}

		bool set_fg_rgb (unsigned r, unsigned g, unsigned b) const NOEXCEPT_MAYBE
		{
			return error_guard (ncdirect_set_fg_rgb8 (direct, r, g, b), -1);
		}

		bool fg_palindex (int pidx) const NOEXCEPT_MAYBE
		{
			return error_guard (ncdirect_set_fg_palindex (direct, pidx), -1);
		}

		bool set_bg_default () const NOEXCEPT_MAYBE
		{
			return error_guard (ncdirect_set_bg_default (direct), -1);
		}

		bool set_bg_rgb (unsigned rgb) const NOEXCEPT_MAYBE
		{
			return error_guard (ncdirect_set_bg_rgb (direct, rgb), -1);
		}

		bool set_bg_rgb (unsigned r, unsigned g, unsigned b) const NOEXCEPT_MAYBE
		{
			return error_guard (ncdirect_set_bg_rgb8 (direct, r, g, b), -1);
		}

		bool bg_palindex (int pidx) const NOEXCEPT_MAYBE
		{
			return error_guard (ncdirect_set_bg_palindex (direct, pidx), -1);
		}

		unsigned get_dim_x () const NOEXCEPT_MAYBE
		{
			return ncdirect_dim_x (direct);
		}

		unsigned get_dim_y () const NOEXCEPT_MAYBE
		{
			return ncdirect_dim_y (direct);
		}

		unsigned get_palette_size () const noexcept
		{
			return ncdirect_palette_size (direct);
		}

		void styles_set (CellStyle stylebits) const noexcept
		{
			ncdirect_set_styles (direct, static_cast<unsigned>(stylebits));
		}

		void styles_on (CellStyle stylebits) const noexcept
		{
			ncdirect_on_styles (direct, static_cast<unsigned>(stylebits));
		}

		void styles_off (CellStyle stylebits) const noexcept
		{
			ncdirect_off_styles (direct, static_cast<unsigned>(stylebits));
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

		void get_cursor_yx (unsigned *y, unsigned *x) const noexcept
		{
			ncdirect_cursor_yx (direct, y, x);
		}

		void get_cursor_yx (unsigned &y, unsigned &x) const noexcept
		{
			get_cursor_yx (&y, &x);
		}

		int render_image (const char* file, NCAlign align, ncblitter_e blitter, ncscale_e scale) const noexcept
		{
			return ncdirect_render_image (direct, file, static_cast<ncalign_e>(align), blitter, scale);
		}

		ncdirectv* prep_image (const char* file, ncblitter_e blitter, ncscale_e scale, int maxy, int maxx) const noexcept
		{
			return ncdirect_render_frame (direct, file, blitter, scale, maxy, maxx);
		}

		int raster_image (ncdirectv* faken, NCAlign align) const noexcept
		{
			return ncdirect_raster_frame (direct, faken, static_cast<ncalign_e>(align));
		}

		int streamfile (const char* filename, ncstreamcb streamer, struct ncvisual_options* vopts, void* curry) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncdirect_stream(direct, filename, streamer, vopts, curry), -1);
		}

		bool putstr (uint64_t channels, const char* utf8) const NOEXCEPT_MAYBE
		{
			return error_guard (ncdirect_putstr (direct, channels, utf8), -1);
		}

		// TODO: ncdirect_printf_aligned (will need a version which takes vargs)

		int hline_interp (const char* egc, unsigned len, uint64_t h1, uint64_t h2) const noexcept
		{
			return ncdirect_hline_interp (direct, egc, len, h1, h2);
		}

		int vline_interp (const char* egc, unsigned len, uint64_t h1, uint64_t h2) const noexcept
		{
			return ncdirect_vline_interp (direct, egc, len, h1, h2);
		}

		bool box (uint64_t ul, uint64_t ur, uint64_t ll, uint64_t lr, const wchar_t* wchars, int ylen, int xlen, unsigned ctlword) const NOEXCEPT_MAYBE
		{
			return error_guard (ncdirect_box (direct, ul, ur, ll, lr, wchars, ylen, xlen, ctlword), -1);
		}

		bool rounded_box (uint64_t ul, uint64_t ur, uint64_t ll, uint64_t lr, int ylen, int xlen, unsigned ctlword) const NOEXCEPT_MAYBE
		{
			return error_guard (ncdirect_rounded_box (direct, ul, ur, ll, lr, ylen, xlen, ctlword), -1);
		}

		bool double_box (uint64_t ul, uint64_t ur, uint64_t ll, uint64_t lr, int ylen, int xlen, unsigned ctlword) const NOEXCEPT_MAYBE
		{
			return error_guard (ncdirect_double_box (direct, ul, ur, ll, lr, ylen, xlen, ctlword), -1);
		}

		bool flush () const NOEXCEPT_MAYBE
		{
			return error_guard (ncdirect_flush (direct), -1);
		}

		char32_t get (ncinput *ni, bool blocking) const noexcept
		{
			if (blocking)
				return ncdirect_get_blocking (direct, ni);
			return ncdirect_get_nblock (direct, ni);
		}

		char32_t get (const struct timespec *ts, ncinput *ni) const noexcept
		{
			return ncdirect_get (direct, ts, ni);
		}

		int get_inputready_fd () const noexcept
		{
			return ncdirect_inputready_fd (direct);
		}

		bool canopen_images () const noexcept
		{
			return ncdirect_canopen_images (direct);
		}

		bool canutf8 () const noexcept
		{
			return ncdirect_canutf8 (direct);
		}

		int check_pixel_support() noexcept
		{
			return ncdirect_check_pixel_support (direct);
		}

	private:
		ncdirect *direct;
	};
}
#endif

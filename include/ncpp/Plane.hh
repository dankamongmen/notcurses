#ifndef __NCPP_PLANE_HH
#define __NCPP_PLANE_HH

#include <exception>
#include <cstdarg>
#include <ctime>
#include <map>
#include <mutex>
#include <notcurses/notcurses.h>

#include "Root.hh"
#include "Cell.hh"
#include "Visual.hh"
#include "Reel.hh"
#include "CellStyle.hh"
#include "NCAlign.hh"
#include "NCBox.hh"

namespace ncpp
{
	class NCPP_API_EXPORT Plane : public Root
	{
	public:
		Plane (Plane const& other)
		{
			plane = duplicate_plane (other, nullptr);
		}

		explicit Plane (Plane const& other, void *opaque)
		{
			plane = duplicate_plane (other, opaque);
		}

		explicit Plane (Plane *n, int rows, int cols, int yoff, int xoff, void *opaque = nullptr)
			: Plane (static_cast<const Plane*>(n), rows, cols, yoff, xoff, opaque)
		{}

		explicit Plane (const Plane *n, int rows, int cols, int yoff, int xoff, void *opaque = nullptr)
		{
			if (n == nullptr)
				throw invalid_argument ("'n' must be a valid pointer");

			plane = create_plane (*n, rows, cols, yoff, xoff, opaque);
		}

		explicit Plane (const Plane &n, int rows, int cols, int yoff, int xoff, void *opaque = nullptr)
		{
			plane = create_plane (n, rows, cols, yoff, xoff, opaque);
		}

		explicit Plane (int rows, int cols, int yoff, int xoff, void *opaque = nullptr)
		{
			plane = ncplane_new (
				get_notcurses (),
				rows,
				cols,
				yoff,
				xoff,
				opaque
			);

			if (plane == nullptr)
				throw init_error ("notcurses failed to create a new plane");

			map_plane (plane, this);
		}

		explicit Plane (Plane &n, int rows, int cols, int yoff, NCAlign align, void *opaque = nullptr)
		{
			plane = create_plane (n, rows, cols, yoff, align, opaque);
		}

		explicit Plane (Plane const& n, int rows, int cols, int yoff, NCAlign align, void *opaque = nullptr)
		{
			plane = create_plane (const_cast<Plane&>(n), rows, cols, yoff, align, opaque);
		}

		explicit Plane (Plane *n, int rows, int cols, int yoff, NCAlign align, void *opaque = nullptr)
		{
			if (n == nullptr)
				throw invalid_argument ("'n' must be a valid pointer");

			plane = create_plane (*n, rows, cols, yoff, align, opaque);
		}

		explicit Plane (Plane const* n, int rows, int cols, int yoff, NCAlign align, void *opaque = nullptr)
		{
			if (n == nullptr)
				throw invalid_argument ("'n' must be a valid pointer");

			plane = create_plane (const_cast<Plane&>(*n), rows, cols, yoff, align, opaque);
		}

		explicit Plane (ncplane *_plane) noexcept
			: plane (_plane)
		{}

		~Plane () noexcept
		{
			if (is_stdplane)
				return;

			if (!is_notcurses_stopped ())
				ncplane_destroy (plane);
			unmap_plane (this);
		}

		operator ncplane* () noexcept
		{
			return plane;
		}

		operator ncplane const* () const noexcept
		{
			return plane;
		}

		bool resize (int keepy, int keepx, int keepleny, int keeplenx, int yoff, int xoff, int ylen, int xlen) const NOEXCEPT_MAYBE
		{
			int ret = ncplane_resize (
				plane,
				keepy,
				keepx,
				keepleny,
				keeplenx,
				yoff,
				xoff,
				ylen,
				xlen
			);

			return error_guard (ret, -1);
		}

		bool pulse (const timespec* ts, fadecb fader, void* curry) const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_pulse (plane, ts, fader, curry), -1);
		}

		int gradient (const char* egc, uint32_t attrword, uint64_t ul, uint64_t ur, uint64_t ll, uint64_t lr, int ystop, int xstop) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_gradient (plane, egc, attrword, ul, ur, ll, lr, ystop, xstop), -1);
		}

		int gradient_sized (const char* egc, uint32_t attrword, uint64_t ul, uint64_t ur, uint64_t ll, uint64_t lr, int ylen, int xlen) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_gradient_sized (plane, egc, attrword, ul, ur, ll, lr, ylen, xlen), -1);
		}

		int high_gradient (uint64_t ul, uint64_t ur, uint64_t ll, uint64_t lr, int ylen, int xlen) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_highgradient (plane, ul, ur, ll, lr, ylen, xlen), -1);
		}

		int high_gradient_sized (uint64_t ul, uint64_t ur, uint64_t ll, uint64_t lr, int ylen, int xlen) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_highgradient_sized (plane, ul, ur, ll, lr, ylen, xlen), -1);
		}

		void greyscale () const noexcept
		{
			ncplane_greyscale (plane);
		}

		bool resize (int ylen, int xlen) const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_resize_simple (plane, ylen, xlen), -1);
		}

		bool fadeout (timespec *ts, fadecb fader = nullptr, void *curry = nullptr) const NOEXCEPT_MAYBE
		{
			return fadeout (static_cast<const timespec*>(ts), fader, curry);
		}

		bool fadeout (timespec &ts, fadecb fader = nullptr, void *curry = nullptr) const NOEXCEPT_MAYBE
		{
			return fadeout (&ts, fader, curry);
		}

		bool fadeout (timespec const& ts, fadecb fader = nullptr, void *curry = nullptr) const NOEXCEPT_MAYBE
		{
			return fadeout (&ts, fader, curry);
		}

		bool fadeout (const timespec *ts, fadecb fader = nullptr, void *curry = nullptr) const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_fadeout (plane, ts, fader, curry), -1);
		}

		bool fadein (timespec *ts, fadecb fader = nullptr) const NOEXCEPT_MAYBE
		{
			return fadein (static_cast<const timespec*>(ts), fader);
		}

		bool fadein (timespec &ts, fadecb fader = nullptr) const NOEXCEPT_MAYBE
		{
			return fadein (&ts, fader);
		}

		bool fadein (timespec const& ts, fadecb fader = nullptr) const NOEXCEPT_MAYBE
		{
			return fadein (&ts, fader);
		}

		bool fadein (const timespec *ts, fadecb fader = nullptr, void *curry = nullptr) const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_fadein (plane, ts, fader, curry), -1);
		}

		void erase () const noexcept
		{
			ncplane_erase (plane);
		}

		int get_align (NCAlign align, int c) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_align (plane, static_cast<ncalign_e>(align), c), INT_MAX);
		}

		void get_dim (int *rows, int *cols) const noexcept
		{
			ncplane_dim_yx (plane, rows, cols);
		}

		void get_dim (int &rows, int &cols) const noexcept
		{
			get_dim (&rows, &cols);
		}

		int get_dim_x () const noexcept
		{
			return ncplane_dim_x (plane);
		}

		int get_dim_y () const noexcept
		{
			return ncplane_dim_y (plane);
		}

		void get_yx (int *y, int *x) const noexcept
		{
			ncplane_yx (plane, y, x);
		}

		void get_yx (int &y, int &x) const noexcept
		{
			get_yx (&y, &x);
		}

		Plane* reparent (Plane *newparent = nullptr) const noexcept
		{
			ncplane *ret = ncplane_reparent (plane, newparent == nullptr ? nullptr : newparent->plane);
			if (ret == nullptr)
				return nullptr;

			return map_plane (ret);
		}

		Plane* reparent (const Plane *newparent) const noexcept
		{
			return reparent (const_cast<Plane*>(newparent));
		}

		Plane* reparent (const Plane &newparent) const noexcept
		{
			return reparent (const_cast<Plane*>(&newparent));
		}

		bool move (int y, int x) const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_move_yx (plane, y, x), -1);
		}

		bool move_top () const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_move_top (plane), -1);
		}

		bool move_bottom () const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_move_bottom (plane), -1);
		}

		bool move_below (Plane &below) const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_move_below (plane, below.plane), -1);
		}

		bool move_below (Plane *below) const
		{
			if (below == nullptr)
				throw invalid_argument ("'below' must be a valid pointer");

			return move_below (*below);
		}

		bool move_below_unsafe (Plane &below) const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_move_below_unsafe (plane, below.plane), -1);
		}

		bool move_below_unsafe (Plane *below) const
		{
			if (below == nullptr)
				throw invalid_argument ("'below' must be a valid pointer");

			return move_below_unsafe (*below);
		}

		bool move_above (Plane &above) const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_move_above (plane, above.plane), -1);
		}

		bool move_above (Plane *above) const
		{
			if (above == nullptr)
				throw invalid_argument ("'above' must be a valid pointer");

			return move_above (*above);
		}

		bool move_above_unsafe (Plane &above) const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_move_above_unsafe (plane, above.plane), -1);
		}

		bool move_above_unsafe (Plane *above) const
		{
			if (above == nullptr)
				throw invalid_argument ("'above' must be a valid pointer");

			return move_above (*above);
		}

		bool mergedown (Plane &dst) const
		{
			return mergedown (&dst);
		}

		bool mergedown (Plane *dst) const
		{
			if (dst != nullptr && plane == dst->plane)
				throw invalid_argument ("'dst' must refer to a differnt plane than the one this method is called on");

			return error_guard (ncplane_mergedown (plane, dst != nullptr ? dst->plane : nullptr), -1);
		}

		bool cursor_move (int y, int x) const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_cursor_move_yx (plane, y, x), -1);
		}

		void get_cursor_yx (int *y, int *x) const noexcept
		{
			ncplane_cursor_yx (plane, y, x);
		}

		void get_cursor_yx (int &y, int &x) const noexcept
		{
			get_cursor_yx (&y, &x);
		}

		int putc (const Cell &c) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_putc (plane, c), -1);
		}

		int putc (const Cell *c) const
		{
			if (c == nullptr)
				throw invalid_argument ("'c' must be a valid pointer");

			return putc (*c);
		}

		int putc (int y, int x, Cell const& c) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_putc_yx (plane, y, x, c), -1);
		}

		int putc (int y, int x, Cell const* c) const NOEXCEPT_MAYBE
		{
			if (c == nullptr)
				return -1;

			return putc (y, x, *c);
		}

		int putc (char c, bool retain_styling = false) const NOEXCEPT_MAYBE
		{
			int ret;
			if (retain_styling) {
				ret = ncplane_putsimple_stainable (plane, c);
			} else {
				ret = ncplane_putsimple (plane, c);
			}

			return error_guard<int> (ret, -1);
		}

		int putc (int y, int x, char c) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_putsimple_yx (plane, y, x, c), -1);
		}

		int putc (const char *gclust, int *sbytes = nullptr, bool retain_styling = false) const NOEXCEPT_MAYBE
		{
			int ret;
			if (retain_styling) {
				ret = ncplane_putegc_stainable (plane, gclust, sbytes);
			} else {
				ret = ncplane_putegc (plane, gclust, sbytes);
			}

			return error_guard<int> (ret, -1);
		}

		int putc (int y, int x, const char *gclust, int *sbytes = nullptr) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_putegc_yx (plane, y, x, gclust, sbytes), -1);
		}

		int putc (const wchar_t *gclust, int *sbytes = nullptr, bool retain_styling = false) const NOEXCEPT_MAYBE
		{
			int ret;
			if (retain_styling) {
				ret = ncplane_putwegc_stainable (plane, gclust, sbytes);
			} else {
				ret = ncplane_putwegc (plane, gclust, sbytes);
			}

			return error_guard<int> (ret, -1);
		}

		int putc (int y, int x, const wchar_t *gclust, int *sbytes = nullptr) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_putwegc_yx (plane, y, x, gclust, sbytes), -1);
		}

		// OK, this is ugly, but we need to rename this overload or calls similar to n->putc (0, 0, '0') will be
		// considered ambiguous with the above `putc (int, int, char)` overload.
		int putcw (int y, int x, wchar_t w) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_putwc_yx (plane, y, x, w), -1);
		}

		// Ditto
		int putcw (wchar_t w) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_putwc (plane, w), -1);
		}

		int putstr (const char *gclustarr) const NOEXCEPT_MAYBE
		{
			int ret = ncplane_putstr (plane, gclustarr);
			return error_guard_cond<int> (ret, ret < 0);
		}

		int putstr (int y, int x, const char *gclustarr) const NOEXCEPT_MAYBE
		{
			int ret = ncplane_putstr_yx (plane, y, x, gclustarr);
			return error_guard_cond<int> (ret, ret < 0);
		}

		int putstr (int y, NCAlign atype, const char *s) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_putstr_aligned (plane, y, static_cast<ncalign_e>(atype), s), -1);
		}

		int putstr (const wchar_t *gclustarr) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_putwstr (plane, gclustarr), -1);
		}

		int putstr (int y, int x, const wchar_t *gclustarr) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_putwstr_yx (plane, y, x, gclustarr), -1);
		}

		int putstr (int y, NCAlign atype, const wchar_t *gclustattr) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_putwstr_aligned (plane, y, static_cast<ncalign_e>(atype), gclustattr), -1);
		}

		int printf (const char* format, ...) const NOEXCEPT_MAYBE
			__attribute__ ((format (printf, 2, 3)))
		{
			va_list va;

			va_start (va, format);
			int ret = ncplane_vprintf (plane, format, va);
			va_end (va);

			return error_guard<int> (ret, -1);
		}

		int printf (int y, int x, const char *format, ...) const NOEXCEPT_MAYBE
			__attribute__ ((format (printf, 4, 5)))
		{
			va_list va;

			va_start (va, format);
			int ret = ncplane_vprintf_yx (plane, y, x, format, va);
			va_end (va);

			return error_guard<int> (ret, -1);
		}

		int printf (int y, NCAlign align, const char *format, ...) const NOEXCEPT_MAYBE
			__attribute__ ((format (printf, 4, 5)))
		{
			va_list va;

			va_start (va, format);
			int ret = vprintf (y, align, format, va);
			va_end (va);

			return error_guard<int> (ret, -1);
		}

		int vprintf (const char* format, va_list ap) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_vprintf (plane, format, ap), -1);
		}

		int vprintf (int y, int x, const char* format, va_list ap) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_vprintf_yx (plane, y, x, format, ap), -1);
		}

		int vprintf (int y, NCAlign align, const char *format, va_list ap) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_vprintf_aligned (plane, y, static_cast<ncalign_e>(align), format, ap), -1);
		}

		int hline (const Cell &c, int len) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_hline (plane, c, len), -1);
		}

		int hline (const Cell &c, int len, uint64_t c1, uint64_t c2) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_hline_interp (plane, c, len, c1, c2), -1);
		}

		int vline (const Cell &c, int len) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_vline (plane, c, len), -1);
		}

		int vline (const Cell &c, int len, uint64_t c1, uint64_t c2) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_vline_interp (plane, c, len, c1, c2), -1);
		}

		bool load_box (uint32_t attrs, uint64_t channels, Cell &ul, Cell &ur, Cell &ll, Cell &lr, Cell &hl, Cell &vl, const char *gclusters) const NOEXCEPT_MAYBE
		{
			return error_guard (cells_load_box (plane, attrs, channels, ul, ur, ll, lr, hl, vl, gclusters), -1);
		}

		bool load_box (CellStyle style, uint64_t channels, Cell &ul, Cell &ur, Cell &ll, Cell &lr, Cell &hl, Cell &vl, const char *gclusters) const NOEXCEPT_MAYBE
		{
			return load_box (static_cast<uint32_t>(style), channels, ul, ur, ll, lr, hl, vl, gclusters);
		}

		bool load_rounded_box (uint32_t attr, uint64_t channels, Cell &ul, Cell &ur, Cell &ll, Cell &lr, Cell &hl, Cell &vl) const NOEXCEPT_MAYBE
		{
			return error_guard (cells_rounded_box (plane, attr, channels, ul, ur, ll, lr, hl, vl), -1);
		}

		bool load_rounded_box (CellStyle style, uint64_t channels, Cell &ul, Cell &ur, Cell &ll, Cell &lr, Cell &hl, Cell &vl) const NOEXCEPT_MAYBE
		{
			return load_rounded_box (static_cast<uint32_t>(style), channels, ul, ur, ll, lr, hl, vl);
		}

		bool load_double_box (uint32_t attr, uint64_t channels, Cell &ul, Cell &ur, Cell &ll, Cell &lr, Cell &hl, Cell &vl) const NOEXCEPT_MAYBE
		{
			return error_guard (cells_double_box (plane, attr, channels, ul, ur, ll, lr, hl, vl), -1);
		}

		bool load_double_box (CellStyle style, uint64_t channels, Cell &ul, Cell &ur, Cell &ll, Cell &lr, Cell &hl, Cell &vl) const NOEXCEPT_MAYBE
		{
			return load_double_box (static_cast<uint32_t>(style), channels, ul, ur, ll, lr, hl, vl);
		}

		bool box (const Cell &ul, const Cell &ur, const Cell &ll, const Cell &lr,
							const Cell &hline, const Cell &vline, int ystop, int xstop,
							unsigned ctlword) const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_box (plane, ul, ur, ll, lr, hline, vline, ystop, xstop, ctlword), -1);
		}

		bool box_sized (const Cell &ul, const Cell &ur, const Cell &ll, const Cell &lr,
						const Cell &hline, const Cell &vline, int ylen, int xlen,
						unsigned ctlword) const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_box_sized (plane, ul, ur, ll, lr, hline, vline, ylen, xlen, ctlword), -1);
		}

		bool rounded_box (uint32_t attr, uint64_t channels, int ystop, int xstop, unsigned ctlword) const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_rounded_box (plane, attr, channels, ystop, xstop, ctlword), -1);
		}

		bool rounded_box_sized (uint32_t attr, uint64_t channels, int ylen, int xlen, unsigned ctlword) const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_rounded_box_sized (plane, attr, channels, ylen, xlen, ctlword), -1);
		}

		bool double_box (uint32_t attr, uint64_t channels, int ystop, int xstop, unsigned ctlword) const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_double_box (plane, attr, channels, ystop, xstop, ctlword), -1);
		}

		bool double_box_sized (uint32_t attr, uint64_t channels, int ylen, int xlen, unsigned ctlword) const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_double_box_sized (plane, attr, channels, ylen, xlen, ctlword), -1);
		}

		bool perimeter (const Cell &ul, const Cell &ur, const Cell &ll, const Cell &lr, const Cell &hline, const Cell &vline, unsigned ctlword) const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_perimeter (plane, ul, ur, ll, lr, hline, vline, ctlword), -1);
		}

		int polyfill (int y, int x, const Cell& c) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_polyfill_yx (plane, y, x, c), -1);
		}

		uint32_t* rgba(int begy, int begx, int leny, int lenx) const noexcept
		{
			return ncplane_rgba (plane, begy, begx, leny, lenx);
		}

		char* content(int begy, int begx, int leny, int lenx) const noexcept
		{
			return ncplane_contents (plane, begy, begx, leny, lenx);
		}

		uint64_t get_channels () const noexcept
		{
			return ncplane_channels (plane);
		}

		uint32_t get_attr () const noexcept
		{
			return ncplane_attr (plane);
		}

		unsigned get_bchannel () const noexcept
		{
			return ncplane_bchannel (plane);
		}

		unsigned get_fchannel () const noexcept
		{
			return ncplane_fchannel (plane);
		}

		unsigned get_fg () const noexcept
		{
			return ncplane_fg (plane);
		}

		unsigned get_bg () const noexcept
		{
			return ncplane_bg (plane);
		}

		unsigned get_fg_alpha () const noexcept
		{
			return ncplane_fg_alpha (plane);
		}

		bool set_fg_alpha (int alpha) const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_set_fg_alpha (plane, alpha), -1);
		}

		unsigned get_bg_alpha () const noexcept
		{
			return ncplane_bg_alpha (plane);
		}

		bool set_bg_alpha (int alpha) const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_set_bg_alpha (plane, alpha), -1);
		}

		unsigned get_fg_rgb (unsigned *r, unsigned *g, unsigned *b) const noexcept
		{
			return ncplane_fg_rgb (plane, r, g, b);
		}

		bool set_fg_rgb (int r, int g, int b, bool clipped = false) const NOEXCEPT_MAYBE
		{
			if (clipped) {
				ncplane_set_fg_rgb_clipped (plane, r, g, b);
				return true;
			}

			return error_guard (ncplane_set_fg_rgb (plane, r, g, b), -1);
		}

		bool set_fg_palindex (int idx) const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_set_fg_palindex (plane, idx), -1);
		}

		bool set_fg (uint32_t channel) const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_set_fg (plane, channel), -1);
		}

		void set_fg_default () const noexcept
		{
			ncplane_set_fg_default (plane);
		}

		unsigned get_bg_rgb (unsigned *r, unsigned *g, unsigned *b) const noexcept
		{
			return ncplane_bg_rgb (plane, r, g, b);
		}

		bool set_bg_rgb (int r, int g, int b, bool clipped = false) const NOEXCEPT_MAYBE
		{
			if (clipped) {
				ncplane_set_fg_rgb_clipped (plane, r, g, b);
				return true;
			}

			return error_guard (ncplane_set_bg_rgb (plane, r, g, b), -1);
		}

		bool set_bg_palindex (int idx) const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_set_bg_alpha (plane, idx), -1);
		}

		bool set_bg (uint32_t channel) const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_set_bg (plane, channel), -1);
		}

		void set_bg_default () const noexcept
		{
			ncplane_set_bg_default (plane);
		}

		bool set_scrolling (bool scrollp) const noexcept
		{
			return ncplane_set_scrolling (plane, scrollp);
		}

		void styles_set (CellStyle styles) const noexcept
		{
			ncplane_styles_set (plane, static_cast<unsigned>(styles));
		}

		void styles_on (CellStyle styles) const noexcept
		{
			ncplane_styles_on (plane, static_cast<unsigned>(styles));
		}

		void styles_off (CellStyle styles) const noexcept
		{
			ncplane_styles_off (plane, static_cast<unsigned>(styles));
		}

		int format (int ystop, int xstop, uint32_t attrword) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_format (plane, ystop, xstop, attrword), -1);
		}

		int stain (int ystop, int xstop, uint64_t ul, uint64_t ur, uint64_t ll, uint64_t lr) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_stain (plane, ystop, xstop, ul, ur, ll, lr), -1);
		}

		Plane* get_below () const noexcept
		{
			return map_plane (ncplane_below (plane));
		}

		bool set_base_cell (Cell &c) const NOEXCEPT_MAYBE
		{
			bool ret = ncplane_set_base_cell (plane, c) < 0;
			return error_guard_cond<bool, bool> (ret, ret);
		}

		bool set_base (const char* egc, uint32_t attrword, uint64_t channels) const NOEXCEPT_MAYBE
		{
			bool ret = ncplane_set_base (plane, egc, attrword, channels) < 0;
			return error_guard_cond<bool, bool> (ret, ret);
		}

		bool get_base (Cell &c) const NOEXCEPT_MAYBE
		{
			bool ret = ncplane_base (plane, c) < 0;
			return error_guard_cond<bool, bool> (ret, ret);
		}

		bool at_cursor (Cell &c) const NOEXCEPT_MAYBE
		{
			bool ret = ncplane_at_cursor_cell (plane, c) < 0;
			return error_guard_cond<bool, bool> (ret, ret);
		}

		bool at_cursor (Cell *c) const noexcept
		{
			if (c == nullptr)
				return false;

			return at_cursor (*c);
		}

		char* at_cursor (uint32_t* attrword, uint64_t* channels) const
		{
			if (attrword == nullptr || channels == nullptr)
				return nullptr;

			return ncplane_at_cursor (plane, attrword, channels);
		}

		int get_at (int y, int x, Cell &c) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_at_yx_cell (plane, y, x, c), -1);
		}

		int get_at (int y, int x, Cell *c) const
		{
			if (c == nullptr)
				return -1;

			return get_at (y, x, *c);
		}

		char* get_at (int y, int x, uint32_t* attrword, uint64_t* channels) const
		{
			if (attrword == nullptr || channels == nullptr)
				return nullptr;

			return ncplane_at_yx (plane, y, x, attrword, channels);
		}

		void* set_userptr (void *opaque) const noexcept
		{
			return ncplane_set_userptr (plane, opaque);
		}

		template<typename T>
		T* set_userptr (T *opaque) const noexcept
		{
			return static_cast<T*>(set_userptr (static_cast<void*>(opaque)));
		}

		void* get_userptr () const noexcept
		{
			return ncplane_userptr (plane);
		}

		template<typename T>
		T* get_userptr () const noexcept
		{
			return static_cast<T*>(get_userptr ());
		}

		Visual* visual_open (const char *file, nc_err_e *ncerr) const
		{
			return new Visual (plane, file, ncerr);
		}

		NcReel* ncreel_create (const ncreel_options *popts = nullptr, int efd = -1) const
		{
			return new NcReel (plane, popts, efd);
		}

		// Some Cell APIs go here since they act on individual panels even though it may seem weird at points (e.g.
		// release)

		int load (Cell &cell, const char *gcluster) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (cell_load (plane, cell, gcluster), -1);
		}

		bool load (Cell &cell, char ch) const NOEXCEPT_MAYBE
		{
			return error_guard (cell_load_simple (plane, cell, ch), -1);
		}

		int prime (Cell &cell, const char *gcluster, uint32_t attr, uint64_t channels) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (cell_prime (plane, cell, gcluster, attr, channels), -1);
		}

		void release (Cell &cell) const noexcept
		{
			cell_release (plane, cell);
		}

		int duplicate (Cell &target, Cell &source) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (cell_duplicate (plane, target, source), -1);
		}

		int duplicate (Cell &target, Cell const& source) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (cell_duplicate (plane, target, source), -1);
		}

		int duplicate (Cell &target, Cell *source) const
		{
			if (source == nullptr)
				throw invalid_argument ("'source' must be a valid pointer");

			return duplicate (target, *source);
		}

		int duplicate (Cell &target, Cell const* source) const
		{
			if (source == nullptr)
				throw invalid_argument ("'source' must be a valid pointer");

			return duplicate (target, *source);
		}

		int duplicate (Cell *target, Cell *source) const
		{
			if (target == nullptr)
				throw invalid_argument ("'target' must be a valid pointer");
			if (source == nullptr)
				throw invalid_argument ("'source' must be a valid pointer");

			return duplicate (*target, *source);
		}

		int duplicate (Cell *target, Cell const* source) const
		{
			if (target == nullptr)
				throw invalid_argument ("'target' must be a valid pointer");
			if (source == nullptr)
				throw invalid_argument ("'source' must be a valid pointer");

			return duplicate (*target, *source);
		}

		int duplicate (Cell *target, Cell &source) const
		{
			if (target == nullptr)
				throw invalid_argument ("'target' must be a valid pointer");

			return duplicate (*target, source);
		}

		int duplicate (Cell *target, Cell const& source) const
		{
			if (target == nullptr)
				throw invalid_argument ("'target' must be a valid pointer");

			return duplicate (*target, source);
		}

		void translate (const Plane *dst, int *y = nullptr, int *x = nullptr) const noexcept
		{
			ncplane_translate (*this, dst ? dst->plane: nullptr, y, x);
		}

		void translate (const Plane &dst, int *y = nullptr, int *x = nullptr) const noexcept
		{
			translate (*this, dst, y, x);
		}

		static void translate (const Plane *src, const Plane *dst, int *y = nullptr, int *x = nullptr)
		{
			if (src == nullptr)
				throw invalid_argument ("'src' must be a valid pointer");

			ncplane_translate (*src, dst ? dst->plane : nullptr, y, x);
		}

		static void translate (const Plane &src, const Plane &dst, int *y = nullptr, int *x = nullptr) noexcept
		{
			ncplane_translate (src.plane, dst.plane, y, x);
		}

		bool translate_abs (int *y = nullptr, int *x = nullptr) const noexcept
		{
			return error_guard<bool, bool> (ncplane_translate_abs (plane, y, x), false);
		}

		bool rotate_cw () const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_rotate_cw (plane), -1);
		}

		bool rotate_ccw () const noexcept
		{
			return error_guard (ncplane_rotate_ccw (plane), -1);
		}

		// Upstream call doesn't take ncplane* but we put it here for parity with has_no_background below
		bool has_no_foreground (Cell &cell) const noexcept
		{
			return cell.has_no_foreground ();
		}

		const char* get_extended_gcluster (Cell &cell) const noexcept
		{
			return cell_extended_gcluster (plane, cell);
		}

		const char* get_extended_gcluster (Cell const& cell) const noexcept
		{
			return cell_extended_gcluster (plane, cell);
		}

		static Plane* map_plane (ncplane *ncp, Plane *associated_plane = nullptr) noexcept;

		bool blit_bgrx (int placey, int placex, int linesize, const void* data, int begy, int begx, int leny, int lenx) const NOEXCEPT_MAYBE
		{
			bool ret = ncblit_bgrx (plane, placey, placex, linesize, data, begy, begx, leny, lenx) < 0;
			return error_guard_cond<bool, bool> (ret, ret);
		}

		bool blit_rgba (int placey, int placex, int linesize, const void* data, int begy, int begx, int leny, int lenx) const NOEXCEPT_MAYBE
		{
			bool ret = ncblit_rgba (plane, placey, placex, linesize, data, begy, begx, leny, lenx) < 0;
			return error_guard_cond<bool, bool> (ret, ret);
		}

		int qrcode (int maxversion, const void *data, size_t len) const NOEXCEPT_MAYBE
		{
			int ret = ncplane_qrcode (plane, maxversion, data, len);
			return error_guard_cond<int> (ret, ret < 0);
		}

	protected:
		explicit Plane (ncplane *_plane, bool _is_stdplane)
			: plane (_plane),
				is_stdplane (_is_stdplane)
		{
			if (_plane == nullptr)
				throw invalid_argument ("_plane must be a valid pointer");
		}

		static void unmap_plane (Plane *p) noexcept;

	private:
		ncplane* create_plane (const Plane &n, int rows, int cols, int yoff, int xoff, void *opaque)
		{
			ncplane *ret = ncplane_bound (
				n.plane,
				rows,
				cols,
				yoff,
				xoff,
				opaque
			);

			if (ret == nullptr)
				throw init_error ("notcurses failed to create a new plane");

			map_plane (plane, this);

			return ret;
		}

		ncplane* create_plane (Plane &n, int rows, int cols, int yoff, NCAlign align, void *opaque)
		{
			ncplane *ret = ncplane_aligned (
				n.plane,
				rows,
				cols,
				yoff,
				static_cast<ncalign_e>(align),
				opaque
			);

			if (ret == nullptr)
				throw init_error ("notcurses failed to create an aligned plane");

			map_plane (plane, this);

			return ret;
		}

		ncplane* duplicate_plane (const Plane &other, void *opaque)
		{
			ncplane *ret = ncplane_dup (other.plane, opaque);
			if (ret == nullptr)
				throw init_error ("notcurses failed to duplicate plane");

			return ret;
		}

	private:
		ncplane *plane = nullptr;
		bool is_stdplane = false;
		static std::map<ncplane*,Plane*> *plane_map;
		static std::mutex plane_map_mutex;

		friend class NotCurses;
		friend class Visual;
		friend class NcReel;
		friend class Tablet;
	};
}
#endif

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
#include "CellStyle.hh"
#include "NCAlign.hh"
#include "NCBox.hh"

namespace ncpp
{
	class NcReel;

	class NCPP_API_EXPORT Plane : public Root
	{
	public:
		Plane (Plane&& other)
			: Root (nullptr)
		{
			unmap_plane (&other);

			plane = other.plane;
			is_stdplane = other.is_stdplane;

			map_plane (plane, this);

			other.plane = nullptr;
			other.is_stdplane = false;
		}

		Plane (Plane const& other)
			: Root (nullptr)
		{
			plane = duplicate_plane (other, nullptr);
		}

		explicit Plane (Plane const& other, void *opaque)
			: Root (nullptr)
		{
			plane = duplicate_plane (other, opaque);
		}

		explicit Plane (Plane *n, int rows, int cols, int yoff, int xoff, void *opaque = nullptr)
			: Plane (static_cast<const Plane*>(n), rows, cols, yoff, xoff, opaque)
		{}

		explicit Plane (const Plane *n, int rows, int cols, int yoff, int xoff, void *opaque = nullptr)
			: Root (nullptr)
		{
			if (n == nullptr)
				throw invalid_argument ("'n' must be a valid pointer");

			plane = create_plane (*n, rows, cols, yoff, xoff, opaque);
		}

		explicit Plane (Plane *n, ncplane_options const& nopts, NotCurses *ncinst = nullptr)
			: Plane (static_cast<const Plane*>(n), nopts, ncinst)
		{}

		explicit Plane (const Plane *n, ncplane_options const& nopts, NotCurses *ncinst = nullptr)
			: Root (ncinst)
		{
			if (n == nullptr) {
				throw invalid_argument ("'n' must be a valid pointer");
			}

			plane = create_plane (*n, nopts);
		}

		explicit Plane (const Plane &n, int rows, int cols, int yoff, int xoff, void *opaque = nullptr)
			: Root (nullptr)
		{
			plane = create_plane (n, rows, cols, yoff, xoff, opaque);
		}

		explicit Plane (unsigned rows, unsigned cols, int yoff, int xoff, void *opaque = nullptr, NotCurses *ncinst = nullptr)
			: Root (ncinst)
		{
			ncplane_options nopts = {
				.y = yoff,
				.x = xoff,
				.rows = rows,
				.cols = cols,
				.userptr = opaque,
				.name = nullptr,
				.resizecb = nullptr,
				.flags = 0,
				.margin_b = 0,
				.margin_r = 0,
			};
			plane = ncplane_create (
				notcurses_stdplane(get_notcurses ()),
				&nopts
			);

			if (plane == nullptr)
				throw init_error ("Notcurses failed to create a new plane");

			map_plane (plane, this);
		}

		explicit Plane (Plane &n, int rows, int cols, int yoff, NCAlign align, void *opaque = nullptr)
			: Root (nullptr)
		{
			plane = create_plane (n, rows, cols, yoff, align, opaque);
		}

		explicit Plane (Plane const& n, int rows, int cols, int yoff, NCAlign align, void *opaque = nullptr)
			: Root (nullptr)
		{
			plane = create_plane (const_cast<Plane&>(n), rows, cols, yoff, align, opaque);
		}

		explicit Plane (Plane &n, ncplane_options const& nopts, NotCurses *ncinst = nullptr)
			: Plane (static_cast<Plane const&>(n), nopts, ncinst)
		{}

		explicit Plane (Plane const& n, ncplane_options const& nopts, NotCurses *ncinst = nullptr)
			: Root (ncinst)
		{
			plane = create_plane (n, nopts);
		}

		explicit Plane (Plane *n, int rows, int cols, int yoff, NCAlign align, void *opaque = nullptr)
			: Root (nullptr)
		{
			if (n == nullptr)
				throw invalid_argument ("'n' must be a valid pointer");

			plane = create_plane (*n, rows, cols, yoff, align, opaque);
		}

		explicit Plane (Plane const* n, int rows, int cols, int yoff, NCAlign align, void *opaque = nullptr)
			: Root (nullptr)
		{
			if (n == nullptr)
				throw invalid_argument ("'n' must be a valid pointer");

			plane = create_plane (const_cast<Plane&>(*n), rows, cols, yoff, align, opaque);
		}

		explicit Plane (ncplane *_plane) noexcept
			: Root (nullptr),
			  plane (_plane)
		{}

		~Plane () noexcept
		{
			if (is_stdplane || plane == nullptr)
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

		void center_abs (int *y, int *x) const noexcept
		{
			ncplane_center_abs (plane, y, x);
		}

		ncplane* to_ncplane () const noexcept
		{
			return plane;
		}

		bool resize_maximize () const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_resize_maximize (plane), -1);
		}

		bool resize_realign () const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_resize_realign (plane), -1);
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

		int gradient (int y, int x, unsigned ylen, unsigned xlen, const char* egc, uint16_t stylemask, uint64_t ul, uint64_t ur, uint64_t ll, uint64_t lr) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_gradient (plane, y, x, ylen, xlen, egc, stylemask, ul, ur, ll, lr), -1);
		}

		int gradient2x1 (int y, int x, unsigned ylen, unsigned xlen, uint32_t ul, uint32_t ur, uint32_t ll, uint32_t lr) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_gradient2x1 (plane, y, x, ylen, xlen, ul, ur, ll, lr), -1);
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

		int get_abs_x () const noexcept
		{
			return ncplane_abs_x (plane);
		}

		int get_abs_y () const noexcept
		{
			return ncplane_abs_y (plane);
		}

		int get_x () const noexcept
		{
			return ncplane_x (plane);
		}

		int get_y () const noexcept
		{
			return ncplane_y (plane);
		}

		int get_align (NCAlign align, int c) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_halign (plane, static_cast<ncalign_e>(align), c), INT_MAX);
		}

		int get_halign (NCAlign align, int c) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_halign (plane, static_cast<ncalign_e>(align), c), INT_MAX);
		}

		int get_valign (NCAlign align, int r) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_valign (plane, static_cast<ncalign_e>(align), r), INT_MAX);
		}

		void get_dim (unsigned *rows, unsigned *cols) const noexcept
		{
			ncplane_dim_yx (plane, rows, cols);
		}

		void get_dim (unsigned &rows, unsigned &cols) const noexcept
		{
			get_dim (&rows, &cols);
		}

		unsigned get_dim_x () const noexcept
		{
			return ncplane_dim_x (plane);
		}

		unsigned get_dim_y () const noexcept
		{
			return ncplane_dim_y (plane);
		}

		void get_abs_yx (int* y, int* x) const noexcept
		{
			ncplane_abs_yx (plane, y, x);
		}

		void get_yx (int *y, int *x) const noexcept
		{
			ncplane_yx (plane, y, x);
		}

		void get_yx (int &y, int &x) const noexcept
		{
			get_yx (&y, &x);
		}

		Plane* get_parent () const noexcept
		{
			ncplane *ret = ncplane_parent (plane);
			if (ret == nullptr) {
				return nullptr;
			}

			return map_plane (ret);
		}

		Plane* reparent (Plane *newparent = nullptr) const noexcept
		{
			ncplane *ret = ncplane_reparent (plane, newparent == nullptr ? plane : newparent->plane);
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

		Plane* reparent_family (Plane *newparent = nullptr) const noexcept
		{
			ncplane *ret = ncplane_reparent_family (plane, newparent == nullptr ? plane : newparent->plane);
			if (ret == nullptr)
				return nullptr;

			return map_plane (ret);
		}

		Plane* reparent_family (const Plane *newparent) const noexcept
		{
			return reparent_family (const_cast<Plane*>(newparent));
		}

		Plane* reparent_family (const Plane &newparent) const noexcept
		{
			return reparent_family (const_cast<Plane*>(&newparent));
		}

		void home () const noexcept
		{
			ncplane_home (plane);
		}

		bool move (int y, int x) const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_move_yx (plane, y, x), -1);
		}

		void move_top () noexcept
		{
			ncplane_move_top (plane);
		}

		void move_bottom () noexcept
		{
			ncplane_move_bottom (plane);
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

		bool mergedown (Plane &dst, unsigned begsrcy, unsigned begsrcx, unsigned leny, unsigned lenx, unsigned dsty, unsigned dstx) const
		{
			return mergedown (&dst, begsrcy, begsrcx, leny, lenx, dsty, dstx);
		}

		bool mergedown (Plane *dst, int begsrcy, int begsrcx, unsigned leny, unsigned lenx, int dsty, int dstx) const
		{
			if (plane == dst->plane)
				throw invalid_argument ("'dst' must refer to a different plane than the one this method is called on");

			return error_guard (ncplane_mergedown (plane, dst->plane, begsrcy, begsrcx, leny, lenx, dsty, dstx), -1);
		}

		bool mergedown_simple (Plane &dst) const
		{
			return mergedown_simple (&dst);
		}

		bool mergedown_simple (Plane *dst) const
		{
			if (plane == dst->plane)
				throw invalid_argument ("'dst' must refer to a different plane than the one this method is called on");

			return error_guard (ncplane_mergedown_simple (plane, dst->plane), -1);
		}

		bool cursor_move (int y, int x) const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_cursor_move_yx (plane, y, x), -1);
		}

		void get_cursor_yx (unsigned *y, unsigned *x) const noexcept
		{
			ncplane_cursor_yx (plane, y, x);
		}

		void get_cursor_yx (unsigned &y, unsigned &x) const noexcept
		{
			get_cursor_yx (&y, &x);
		}

		unsigned cursor_y() const noexcept
	  {
			return ncplane_cursor_y(plane);
		}

		unsigned cursor_x() const noexcept
	  {
			return ncplane_cursor_x(plane);
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
				ret = ncplane_putchar_stained (plane, c);
			} else {
				ret = ncplane_putchar (plane, c);
			}

			return error_guard<int> (ret, -1);
		}

		int putc (int y, int x, char c) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_putchar_yx (plane, y, x, c), -1);
		}

		int putc (const char *gclust, size_t *sbytes = nullptr, bool retain_styling = false) const NOEXCEPT_MAYBE
		{
			int ret;
			if (retain_styling) {
				ret = ncplane_putegc_stained (plane, gclust, sbytes);
			} else {
				ret = ncplane_putegc (plane, gclust, sbytes);
			}

			return error_guard<int> (ret, -1);
		}

		int putc (int y, int x, const char *gclust, size_t *sbytes = nullptr) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_putegc_yx (plane, y, x, gclust, sbytes), -1);
		}

		int putc (const wchar_t *gclust, size_t *sbytes = nullptr, bool retain_styling = false) const NOEXCEPT_MAYBE
		{
			int ret;
			if (retain_styling) {
				ret = ncplane_putwegc_stained (plane, gclust, sbytes);
			} else {
				ret = ncplane_putwegc (plane, gclust, sbytes);
			}

			return error_guard<int> (ret, -1);
		}

		int putc (int y, int x, const wchar_t *gclust, size_t *sbytes = nullptr) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_putwegc_yx (plane, y, x, gclust, sbytes), -1);
		}

		// OK, this is ugly, but we need to rename this overload or calls similar to n->putc (0, 0, '0') will be
		// considered ambiguous with the above `putc (int, int, char)` overload.
		int putwch (int y, int x, wchar_t w) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_putwc_yx (plane, y, x, w), -1);
		}

		// Ditto
		int putwch (wchar_t w) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_putwc (plane, w), -1);
		}

		int putwc_stained (wchar_t w) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_putwc_stained (plane, w), -1);
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

		int putstr (int y, NCAlign atype, const wchar_t *gcluststyles) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_putwstr_aligned (plane, y, static_cast<ncalign_e>(atype), gcluststyles), -1);
		}

		int putstr_stained (const wchar_t* gclustarr) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_putwstr_stained (plane, gclustarr), -1);
		}

		int putstr_stained (const char* s) const NOEXCEPT_MAYBE
		{
			int ret = ncplane_putstr_stained (plane, s);
			return error_guard_cond<int> (ret, ret < 0);
		}

		int printf_stained (const char* format, ...) const NOEXCEPT_MAYBE
			__attribute__ ((format (printf, 2, 3)))
		{
			va_list va;

			va_start (va, format);
			int ret = ncplane_vprintf_stained (plane, format, va);
			va_end (va);

			return error_guard<int> (ret, -1);
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

		int vprintf_stained (const char* format, va_list ap) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_vprintf_stained (plane, format, ap), -1);
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

		int hline (const Cell &c, unsigned len) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_hline (plane, c, len), -1);
		}

		int hline (const Cell &c, unsigned len, uint64_t c1, uint64_t c2) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_hline_interp (plane, c, len, c1, c2), -1);
		}

		int vline (const Cell &c, unsigned len) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_vline (plane, c, len), -1);
		}

		int vline (const Cell &c, unsigned len, uint64_t c1, uint64_t c2) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_vline_interp (plane, c, len, c1, c2), -1);
		}

		bool load_box (uint16_t styles, uint64_t channels, Cell &ul, Cell &ur, Cell &ll, Cell &lr, Cell &hl, Cell &vl, const char *gclusters) const NOEXCEPT_MAYBE
		{
			return error_guard (nccells_load_box (plane, styles, channels, ul, ur, ll, lr, hl, vl, gclusters), -1);
		}

		bool load_box (CellStyle style, uint64_t channels, Cell &ul, Cell &ur, Cell &ll, Cell &lr, Cell &hl, Cell &vl, const char *gclusters) const NOEXCEPT_MAYBE
		{
			return load_box (static_cast<uint16_t>(style), channels, ul, ur, ll, lr, hl, vl, gclusters);
		}

		bool load_rounded_box (uint16_t styles, uint64_t channels, Cell &ul, Cell &ur, Cell &ll, Cell &lr, Cell &hl, Cell &vl) const NOEXCEPT_MAYBE
		{
			return error_guard (nccells_rounded_box (plane, styles, channels, ul, ur, ll, lr, hl, vl), -1);
		}

		bool load_rounded_box (CellStyle style, uint64_t channels, Cell &ul, Cell &ur, Cell &ll, Cell &lr, Cell &hl, Cell &vl) const NOEXCEPT_MAYBE
		{
			return load_rounded_box (static_cast<uint16_t>(style), channels, ul, ur, ll, lr, hl, vl);
		}

		bool load_double_box (uint16_t styles, uint64_t channels, Cell &ul, Cell &ur, Cell &ll, Cell &lr, Cell &hl, Cell &vl) const NOEXCEPT_MAYBE
		{
			return error_guard (nccells_double_box (plane, styles, channels, ul, ur, ll, lr, hl, vl), -1);
		}

		bool load_double_box (CellStyle style, uint64_t channels, Cell &ul, Cell &ur, Cell &ll, Cell &lr, Cell &hl, Cell &vl) const NOEXCEPT_MAYBE
		{
			return load_double_box (static_cast<uint16_t>(style), channels, ul, ur, ll, lr, hl, vl);
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

		bool rounded_box (uint16_t styles, uint64_t channels, int ystop, int xstop, unsigned ctlword) const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_rounded_box (plane, styles, channels, ystop, xstop, ctlword), -1);
		}

		bool rounded_box_sized (uint16_t styles, uint64_t channels, int ylen, int xlen, unsigned ctlword) const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_rounded_box_sized (plane, styles, channels, ylen, xlen, ctlword), -1);
		}

		bool double_box (uint16_t styles, uint64_t channels, int ystop, int xstop, unsigned ctlword) const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_double_box (plane, styles, channels, ystop, xstop, ctlword), -1);
		}

		bool double_box_sized (uint16_t styles, uint64_t channels, int ylen, int xlen, unsigned ctlword) const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_double_box_sized (plane, styles, channels, ylen, xlen, ctlword), -1);
		}

		bool perimeter (const Cell &ul, const Cell &ur, const Cell &ll, const Cell &lr, const Cell &hline, const Cell &vline, unsigned ctlword) const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_perimeter (plane, ul, ur, ll, lr, hline, vline, ctlword), -1);
		}

		bool perimeter_rounded (uint16_t stylemask, uint64_t channels, unsigned ctlword) const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_perimeter_rounded (plane, stylemask, channels, ctlword), -1);
		}

		bool perimeter_double (uint16_t stylemask, uint64_t channels, unsigned ctlword) const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_perimeter_double (plane, stylemask, channels, ctlword), -1);
		}

		int polyfill (int y, int x, const Cell& c) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_polyfill_yx (plane, y, x, c), -1);
		}

		uint32_t* rgba(ncblitter_e blit, unsigned begy, unsigned begx, unsigned leny, unsigned lenx) const noexcept
		{
			return ncplane_as_rgba (plane, blit, begy, begx, leny, lenx, nullptr, nullptr);
		}

		char* content(unsigned begy, unsigned begx, unsigned leny, unsigned lenx) const noexcept
		{
			return ncplane_contents (plane, begy, begx, leny, lenx);
		}

		uint64_t get_channels () const noexcept
		{
			return ncplane_channels (plane);
		}

		unsigned get_bchannel () const noexcept
		{
			return ncplane_bchannel (plane);
		}

		unsigned get_fchannel () const noexcept
		{
			return ncplane_fchannel (plane);
		}

		unsigned get_fg_rgb () const noexcept
		{
			return ncplane_fg_rgb (plane);
		}

		unsigned get_bg_rgb () const noexcept
		{
			return ncplane_bg_rgb (plane);
		}

		unsigned get_fg_alpha () const noexcept
		{
			return ncplane_fg_alpha (plane);
		}

		void set_channels (uint64_t channels) const noexcept
		{
			ncplane_set_channels (plane, channels);
		}

		bool set_fg_alpha (unsigned alpha) const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_set_fg_alpha (plane, alpha), -1);
		}

		unsigned get_bg_alpha () const noexcept
		{
			return ncplane_bg_alpha (plane);
		}

		bool set_bg_alpha (unsigned alpha) const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_set_bg_alpha (plane, alpha), -1);
		}

		unsigned get_fg_rgb8 (unsigned *r, unsigned *g, unsigned *b) const noexcept
		{
			return ncplane_fg_rgb8 (plane, r, g, b);
		}

		bool set_fg_rgb8 (int r, int g, int b, bool clipped = false) const NOEXCEPT_MAYBE
		{
			if (clipped) {
				ncplane_set_fg_rgb8_clipped (plane, r, g, b);
				return true;
			}

			return error_guard (ncplane_set_fg_rgb8 (plane, r, g, b), -1);
		}

		bool set_fg_palindex (int idx) const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_set_fg_palindex (plane, idx), -1);
		}

		bool set_fg_rgb (uint32_t channel) const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_set_fg_rgb (plane, channel), -1);
		}

		void set_fg_default () const noexcept
		{
			ncplane_set_fg_default (plane);
		}

		unsigned get_bg_rgb8 (unsigned *r, unsigned *g, unsigned *b) const noexcept
		{
			return ncplane_bg_rgb8 (plane, r, g, b);
		}

		bool set_bg_rgb8 (int r, int g, int b, bool clipped = false) const NOEXCEPT_MAYBE
		{
			if (clipped) {
				ncplane_set_fg_rgb8_clipped (plane, r, g, b);
				return true;
			}

			return error_guard (ncplane_set_bg_rgb8 (plane, r, g, b), -1);
		}

		bool set_bg_palindex (int idx) const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_set_bg_alpha (plane, idx), -1);
		}

		bool set_bg_rgb (uint32_t channel) const NOEXCEPT_MAYBE
		{
			return error_guard (ncplane_set_bg_rgb (plane, channel), -1);
		}

		void set_bg_default () const noexcept
		{
			ncplane_set_bg_default (plane);
		}

		bool set_scrolling (bool scrollp) const noexcept
		{
			return ncplane_set_scrolling (plane, scrollp);
		}

		unsigned get_styles () const noexcept
		{
			return ncplane_styles (plane);
		}

		void styles_set (CellStyle styles) const noexcept
		{
			ncplane_set_styles (plane, static_cast<unsigned>(styles));
		}

		void styles_on (CellStyle styles) const noexcept
		{
			ncplane_on_styles (plane, static_cast<unsigned>(styles));
		}

		void styles_off (CellStyle styles) const noexcept
		{
			ncplane_off_styles (plane, static_cast<unsigned>(styles));
		}

		int format (int y, int x, unsigned ylen, unsigned xlen, uint16_t stylemask) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_format (plane, y, x, ylen, xlen, stylemask), -1);
		}

		int stain (int y, int x, unsigned ylen, unsigned xlen, uint64_t ul, uint64_t ur, uint64_t ll, uint64_t lr) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (ncplane_stain (plane, y, x, ylen, xlen, ul, ur, ll, lr), -1);
		}

		Plane* get_below () const noexcept
		{
			return map_plane (ncplane_below (plane));
		}

		Plane* get_above () const noexcept
		{
			return map_plane (ncplane_above (plane));
		}

		bool set_base_cell (Cell &c) const NOEXCEPT_MAYBE
		{
			bool ret = ncplane_set_base_cell (plane, c) < 0;
			return error_guard_cond<bool, bool> (ret, ret);
		}

		bool set_base (const char* egc, uint16_t stylemask, uint64_t channels) const NOEXCEPT_MAYBE
		{
			bool ret = ncplane_set_base (plane, egc, stylemask, channels) < 0;
			return error_guard_cond<bool, bool> (ret, ret);
		}

		bool get_base (Cell &c) const NOEXCEPT_MAYBE
		{
			bool ret = ncplane_base (plane, c) < 0;
			return error_guard_cond<bool, bool> (ret, ret);
		}

		int at_cursor (Cell &c) const NOEXCEPT_MAYBE
		{
			return error_guard<int>(ncplane_at_cursor_cell (plane, c), -1);
		}

		int at_cursor (Cell *c) const noexcept
		{
			if (c == nullptr)
				return false;

			return at_cursor (*c);
		}

		char* at_cursor (uint16_t* stylemask, uint64_t* channels) const
		{
			if (stylemask == nullptr || channels == nullptr)
				return nullptr;

			return ncplane_at_cursor (plane, stylemask, channels);
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

		char* get_at (int y, int x, uint16_t* stylemask, uint64_t* channels) const
		{
			if (stylemask == nullptr || channels == nullptr)
				return nullptr;

			return ncplane_at_yx (plane, y, x, stylemask, channels);
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

		NcReel* ncreel_create (const ncreel_options *popts = nullptr);

		// Some Cell APIs go here since they act on individual panels even though it may seem weird at points (e.g.
		// release)

		int load_egc32 (Cell &cell, uint32_t egc) const NOEXCEPT_MAYBE
		{
			int ret = nccell_load_egc32 (plane, cell, egc);
			return error_guard_cond<int> (ret, ret != 1);
		}

		int load (Cell &cell, const char *gcluster) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (nccell_load (plane, cell, gcluster), -1);
		}

		bool load (Cell &cell, char ch) const NOEXCEPT_MAYBE
		{
			return error_guard (nccell_load_char (plane, cell, ch), -1);
		}

		int prime (Cell &cell, const char *gcluster, uint16_t styles, uint64_t channels) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (nccell_prime (plane, cell, gcluster, styles, channels), -1);
		}

		void release (Cell &cell) const noexcept
		{
			nccell_release (plane, cell);
		}

		int duplicate (Cell &target, Cell &source) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (nccell_duplicate (plane, target, source), -1);
		}

		int duplicate (Cell &target, Cell const& source) const NOEXCEPT_MAYBE
		{
			return error_guard<int> (nccell_duplicate (plane, target, source), -1);
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

		bool translate_abs (int *y = nullptr, int *x = nullptr) const NOEXCEPT_MAYBE
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

		char* strdup (Cell const& cell) const noexcept
		{
			return nccell_strdup (plane, cell);
		}

		char* extract (Cell const& cell, uint16_t *stylemask = nullptr, uint64_t *channels = nullptr)
		{
			return nccell_extract (plane, cell, stylemask, channels);
		}

		const char* get_extended_gcluster (Cell &cell) const noexcept
		{
			return nccell_extended_gcluster (plane, cell);
		}

		const char* get_extended_gcluster (Cell const& cell) const noexcept
		{
			return nccell_extended_gcluster (plane, cell);
		}

		static Plane* map_plane (ncplane *ncp, Plane *associated_plane = nullptr) noexcept;

		bool blit_bgrx (const void* data, int linesize, const struct ncvisual_options *vopts) const NOEXCEPT_MAYBE
		{
			bool ret = ncblit_bgrx (data, linesize, vopts) < 0;
			return error_guard_cond<bool, bool> (ret, ret);
		}

		bool blit_rgba (const void* data, int linesize, const struct ncvisual_options *vopts) const NOEXCEPT_MAYBE
		{
			bool ret = ncblit_rgba (data, linesize, vopts) < 0;
			return error_guard_cond<bool, bool> (ret, ret);
		}

		int qrcode (unsigned* ymax, unsigned* xmax, const void *data, size_t len) const NOEXCEPT_MAYBE
		{
			int ret = ncplane_qrcode (plane, ymax, xmax, data, len);
			return error_guard_cond<int> (ret, ret < 0);
		}

		bool is_descendant_of (const Plane& ancestor) const noexcept
		{
			return ncplane_descendant_p (plane, ancestor) != 0;
		}

		bool is_fg_default () const noexcept
		{
			return ncplane_fg_default_p (plane);
		}

		bool is_bg_default () const noexcept
		{
			return ncplane_bg_default_p (plane);
		}

		bool is_valid () const noexcept
		{
			return plane != nullptr;
		}

		void set_resizecb (int(*resizecb)(struct ncplane*)) const noexcept
		{
			ncplane_set_resizecb (plane, resizecb);
		}

	protected:
		explicit Plane (ncplane *_plane, bool _is_stdplane)
			: Root (nullptr),
			  plane (_plane),
			  is_stdplane (_is_stdplane)
		{
			if (_plane == nullptr)
				throw invalid_argument ("_plane must be a valid pointer");
		}

		// This is used by child classes which cannot provide a valid ncplane* in their constructor when initializing
		// the parent class (e.g. Pile)
		Plane (NotCurses *ncinst = nullptr)
			: Root (ncinst),
			  plane (nullptr)
		{}

		// Can be used only once and only if plane == nullptr. Meant to be used by child classes which cannot provide a
		// valid ncplane* in their constructor when initializing the parent class (e.g. Pile)
		void set_plane (ncplane *_plane)
		{
			if (_plane == nullptr) {
				throw invalid_argument ("_plane must be a valid pointer");
			}

			if (plane != nullptr) {
				throw invalid_state_error ("Plane::set_plane can be called only once");
			}

			plane = _plane;
			map_plane (plane, this);
		}

		void release_native_plane () noexcept
		{
			if (plane == nullptr)
				return;

			unmap_plane (this);
			plane = nullptr;
		}

		static void unmap_plane (Plane *p) noexcept;

	private:
		ncplane* create_plane (const Plane &n, unsigned rows, unsigned cols, int yoff, int xoff, void *opaque)
		{
			ncplane_options nopts = {
				.y = yoff,
				.x = xoff,
				.rows = rows,
				.cols = cols,
				.userptr = opaque,
				.name = nullptr,
				.resizecb = nullptr,
				.flags = 0,
				.margin_b = 0,
				.margin_r = 0,
			};
			return create_plane (n, nopts);
		}

		ncplane* create_plane (Plane &n, unsigned rows, unsigned cols, int yoff, NCAlign align, void *opaque)
		{
			ncplane_options nopts = {
				yoff,
				static_cast<ncalign_e>(align),
				rows,
				cols,
				opaque,
				nullptr,
				nullptr,
				0,
				0,
				0,
			};
			return create_plane (n, nopts);
		}

		ncplane* create_plane (const Plane &n, ncplane_options const& nopts)
		{
			ncplane *ret = ncplane_create (
				n.plane,
				&nopts
			);

			if (ret == nullptr) {
				throw init_error ("Notcurses failed to create an aligned plane");
			}

			map_plane (plane, this);
			return ret;
		}

		ncplane* duplicate_plane (const Plane &other, void *opaque)
		{
			ncplane *ret = ncplane_dup (other.plane, opaque);
			if (ret == nullptr)
				throw init_error ("Notcurses failed to duplicate plane");

			return ret;
		}

	private:
		ncplane *plane = nullptr;
		bool is_stdplane = false;
		static std::map<ncplane*,Plane*> *plane_map;
		static std::mutex plane_map_mutex;

		friend class NotCurses;
		friend class NcReel;
		friend class Tablet;
		friend class Widget;
		template<typename TPlot, typename TCoord> friend class PlotBase;
	};
}
#endif

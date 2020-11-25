#ifndef __NCPP_NOTCURSES_HH
#define __NCPP_NOTCURSES_HH

#include <cstdio>
#include <ctime>
#include <csignal>
#include <mutex>

#include <notcurses/notcurses.h>

#include "CellStyle.hh"
#include "NCKey.hh"
#include "NCLogLevel.hh"
#include "Palette256.hh"
#include "Plane.hh"
#include "Root.hh"
#include "_helpers.hh"

namespace ncpp
{
	class NCPP_API_EXPORT NotCurses : public Root
	{
	public:
		static notcurses_options default_notcurses_options;

	public:
		explicit NotCurses (FILE *fp = nullptr)
			: NotCurses (default_notcurses_options, fp)
		{}

		explicit NotCurses (const notcurses_options &nc_opts, FILE *fp = nullptr);

		// Must not move or copy a NotCurses instance because we have no way to guarantee validity of any other copy
		// when even a single instance is destructed as that operation would close notcurses.
		NotCurses (const NotCurses &other) = delete;
		NotCurses (NotCurses &&other) = delete;
		~NotCurses ();

		operator notcurses* () noexcept
		{
			return nc;
		}

		operator notcurses const* () const noexcept
		{
			return nc;
		}

		static NotCurses& get_instance ()
		{
			if (_instance == nullptr)
				throw invalid_state_error ("NotCurses instance not found.");
			if (_instance->nc == nullptr)
				throw invalid_state_error (ncpp_invalid_state_message);

			return *_instance;
		}

		static bool is_notcurses_stopped (const NotCurses *ncinst = nullptr)
		{
			if (ncinst != nullptr)
				return ncinst->nc == nullptr;

			return *_instance == nullptr || _instance->nc == nullptr;
		}

		static const char* ncmetric (uintmax_t val, uintmax_t decimal, char *buf, int omitdec, unsigned mult, int uprefix) noexcept
		{
			return ::ncmetric (val, decimal, buf, omitdec, mult, uprefix);
		}

		static const char* qprefix (uintmax_t val, uintmax_t decimal, char *buf, int omitdec) noexcept
		{
			return ::qprefix (val, decimal, buf, omitdec);
		}

		static const char* iprefix (uintmax_t val, uintmax_t decimal, char *buf, int omitdec) noexcept
		{
			return ::iprefix (val, decimal, buf, omitdec);
		}

		static const char* bprefix (uintmax_t val, uintmax_t decimal, char *buf, int omitdec) noexcept
		{
			return ::bprefix (val, decimal, buf, omitdec);
		}

		static const char* version () noexcept
		{
			return notcurses_version ();
		}

		static void version_components (int* major, int* minor, int* patch, int* tweak) noexcept
		{
			notcurses_version_components (major, minor, patch, tweak);
		}

		static const char* str_blitter (ncblitter_e blitter) noexcept
		{
			return notcurses_str_blitter (blitter);
		}

		static const char* str_scalemode (ncscale_e scalemode) noexcept
		{
			return notcurses_str_scalemode (scalemode);
		}

		static bool lex_margins (const char* op, notcurses_options* opts) NOEXCEPT_MAYBE
		{
			return error_guard (notcurses_lex_margins (op, opts), -1);
		}

		static bool lex_blitter (const char* op, ncblitter_e* blitter) NOEXCEPT_MAYBE
		{
			return error_guard (notcurses_lex_blitter (op, blitter), -1);
		}

		static bool lex_scalemode (const char* op, ncscale_e* scalemode) NOEXCEPT_MAYBE
		{
			return error_guard (notcurses_lex_scalemode (op, scalemode), -1);
		}

		bool stop ();

		bool can_utf8 () const noexcept
		{
			return notcurses_canutf8 (nc);
		}

		bool can_fade () const noexcept
		{
			return notcurses_canfade (nc);
		}

		bool can_open_images () const noexcept
		{
			return notcurses_canopen_images (nc);
		}

		bool can_open_videos () const noexcept
		{
			return notcurses_canopen_videos (nc);
		}

		bool can_change_color () const noexcept
		{
			return notcurses_canchangecolor (nc);
		}

		bool can_truecolor () const noexcept
		{
			return notcurses_cantruecolor (nc);
		}

		int cursor_enable (int y, int x) const NOEXCEPT_MAYBE
		{
			return error_guard (notcurses_cursor_enable (nc, y, x), -1);
		}

		int cursor_disable () const NOEXCEPT_MAYBE
		{
			return error_guard (notcurses_cursor_disable (nc), -1);
		}

		void get_stats (ncstats *stats) const noexcept
		{
			if (stats == nullptr)
				return;

			notcurses_stats (nc, stats);
		}

		void reset_stats (ncstats *stats) const
		{
			if (stats == nullptr)
				throw invalid_argument ("'stats' must be a valid pointer");

			notcurses_stats_reset (nc, stats);
		}

		bool use (const Palette256 *p) const
		{
			if (p == nullptr)
				throw invalid_argument ("'p' must be a valid pointer");

			return use (*p);
		}

		bool use (const Palette256 &p) const NOEXCEPT_MAYBE
		{
			return error_guard (palette256_use (nc, reinterpret_cast<const palette256*>(&p)), -1);
		}

		bool render () const NOEXCEPT_MAYBE
		{
			return error_guard (notcurses_render (nc), -1);
		}

		bool render_to_file (FILE* fp) const NOEXCEPT_MAYBE
		{
			return error_guard (notcurses_render_to_file (nc, fp), -1);
		}

		void get_term_dim (int *rows, int *cols) const noexcept
		{
			notcurses_term_dim_yx (nc, rows, cols);
		}

		void get_term_dim (int &rows, int &cols) const noexcept
		{
			get_term_dim (&rows, &cols);
		}

		bool refresh (int* rows, int* cols) const NOEXCEPT_MAYBE
		{
			return error_guard (notcurses_refresh (nc, rows, cols), -1);
		}

		bool refresh (int& rows, int& cols) const NOEXCEPT_MAYBE
		{
			return refresh (&rows, &cols);
		}

		unsigned get_palette_size () const noexcept
		{
			return notcurses_palette_size (static_cast<const notcurses*> (nc));
		}

		bool mouse_enable () const NOEXCEPT_MAYBE
		{
			return error_guard (notcurses_mouse_enable (nc), -1);
		}

		bool mouse_disable () const NOEXCEPT_MAYBE
		{
			return error_guard (notcurses_mouse_disable (nc), -1);
		}

		CellStyle get_supported_styles () const noexcept
		{
			return static_cast<CellStyle>(notcurses_supported_styles (nc));
		}

		char32_t getc (const timespec *ts, sigset_t *sigmask = nullptr, ncinput *ni = nullptr) const noexcept
		{
			return notcurses_getc (nc, ts, sigmask, ni);
		}

		char32_t getc (bool blocking = false, ncinput *ni = nullptr) const noexcept
		{
			if (blocking)
				return notcurses_getc_blocking (nc, ni);

			return notcurses_getc_nblock (nc, ni);
		}

		char* get_at (int yoff, int xoff, uint16_t* attr, uint64_t* channels) const noexcept
		{
			return notcurses_at_yx (nc, yoff, xoff, attr, channels);
		}

		int get_inputready_fd () const noexcept
		{
			return notcurses_inputready_fd (nc);
		}

		void drop_planes () const noexcept
		{
			notcurses_drop_planes (nc);
		}

		void debug (FILE *debugfp) const noexcept
		{
			notcurses_debug (nc, debugfp);
		}

		bool align (int availcols, ncalign_e align, int cols) const NOEXCEPT_MAYBE
		{
			return error_guard (notcurses_align (availcols, align, cols), -INT_MAX);
		}

		ncstats* stats_alloc () const noexcept
		{
			return notcurses_stats_alloc (nc);
		}

		static bool ucs32_to_utf8 (const char32_t *ucs32, unsigned ucs32count, unsigned char *resultbuf, size_t buflen) NOEXCEPT_MAYBE
		{
			return error_guard (notcurses_ucs32_to_utf8 (ucs32, ucs32count, resultbuf, buflen), -1);
		}

		Plane* get_stdplane () noexcept
		{
			return new Plane (notcurses_stdplane (nc), true);
		}

		Plane* get_stdplane (int *y, int *x)
		{
			if (y == nullptr)
				throw invalid_argument ("'y' must be a valid pointer");
			if (x == nullptr)
				throw invalid_argument ("'x' must be a valid pointer");

			return get_stdplane (*y, *x);
		}

		Plane* get_stdplane (int &y, int &x) noexcept
		{
			return new Plane (notcurses_stddim_yx (nc, &y, &x));
		}

		Plane* get_top () noexcept;
		Plane* get_bottom () noexcept;

	private:
		notcurses *nc;

		static NotCurses *_instance;
		static std::mutex init_mutex;
	};
}

#endif

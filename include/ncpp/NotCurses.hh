#ifndef __NCPP_NOTCURSES_HH
#define __NCPP_NOTCURSES_HH

#include <cstdio>
#include <ctime>
#include <csignal>
#include <mutex>

#include <notcurses.h>

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
				throw new invalid_state_error ("NotCurses instance not found.");
			if (_instance->nc == nullptr)
				throw new invalid_state_error (ncpp_invalid_state_message);

			return *_instance;
		}

		static bool is_notcurses_stopped ()
		{
			return *_instance == nullptr || _instance->nc == nullptr;
		}

		static const char* enmetric (uintmax_t val, unsigned decimal, char *buf, int omitdec, unsigned mult, int uprefix) noexcept
		{
			return ::enmetric (val, decimal, buf, omitdec, mult, uprefix);
		}

		static const char* qprefix (uintmax_t val, unsigned decimal, char *buf, int omitdec) noexcept
		{
			return ::qprefix (val, decimal, buf, omitdec);
		}

		static const char* bprefix (uintmax_t val, unsigned decimal, char *buf, int omitdec) noexcept
		{
			return ::bprefix (val, decimal, buf, omitdec);
		}

		static const char* version () noexcept
		{
			return notcurses_version ();
		}

		// This is potentially dangerous, but alas necessary. It can cause other calls here to fail in a bad way, but we
		// need a way to report errors to std{out,err} in case of failure and that will work only if notcurses is
		// stopped, so...
		//
		bool stop ()
		{
			if (nc == nullptr)
				throw new invalid_state_error (ncpp_invalid_state_message);

			notcurses_stop (nc);
			nc = nullptr;

			return true;
		}

		bool can_fade () const noexcept
		{
			return notcurses_canfade (nc);
		}

		bool can_open () const noexcept
		{
			return notcurses_canopen (nc);
		}

		bool can_change_color () const noexcept
		{
			return notcurses_canchangecolor (nc);
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
				throw new invalid_argument ("'stats' must be a valid pointer");

			notcurses_reset_stats (nc, stats);
		}

		bool use (const Palette256 *p) const
		{
			if (p == nullptr)
				throw new invalid_argument ("'p' must be a valid pointer");

			return use (*p);
		}

		bool use (const Palette256 &p) const noexcept
		{
			return palette256_use (nc, reinterpret_cast<const palette256*>(&p)) != -1;
		}

		bool render () const noexcept
		{
			return notcurses_render (nc) == 0;
		}

		bool resize (int *rows, int *cols) const noexcept
		{
			return notcurses_resize (nc, rows, cols) == 0;
		}

		bool resize (int &rows, int &cols) const noexcept
		{
			return resize (&rows, &cols) == 0;
		}

		void get_term_dim (int *rows, int *cols) const noexcept
		{
			notcurses_term_dim_yx (nc, rows, cols);
		}

		void get_term_dim (int &rows, int &cols) const noexcept
		{
			get_term_dim (&rows, &cols);
		}

		bool refresh () const noexcept
		{
			return notcurses_refresh (nc) == 0;
		}

		int get_palette_size () const noexcept
		{
			return notcurses_palette_size (static_cast<const notcurses*> (nc));
		}

		bool mouse_enable () const noexcept
		{
			return notcurses_mouse_enable (nc) != -1;
		}

		bool mouse_disable () const noexcept
		{
			return notcurses_mouse_disable (nc) != -1;
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

		char* get_at (int yoff, int xoff, Cell &c) const noexcept
		{
			return notcurses_at_yx (nc, yoff, xoff, c);
		}

		Plane* get_stdplane () noexcept
		{
			return new Plane (notcurses_stdplane (nc), true);
		}

		Plane* get_top () noexcept;

	private:
		notcurses *nc;

		static NotCurses *_instance;
		static std::mutex init_mutex;
	};
}

#endif

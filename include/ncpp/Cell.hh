#ifndef __NCPP_CELL_HH
#define __NCPP_CELL_HH

#include <map>
#include <mutex>
#include <notcurses/notcurses.h>

#include "Root.hh"
#include "CellStyle.hh"

namespace ncpp
{
	class NotCurses;

	class NCPP_API_EXPORT Cell : public Root
	{
	public:
		static constexpr int AlphaHighContrast  = NCALPHA_HIGHCONTRAST;
		static constexpr int AlphaTransparent   = NCALPHA_TRANSPARENT;
		static constexpr int AlphaBlend         = NCALPHA_BLEND;
		static constexpr int AlphaOpaque        = NCALPHA_OPAQUE;

	public:
		Cell (NotCurses *ncinst = nullptr) noexcept
			: Root (ncinst)
		{
			init ();
		}

		explicit Cell (uint32_t c, NotCurses *ncinst = nullptr) noexcept
			: Root (ncinst)
		{
			_cell = NCCELL_CHAR_INITIALIZER (c);
		}

		explicit Cell (uint32_t c, uint16_t a, uint64_t chan, NotCurses *ncinst = nullptr) noexcept
			: Root (ncinst)
		{
			_cell = NCCELL_INITIALIZER (c, a, chan);
		}

		operator nccell* () noexcept
		{
			return &_cell;
		}

		operator nccell const* () const noexcept
		{
			return &_cell;
		}

		nccell& get () noexcept
		{
			return _cell;
		}

		void init () noexcept
		{
			nccell_init (&_cell);
		}

		uint16_t get_stylemask () const noexcept
		{
			return _cell.stylemask;
		}

		uint64_t get_channels () const noexcept
		{
			return _cell.channels;
		}

		void set_styles (CellStyle styles) noexcept
		{
			nccell_set_styles (&_cell, static_cast<unsigned>(styles));
		}

		CellStyle get_styles () noexcept
		{
			return static_cast<CellStyle>(nccell_styles (&_cell));
		}

		void styles_on (CellStyle styles) noexcept
		{
			nccell_on_styles (&_cell, static_cast<unsigned>(styles));
		}

		void styles_off (CellStyle styles) noexcept
		{
			nccell_off_styles (&_cell, static_cast<unsigned>(styles));
		}

		bool is_double_wide () const noexcept
		{
			return nccell_double_wide_p (&_cell);
		}

		unsigned get_fg_rgb () const noexcept
		{
			return nccell_fg_rgb (&_cell);
		}

		unsigned get_bg_rgb () const noexcept
		{
			return nccell_bg_rgb (&_cell);
		}

		unsigned get_fg_alpha () const noexcept
		{
			return nccell_fg_alpha (&_cell);
		}

		bool is_fg_default () const noexcept
		{
			return nccell_fg_default_p (&_cell);
		}

		bool set_fg_alpha (unsigned alpha) noexcept
		{
			return nccell_set_fg_alpha (&_cell, alpha) != -1;
		}

		unsigned get_bg_alpha () const noexcept
		{
			return nccell_bg_alpha (&_cell);
		}

		bool set_bg_alpha (unsigned alpha) noexcept
		{
			return nccell_set_bg_alpha (&_cell, alpha) != -1;
		}

		unsigned get_fg_rgb8 (unsigned *r, unsigned *g, unsigned *b) const noexcept
		{
			return nccell_fg_rgb8 (&_cell, r, g, b);
		}

		bool set_fg_rgb8 (int r, int g, int b, bool clipped = false) noexcept
		{
			if (clipped) {
				nccell_set_fg_rgb8_clipped (&_cell, r, g, b);
				return true;
			}

			return nccell_set_fg_rgb8 (&_cell, r, g, b) != -1;
		}

		void set_fg_rgb (uint32_t channel) noexcept
		{
			nccell_set_fg_rgb (&_cell, channel);
		}

		void set_fg_default () noexcept
		{
			nccell_set_fg_default (&_cell);
		}

		unsigned get_bg_rgb8 (unsigned *r, unsigned *g, unsigned *b) const noexcept
		{
			return nccell_bg_rgb8 (&_cell, r, g, b);
		}

		bool set_bg_rgb8 (int r, int g, int b, bool clipped = false) noexcept
		{
			if (clipped) {
				nccell_set_bg_rgb8_clipped (&_cell, r, g, b);
				return true;
			}

			return nccell_set_bg_rgb8 (&_cell, r, g, b) != -1;
		}

		void set_bg_rgb (uint32_t channel) noexcept
		{
			nccell_set_bg_rgb (&_cell, channel);
		}

		void set_bg_default () noexcept
		{
			nccell_set_bg_default (&_cell);
		}

		bool is_bg_default () const noexcept
		{
			return nccell_bg_default_p (&_cell);
		}

		bool is_wide_right () const noexcept
		{
			return nccell_wide_right_p (&_cell);
		}

		bool is_wide_left () const noexcept
		{
			return nccell_wide_left_p (&_cell);
		}

	private:
		nccell _cell;
	};
}

#endif

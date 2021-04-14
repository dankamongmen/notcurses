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
		static constexpr uint64_t FGDefaultMask = CELL_FGDEFAULT_MASK;
		static constexpr uint64_t FGRGBMask     = CELL_FG_RGB_MASK;
		static constexpr uint64_t BGDefaultMask = CELL_BGDEFAULT_MASK;
		static constexpr uint64_t BGRGBMask     = CELL_BG_RGB_MASK;
		static constexpr uint64_t BGAlphaMask   = CELL_BG_ALPHA_MASK;
		static constexpr uint64_t FGAlphaMask   = CELL_FG_ALPHA_MASK;
		static constexpr int AlphaHighContrast  = CELL_ALPHA_HIGHCONTRAST;
		static constexpr int AlphaTransparent   = CELL_ALPHA_TRANSPARENT;
		static constexpr int AlphaBlend         = CELL_ALPHA_BLEND;
		static constexpr int AlphaOpaque        = CELL_ALPHA_OPAQUE;

	public:
		Cell (NotCurses *ncinst = nullptr) noexcept
			: Root (ncinst)
		{
			init ();
		}

		explicit Cell (uint32_t c, NotCurses *ncinst = nullptr) noexcept
			: Root (ncinst)
		{
			_cell = CELL_CHAR_INITIALIZER (c);
		}

		explicit Cell (uint32_t c, uint16_t a, uint64_t chan, NotCurses *ncinst = nullptr) noexcept
			: Root (ncinst)
		{
			_cell = CELL_INITIALIZER (c, a, chan);
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
			cell_on_styles (&_cell, static_cast<unsigned>(styles));
		}

		void styles_off (CellStyle styles) noexcept
		{
			cell_off_styles (&_cell, static_cast<unsigned>(styles));
		}

		bool is_double_wide () const noexcept
		{
			return cell_double_wide_p (&_cell);
		}

		unsigned get_fg_rgb () const noexcept
		{
			return cell_fg_rgb (&_cell);
		}

		unsigned get_bg_rgb () const noexcept
		{
			return cell_bg_rgb (&_cell);
		}

		unsigned get_fg_alpha () const noexcept
		{
			return cell_fg_alpha (&_cell);
		}

		bool is_fg_default () const noexcept
		{
			return cell_fg_default_p (&_cell);
		}

		bool set_fg_alpha (unsigned alpha) noexcept
		{
			return cell_set_fg_alpha (&_cell, alpha) != -1;
		}

		unsigned get_bg_alpha () const noexcept
		{
			return cell_bg_alpha (&_cell);
		}

		bool set_bg_alpha (unsigned alpha) noexcept
		{
			return cell_set_bg_alpha (&_cell, alpha) != -1;
		}

		unsigned get_fg_rgb8 (unsigned *r, unsigned *g, unsigned *b) const noexcept
		{
			return cell_fg_rgb8 (&_cell, r, g, b);
		}

		bool set_fg_rgb8 (int r, int g, int b, bool clipped = false) noexcept
		{
			if (clipped) {
				cell_set_fg_rgb8_clipped (&_cell, r, g, b);
				return true;
			}

			return cell_set_fg_rgb8 (&_cell, r, g, b) != -1;
		}

		void set_fg_rgb (uint32_t channel) noexcept
		{
			cell_set_fg_rgb (&_cell, channel);
		}

		void set_fg_default () noexcept
		{
			cell_set_fg_default (&_cell);
		}

		unsigned get_bg_rgb8 (unsigned *r, unsigned *g, unsigned *b) const noexcept
		{
			return cell_bg_rgb8 (&_cell, r, g, b);
		}

		bool set_bg_rgb8 (int r, int g, int b, bool clipped = false) noexcept
		{
			if (clipped) {
				cell_set_bg_rgb8_clipped (&_cell, r, g, b);
				return true;
			}

			return cell_set_bg_rgb8 (&_cell, r, g, b) != -1;
		}

		void set_bg_rgb (uint32_t channel) noexcept
		{
			cell_set_bg_rgb (&_cell, channel);
		}

		void set_bg_default () noexcept
		{
			cell_set_bg_default (&_cell);
		}

		bool is_bg_default () const noexcept
		{
			return cell_bg_default_p (&_cell);
		}

		bool is_wide_right () const noexcept
		{
			return cell_wide_right_p (&_cell);
		}

		bool is_wide_left () const noexcept
		{
			return cell_wide_left_p (&_cell);
		}

	private:
		nccell _cell;
	};
}

#endif

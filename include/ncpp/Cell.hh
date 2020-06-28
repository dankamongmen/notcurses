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
		static constexpr uint64_t WideAsianMask = CELL_WIDEASIAN_MASK;
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
			_cell = CELL_SIMPLE_INITIALIZER (c);
		}

		explicit Cell (uint32_t c, uint32_t a, uint64_t chan, NotCurses *ncinst = nullptr) noexcept
			: Root (ncinst)
		{
			_cell = CELL_INITIALIZER (c, a, chan);
		}

		operator cell* () noexcept
		{
			return &_cell;
		}

		operator cell const* () const noexcept
		{
			return &_cell;
		}

		cell& get () noexcept
		{
			return _cell;
		}

		void init () noexcept
		{
			cell_init (&_cell);
		}

		uint32_t get_attrword () const noexcept
		{
			return _cell.attrword;
		}

		uint64_t get_channels () const noexcept
		{
			return _cell.channels;
		}

		uint64_t set_fchannel (uint32_t channel) noexcept
		{
			return cell_set_fchannel (&_cell, channel);
		}

		uint64_t blend_fchannel (unsigned channel, unsigned* blends) noexcept
		{
			return cell_blend_fchannel (&_cell, channel, blends);
		}

		uint64_t set_bchannel (uint32_t channel) noexcept
		{
			return cell_set_bchannel (&_cell, channel);
		}

		uint64_t blend_bchannel (unsigned channel, unsigned* blends) noexcept
		{
			return cell_blend_bchannel (&_cell, channel, blends);
		}

		void set_styles (CellStyle styles) noexcept
		{
			cell_styles_set (&_cell, static_cast<unsigned>(styles));
		}

		CellStyle get_styles () noexcept
		{
			return static_cast<CellStyle>(cell_styles (&_cell));
		}

		void styles_on (CellStyle styles) noexcept
		{
			cell_styles_on (&_cell, static_cast<unsigned>(styles));
		}

		void styles_off (CellStyle styles) noexcept
		{
			cell_styles_off (&_cell, static_cast<unsigned>(styles));
		}

		bool is_double_wide () const noexcept
		{
			return cell_double_wide_p (&_cell);
		}

		bool is_simple () const noexcept
		{
			return cell_simple_p (&_cell);
		}

		uint32_t get_egc_idx () const noexcept
		{
			return cell_egc_idx (&_cell);
		}

		unsigned get_bchannel () const noexcept
		{
			return cell_bchannel (&_cell);
		}

		unsigned get_fchannel () const noexcept
		{
			return cell_fchannel (&_cell);
		}

		unsigned get_fg () const noexcept
		{
			return cell_fg (&_cell);
		}

		unsigned get_bg () const noexcept
		{
			return cell_bg (&_cell);
		}

		unsigned get_fg_alpha () const noexcept
		{
			return cell_fg_alpha (&_cell);
		}

		bool is_fg_default () const noexcept
		{
			return cell_fg_default_p (&_cell);
		}

		bool set_fg_alpha (int alpha) noexcept
		{
			return cell_set_fg_alpha (&_cell, alpha) != -1;
		}

		unsigned get_bg_alpha () const noexcept
		{
			return cell_bg_alpha (&_cell);
		}

		bool set_bg_alpha (int alpha) noexcept
		{
			return cell_set_bg_alpha (&_cell, alpha) != -1;
		}

		unsigned get_fg_rgb (unsigned *r, unsigned *g, unsigned *b) const noexcept
		{
			return cell_fg_rgb (&_cell, r, g, b);
		}

		bool set_fg_rgb (int r, int g, int b, bool clipped = false) noexcept
		{
			if (clipped) {
				cell_set_fg_rgb_clipped (&_cell, r, g, b);
				return true;
			}

			return cell_set_fg_rgb (&_cell, r, g, b) != -1;
		}

		void set_fg (uint32_t channel) noexcept
		{
			cell_set_fg (&_cell, channel);
		}

		void set_fg_default () noexcept
		{
			cell_set_fg_default (&_cell);
		}

		unsigned get_bg_rgb (unsigned *r, unsigned *g, unsigned *b) const noexcept
		{
			return cell_bg_rgb (&_cell, r, g, b);
		}

		bool set_bg_rgb (int r, int g, int b, bool clipped = false) noexcept
		{
			if (clipped) {
				cell_set_bg_rgb_clipped (&_cell, r, g, b);
				return true;
			}

			return cell_set_bg_rgb (&_cell, r, g, b) != -1;
		}

		void set_bg (uint32_t channel) noexcept
		{
			cell_set_bg (&_cell, channel);
		}

		void set_bg_default () noexcept
		{
			cell_set_bg_default (&_cell);
		}

		bool is_bg_default () const noexcept
		{
			return cell_bg_default_p (&_cell);
		}

		bool has_no_foreground () const noexcept
		{
			return cell_noforeground_p (&_cell);
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
		cell _cell;
	};
}

#endif

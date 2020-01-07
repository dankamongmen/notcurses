#ifndef __NCPP_CELL_STYLE_HH
#define __NCPP_CELL_STYLE_HH

#include <cstdint>

#include <notcurses.h>

#include "_flag_enum_operator_helpers.hh"

namespace ncpp
{
	enum class CellStyle : uint32_t
	{
		None      = 0,
		Standout  = CELL_STYLE_STANDOUT,
		Underline = CELL_STYLE_UNDERLINE,
		Reverse   = CELL_STYLE_REVERSE,
		Blink     = CELL_STYLE_BLINK,
		Dim       = CELL_STYLE_DIM,
		Bold      = CELL_STYLE_BOLD,
		Invis     = CELL_STYLE_INVIS,
		Protect   = CELL_STYLE_PROTECT,
		Italic    = CELL_STYLE_ITALIC,
	};

	DECLARE_ENUM_FLAG_OPERATORS (CellStyle)
}
#endif

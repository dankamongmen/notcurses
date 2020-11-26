#ifndef __NCPP_NCSTYLE_HH
#define __NCPP_NCSTYLE_HH

#include <cstdint>

#include <notcurses/notcurses.h>

#include "_flag_enum_operator_helpers.hh"

namespace ncpp
{
	enum class CellStyle : uint32_t
	{
		None      = 0,
		Standout  = NCSTYLE_STANDOUT,
		Underline = NCSTYLE_UNDERLINE,
		Reverse   = NCSTYLE_REVERSE,
		Blink     = NCSTYLE_BLINK,
		Dim       = NCSTYLE_DIM,
		Bold      = NCSTYLE_BOLD,
		Invis     = NCSTYLE_INVIS,
		Protect   = NCSTYLE_PROTECT,
		Italic    = NCSTYLE_ITALIC,
		Struck    = NCSTYLE_STRUCK,
	};

	DECLARE_ENUM_FLAG_OPERATORS (CellStyle)
}
#endif

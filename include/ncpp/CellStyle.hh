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
		Underline = NCSTYLE_UNDERLINE,
		Bold      = NCSTYLE_BOLD,
		Italic    = NCSTYLE_ITALIC,
		Struck    = NCSTYLE_STRUCK,
		Undercurl = NCSTYLE_UNDERCURL,
	};

	DECLARE_ENUM_FLAG_OPERATORS (CellStyle)
}
#endif

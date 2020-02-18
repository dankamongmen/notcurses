#ifndef __NCPP_NCALIGN_HH
#define __NCPP_NCALIGN_HH

#include <notcurses/notcurses.h>

namespace ncpp
{
	enum class NCAlign
	{
		Left   = NCALIGN_LEFT,
		Center = NCALIGN_CENTER,
		Right  = NCALIGN_RIGHT,
	};
}
#endif

#ifndef __NCPP_NCSCALE_HH
#define __NCPP_NCSCALE_HH

#include <notcurses.h>

namespace ncpp
{
	enum class NCScale
	{
		None = NCSCALE_NONE,
		Scale = NCSCALE_SCALE,
		Stretch = NCSCALE_STRETCH,
	};
}
#endif

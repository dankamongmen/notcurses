#ifndef __NCPP_INPUT_HH
#define __NCPP_INPUT_HH

#include <notcurses/notcurses.h>

namespace ncpp
{
	struct EvType
	{
		static constexpr unsigned Silent = ncinput::NCTYPE_UNKNOWN;
		static constexpr unsigned Press = ncinput::NCTYPE_PRESS;
		static constexpr unsigned Repeat = ncinput::NCTYPE_REPEAT;
		static constexpr unsigned Release = ncinput::NCTYPE_RELEASE;
	};
}
#endif

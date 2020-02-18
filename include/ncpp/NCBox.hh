#ifndef __NCPP_NCBOX_HH
#define __NCPP_NCBOX_HH

#include <notcurses/notcurses.h>

namespace ncpp
{
	struct NCBox
	{
		static constexpr unsigned MaskTop	  = NCBOXMASK_TOP;
		static constexpr unsigned MaskRight	  = NCBOXMASK_RIGHT;
		static constexpr unsigned MaskBottom  = NCBOXMASK_BOTTOM;
		static constexpr unsigned MaskLeft	  = NCBOXMASK_LEFT;

		static constexpr unsigned GradTop	  = NCBOXGRAD_TOP;
		static constexpr unsigned GradRight	  = NCBOXGRAD_RIGHT;
		static constexpr unsigned GradBottom  = NCBOXGRAD_BOTTOM;
		static constexpr unsigned GradLeft	  = NCBOXGRAD_LEFT;

		static constexpr unsigned CornerMask  = NCBOXCORNER_MASK;
		static constexpr unsigned CornerShift = NCBOXCORNER_SHIFT;
	};
}
#endif

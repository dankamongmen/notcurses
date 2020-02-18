#ifndef __NCPP_NCLOGLEVEL_HH
#define __NCPP_NCLOGLEVEL_HH

#include <notcurses/notcurses.h>

namespace ncpp
{
	struct NCLogLevel
	{
		static constexpr ncloglevel_e Silent = NCLOGLEVEL_SILENT;
		static constexpr ncloglevel_e Panic = NCLOGLEVEL_PANIC;
		static constexpr ncloglevel_e Fatal = NCLOGLEVEL_FATAL;
		static constexpr ncloglevel_e Error = NCLOGLEVEL_ERROR;
		static constexpr ncloglevel_e Warning = NCLOGLEVEL_WARNING;
		static constexpr ncloglevel_e Info = NCLOGLEVEL_INFO;
		static constexpr ncloglevel_e Verbose = NCLOGLEVEL_VERBOSE;
		static constexpr ncloglevel_e Debug = NCLOGLEVEL_DEBUG;
		static constexpr ncloglevel_e Trace = NCLOGLEVEL_TRACE;
	};
}
#endif

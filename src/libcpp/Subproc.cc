#include <ncpp/Subproc.hh>

using namespace ncpp;

ncsubproc_options Subproc::default_options = {
	nullptr, // curry
	0,			 // restart_period
	0,			 // flags
};

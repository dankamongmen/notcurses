#include <ncpp/Plane.hh>
#include <ncpp/Selector.hh>

using namespace ncpp;

selector_options Selector::default_options = {
	/* title */ nullptr,
	/* secondary */ nullptr,
	/* footer */ nullptr,
	/* items */ nullptr,
	/* itemcount */ 0,
	/* defidx */ 0,
	/* maxdisplay */ 0,
	/* opchannels */ 0,
	/* descchannels */ 0,
	/* titlechannels */ 0,
	/* footchannels */ 0,
	/* boxchannels */ 0,
	/* bgchannels */ 0,
};

Plane* Selector::get_plane () const noexcept
{
	return Plane::map_plane (ncselector_plane (selector));
}

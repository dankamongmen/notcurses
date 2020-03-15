#include <ncpp/Plane.hh>
#include <ncpp/MultiSelector.hh>

using namespace ncpp;

multiselector_options MultiSelector::default_options = {
	/* title */ nullptr,
	/* secondary */ nullptr,
	/* footer */ nullptr,
	/* items */ nullptr,
	/* itemcount */ 0,
	/* maxdisplay */ 0,
	/* opchannels */ 0,
	/* descchannels */ 0,
	/* titlechannels */ 0,
	/* footchannels */ 0,
	/* boxchannels */ 0,
	/* bgchannels */ 0,
};

Plane* MultiSelector::get_plane () const noexcept
{
	return Plane::map_plane (ncmultiselector_plane (multiselector));
}

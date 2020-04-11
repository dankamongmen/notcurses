#include <ncpp/Plot.hh>
#include <ncpp/Plane.hh>

using namespace ncpp;

ncplot_options Plot::default_options = {
	0, // maxchannel
	0, // minchannel
	ncgridgeom_e::NCPLOT_1x1, // ncgridgeom_e
	0, // rangex
	0, // miny
	0, // maxy
	false, // labelaxisd,
	false, // exponentialy
	false, // verticalindep
};

Plane* Plot::get_plane () const noexcept
{
	return Plane::map_plane (ncplot_plane (plot));
}

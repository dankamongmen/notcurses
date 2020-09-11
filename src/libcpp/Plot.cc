#include <ncpp/Plot.hh>
#include <ncpp/Plane.hh>

using namespace ncpp;

ncplot_options PlotD::default_options = {
	0,  // maxchannels
	0,  // minchannels
  0,  // legendstyle
	ncblitter_e::NCBLIT_1x1, // ncblitter_e
	0,  // rangex
  "", // title
	0,  // flags
};

ncplot_options PlotU::default_options = {
	0,  // maxchannels
	0,  // minchannels
  0,  // legendstyle
	ncblitter_e::NCBLIT_1x1, // ncblitter_e
	0,  // rangex
  "", // title
	0,  // flags
};

Plane* PlotD::get_plane () const noexcept
{
	return Plane::map_plane (ncdplot_plane (get_plot ()));
}

Plane* PlotU::get_plane () const noexcept
{
	return Plane::map_plane (ncuplot_plane (get_plot ()));
}

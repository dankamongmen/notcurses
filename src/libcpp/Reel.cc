#include <ncpp/Plane.hh>
#include <ncpp/Reel.hh>
#include <ncpp/NCBox.hh>

using namespace ncpp;

ncreel_options NcReel::default_options = {
	/* bordermask         */ NCBox::MaskBottom | NCBox::MaskTop | NCBox::MaskRight | NCBox::MaskLeft,
	/* borderchan         */ 0,
	/* tabletmask         */ 0,
	/* tabletchan         */ 0,
	/* focusedchan        */ 0,
	/* flags              */ 0,
};

Plane* NcReel::get_plane () const noexcept
{
	return Plane::map_plane (ncreel_plane (reel));
}

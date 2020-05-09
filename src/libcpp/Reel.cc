#include <ncpp/Plane.hh>
#include <ncpp/Reel.hh>
#include <ncpp/NCBox.hh>

using namespace ncpp;

ncreel_options NcReel::default_options = {
	/* min_supported_cols */ 0,
	/* min_supported_rows */ 0,
	/* max_supported_cols */ 0,
	/* max_supported_rows */ 0,
	/* toff               */ 0,
	/* roff               */ 0,
	/* boff               */ 0,
	/* loff               */ 0,
	/* bordermask         */ NCBox::MaskBottom | NCBox::MaskTop | NCBox::MaskRight | NCBox::MaskLeft,
	/* borderchan         */ 0,
	/* tabletmask         */ 0,
	/* tabletchan         */ 0,
	/* focusedchan        */ 0,
	/* bgchannel          */ 0,
  /* flags              */ 0,
};

Plane* NcReel::get_plane () const noexcept
{
	return Plane::map_plane (ncreel_plane (reel));
}

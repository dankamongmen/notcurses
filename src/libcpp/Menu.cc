#include <ncpp/Plane.hh>
#include <ncpp/Menu.hh>

using namespace ncpp;

ncmenu_options Menu::default_options = {
	/* sections */ nullptr,
	/* sectioncount */ 0,
	/* headerchannels */ 0,
	/* sectionchannels */ 0,
  /* flags */ 0,
};

Plane* Menu::get_plane () const noexcept
{
	return Plane::map_plane (ncmenu_plane (menu));
}

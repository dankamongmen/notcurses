#include <ncpp/Visual.hh>
#include <ncpp/Plane.hh>

using namespace ncpp;

Plane* Visual::get_plane () const noexcept
{
	return Plane::map_plane (ncvisual_plane (visual));
}

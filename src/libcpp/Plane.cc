#include <ncpp/Plane.hh>
#include <ncpp/Reel.hh>
#include <ncpp/internal/Helpers.hh>

using namespace ncpp;

std::map<ncplane*,Plane*> *Plane::plane_map = nullptr;
std::mutex Plane::plane_map_mutex;

Plane* Plane::map_plane (ncplane *ncp, Plane *associated_plane) noexcept
{
	if (ncp == nullptr)
		return nullptr;

	return internal::Helpers::lookup_map_entry <ncplane*, Plane*> (
		plane_map,
		plane_map_mutex,
		ncp,
		[&] (ncplane *_ncp) -> Plane* {
			return associated_plane == nullptr ? new Plane (_ncp) : associated_plane;
		}
	);
}

void Plane::unmap_plane (Plane *p) noexcept
{
	if (p == nullptr)
		return;

	internal::Helpers::remove_map_entry (plane_map, plane_map_mutex, p->plane);
}

NcReel* Plane::ncreel_create (const ncreel_options *popts)
{
	return new NcReel (this, popts);
}

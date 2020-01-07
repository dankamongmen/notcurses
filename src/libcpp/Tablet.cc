#include <ncpp/Plane.hh>
#include <ncpp/Tablet.hh>
#include <ncpp/internal/Helpers.hh>

using namespace ncpp;

std::map<tablet*,Tablet*> *Tablet::tablet_map = nullptr;
std::mutex Tablet::tablet_map_mutex;

Tablet* Tablet::map_tablet (tablet *t) noexcept
{
	if (t == nullptr)
		return nullptr;

	return internal::Helpers::lookup_map_entry <tablet*, Tablet*> (
		tablet_map,
		tablet_map_mutex,
		t,
		[&] (tablet *_t) -> Tablet* {
			return new Tablet (_t);
		}
	);
}

void Tablet::unmap_tablet (Tablet *p) noexcept
{
	if (p == nullptr)
		return;

	internal::Helpers::remove_map_entry (tablet_map, tablet_map_mutex, p->_tablet);
}

Plane* Tablet::get_plane () const noexcept
{
	return Plane::map_plane (tablet_ncplane (_tablet));
}

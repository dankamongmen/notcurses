#include <ncpp/Plane.hh>
#include <ncpp/Tablet.hh>
#include <ncpp/internal/Helpers.hh>

using namespace ncpp;

std::map<nctablet*,NcTablet*> *NcTablet::tablet_map = nullptr;
std::mutex NcTablet::tablet_map_mutex;

NcTablet* NcTablet::map_tablet (nctablet *t, NotCurses *ncinst) noexcept
{
	if (t == nullptr)
		return nullptr;

	return internal::Helpers::lookup_map_entry <nctablet*, NcTablet*> (
		tablet_map,
		tablet_map_mutex,
		t,
		[&] (nctablet *_t) -> NcTablet* {
			return new NcTablet (_t, ncinst);
		}
	);
}

void NcTablet::unmap_tablet (NcTablet *p) noexcept
{
	if (p == nullptr)
		return;

	internal::Helpers::remove_map_entry (tablet_map, tablet_map_mutex, p->_tablet);
}

Plane* NcTablet::get_plane () const noexcept
{
	return Plane::map_plane (nctablet_plane (_tablet));
}

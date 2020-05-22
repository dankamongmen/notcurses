#include <ncpp/Utilities.hh>
#include <ncpp/Plane.hh>

using namespace ncpp;

ncplane* Utilities::to_ncplane (const Plane *plane) noexcept
{
	if (plane == nullptr)
		return nullptr;

	return const_cast<Plane*>(plane)->to_ncplane ();
}

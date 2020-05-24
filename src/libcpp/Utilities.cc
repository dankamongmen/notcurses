#include <ncpp/Utilities.hh>
#include <ncpp/Plane.hh>

using namespace ncpp;

ncplane* Utilities::to_ncplane (const Plane *plane) noexcept
{
	if (plane == nullptr)
		return nullptr;

	return const_cast<Plane*>(plane)->to_ncplane ();
}

NotCurses* Utilities::get_notcurses_cpp (const Root *o) noexcept
{
	if (o == nullptr)
		return nullptr;

	return const_cast<Root*>(o)->get_notcurses_cpp ();
}

NotCurses* Utilities::get_notcurses_cpp (const Plane *plane) noexcept
{
	return get_notcurses_cpp (static_cast<const Root*>(plane));
}

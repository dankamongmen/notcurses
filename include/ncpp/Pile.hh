#ifndef __NCPP_PILE_HH
#define __NCPP_PILE_HH

#include <exception>
#include <notcurses/notcurses.h>

#include "NotCurses.hh"
#include "Plane.hh"

namespace ncpp
{
	class NCPP_API_EXPORT Pile : public Plane
	{
	public:
		Pile (ncplane_options const* nopts, NotCurses* ncinst = nullptr)
			: Plane (ncinst)
		{
			if (nopts == nullptr) {
				throw invalid_argument ("'nopts' must be a valid pointer");
			}

			notcurses *n;
			if (ncinst == nullptr) {
				n = NotCurses::get_instance ();
			} else {
				n = *ncinst;
			}

			ncplane *pile = ncpile_create (n, nopts);
			if (pile == nullptr) {
				throw init_error ("Notcurses failed to create a new pile");
			}

			set_plane (pile);
		}

		bool render () const NOEXCEPT_MAYBE
		{
			return error_guard (ncpile_render (to_ncplane ()), -1);
		}

		bool rasterize () const NOEXCEPT_MAYBE
		{
			return error_guard (ncpile_rasterize (to_ncplane ()), -1);
		}

		bool show () const NOEXCEPT_MAYBE
		{
			if (!render ()) {
				return false;
			}

			return rasterize ();
		}

		static Plane* top_with (const Plane& plane) noexcept
		{
			ncplane* ret = ncpile_top (const_cast<Plane&>(plane));
			if (ret == nullptr) {
				return nullptr;
			}

			return map_plane (ret);
		}

		static Plane* bottom_with (const Plane& plane) noexcept
		{
			ncplane* ret = ncpile_bottom (const_cast<Plane&>(plane));
			if (ret == nullptr) {
				return nullptr;
			}

			return map_plane (ret);
		}
	};
}
#endif

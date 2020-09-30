#ifndef __NCPP_WIDGET_HH
#define __NCPP_WIDGET_HH

#include "Root.hh"
#include "Plane.hh"

namespace ncpp
{
	class NCPP_API_EXPORT Widget : public Root
	{
	protected:
		explicit Widget (NotCurses *ncinst)
			: Root (ncinst)
		{}

		void ensure_valid_plane (Plane *plane) const
		{
			if (plane == nullptr)
				throw invalid_argument ("'plane' must be a valid pointer");
			ensure_valid_plane (*plane);
		}

		void ensure_valid_plane (Plane &plane) const
		{
			if (!plane.is_valid ())
				throw invalid_argument ("Invalid Plane object passed in 'plane'. Widgets must not reuse the same plane.");
		}

		void take_plane_ownership (Plane *plane) const
		{
			if (plane == nullptr)
				return;

			take_plane_ownership (*plane);
		}

		void take_plane_ownership (Plane &plane) const
		{
			plane.release_native_plane ();
		}
	};
}
#endif // __NCPP_WIDGET_HH

#ifndef __NCPP_FDPLANE_HH
#define __NCPP_FDPLANE_HH

#include <notcurses/notcurses.h>

#include "Utilities.hh"
#include "Plane.hh"
#include "Widget.hh"

namespace ncpp
{
	class NCPP_API_EXPORT FDPlane : public Widget
	{
	public:
		static ncfdplane_options default_options;

	public:
		explicit FDPlane (Plane *plane, int fd, ncfdplane_callback cbfxn = nullptr, ncfdplane_done_cb donecbfxn = nullptr)
			: FDPlane (plane, fd, nullptr, cbfxn, donecbfxn)
		{}

		explicit FDPlane (Plane *plane, int fd, ncfdplane_options *opts = nullptr, ncfdplane_callback cbfxn = nullptr, ncfdplane_done_cb donecbfxn = nullptr)
			: Widget (Utilities::get_notcurses_cpp (plane))
		{
			ensure_valid_plane (plane);
			create_fdplane (*plane, fd, opts, cbfxn, donecbfxn);
			take_plane_ownership (plane);
		}

		explicit FDPlane (Plane &plane, int fd, ncfdplane_callback cbfxn = nullptr, ncfdplane_done_cb donecbfxn = nullptr)
			: FDPlane (plane, fd, nullptr, cbfxn, donecbfxn)
		{}

		explicit FDPlane (Plane &plane, int fd, ncfdplane_options *opts = nullptr, ncfdplane_callback cbfxn = nullptr, ncfdplane_done_cb donecbfxn = nullptr)
			: Widget (Utilities::get_notcurses_cpp (plane))
		{
			ensure_valid_plane (plane);
			create_fdplane (plane, fd, opts, cbfxn, donecbfxn);
			take_plane_ownership (plane);
		}

		~FDPlane ()
		{
			if (is_notcurses_stopped ())
				return;

			ncfdplane_destroy (fdplane);
		}

		Plane* get_plane () const noexcept
		{
			return Plane::map_plane (ncfdplane_plane (fdplane));
		}

	private:
		void create_fdplane (Plane& n, int fd, ncfdplane_options *opts, ncfdplane_callback cbfxn, ncfdplane_done_cb donecbfxn)
		{
			fdplane = ncfdplane_create (
				n,
				opts == nullptr ? &default_options : opts,
				fd,
				cbfxn,
				donecbfxn
			);

			if (fdplane == nullptr)
				throw init_error ("NotCurses failed to create an ncfdplane instance");
		}

	private:
		ncfdplane *fdplane;
	};
}
#endif // __NCPP_FDPLANE_HH

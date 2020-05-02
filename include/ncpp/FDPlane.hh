#ifndef __NCPP_FDPLANE_HH
#define __NCPP_FDPLANE_HH

#include <notcurses/notcurses.h>

#include "Root.hh"
#include "Plane.hh"

namespace ncpp
{
	class NCPP_API_EXPORT FDPlane : public Root
	{
	public:
		static ncfdplane_options default_options;

	public:
		explicit FDPlane (Plane* n, int fd, ncfdplane_callback cbfxn = nullptr, ncfdplane_done_cb donecbfxn = nullptr)
			: FDPlane (n, fd, nullptr, cbfxn, donecbfxn)
		{}

		explicit FDPlane (Plane* n, int fd, ncfdplane_options *opts = nullptr, ncfdplane_callback cbfxn = nullptr, ncfdplane_done_cb donecbfxn = nullptr)
		{
			if (n == nullptr)
				throw invalid_argument ("'n' must be a valid pointer");
			create_fdplane (*n, fd, opts, cbfxn, donecbfxn);
		}

		explicit FDPlane (Plane& n, int fd, ncfdplane_callback cbfxn = nullptr, ncfdplane_done_cb donecbfxn = nullptr)
			: FDPlane (n, fd, nullptr, cbfxn, donecbfxn)
		{}

		explicit FDPlane (Plane& n, int fd, ncfdplane_options *opts = nullptr, ncfdplane_callback cbfxn = nullptr, ncfdplane_done_cb donecbfxn = nullptr)
		{
			create_fdplane (n, fd, opts, cbfxn, donecbfxn);
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

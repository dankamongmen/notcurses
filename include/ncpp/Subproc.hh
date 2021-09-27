#ifndef __NCPP_SUBPROC_HH
#define __NCPP_SUBPROC_HH

#include <notcurses/notcurses.h>

#include "Root.hh"
#include "Plane.hh"
#include "Widget.hh"
#include "Utilities.hh"

namespace ncpp
{
	class NCPP_API_EXPORT Subproc : public Widget
	{
	public:
		static ncsubproc_options default_options;

	public:
		explicit Subproc (Plane* plane, const char* bin, bool use_path = true,
		                  char* const arg[] = nullptr, char* const env[] = nullptr,
		                  ncfdplane_callback cbfxn = nullptr, ncfdplane_done_cb donecbfxn = nullptr)
			: Subproc (plane, bin, nullptr, use_path, arg, env, cbfxn, donecbfxn)
		{}

		explicit Subproc (Plane* plane, const char* bin, const ncsubproc_options* opts, bool use_path = true,
		                  char* const arg[] = nullptr, char* const env[] = nullptr,
		                  ncfdplane_callback cbfxn = nullptr, ncfdplane_done_cb donecbfxn = nullptr)
			: Widget (Utilities::get_notcurses_cpp (plane))
		{
			ensure_valid_plane (plane);
			create_subproc (*plane, bin, opts, use_path, arg, env, cbfxn, donecbfxn);
			take_plane_ownership (plane);
		}

		explicit Subproc (Plane& plane, const char* bin, bool use_path = true,
		                  char* const arg[] = nullptr, char* const env[] = nullptr,
		                  ncfdplane_callback cbfxn = nullptr, ncfdplane_done_cb donecbfxn = nullptr)
			: Subproc (plane, bin, nullptr, use_path, arg, env, cbfxn, donecbfxn)
		{}

		explicit Subproc (Plane& plane, const char* bin, const ncsubproc_options* opts, bool use_path = true,
		                  char* const arg[] = nullptr, char* const env[] = nullptr,
		                  ncfdplane_callback cbfxn = nullptr, ncfdplane_done_cb donecbfxn = nullptr)
			: Widget (Utilities::get_notcurses_cpp (plane))
		{
			ensure_valid_plane (plane);
			create_subproc (plane, bin, opts, use_path, arg, env, cbfxn, donecbfxn);
			take_plane_ownership (plane);
		}

		~Subproc ()
		{
			if (is_notcurses_stopped ())
				return;

			ncsubproc_destroy (subproc);
		}

		Plane* get_plane () const noexcept
		{
			return Plane::map_plane (ncsubproc_plane (subproc));
		}

	private:
		void create_subproc (Plane& n, const char* bin, const ncsubproc_options* opts, bool use_path,
		                     char* const arg[], char* const env[],
		                     ncfdplane_callback cbfxn, ncfdplane_done_cb donecbfxn)
		{
			if (bin == nullptr)
				throw invalid_argument ("'bin' must be a valid pointer");

			if (opts == nullptr)
				opts = &default_options;

			if (use_path) {
				if (env != nullptr) {
					subproc = ncsubproc_createvpe (
						n, opts, bin, arg, env, cbfxn, donecbfxn
					);
				} else {
					subproc = ncsubproc_createvp (
						n, opts, bin, arg, cbfxn, donecbfxn
					);
				}
			} else {
				subproc = ncsubproc_createv (
					n, opts, bin, arg, cbfxn, donecbfxn
				);
			}

			if (subproc == nullptr)
				throw init_error ("Notcurses failed to create ncsubproc instance");
		}

	private:
		ncsubproc *subproc;
	};
}
#endif // __NCPP_SUBPROC_HH

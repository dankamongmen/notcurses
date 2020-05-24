#ifndef __NCPP_SUBPROC_HH
#define __NCPP_SUBPROC_HH

#include <notcurses/notcurses.h>

#include "Root.hh"
#include "Plane.hh"

namespace ncpp
{
	class NCPP_API_EXPORT Subproc : public Root
	{
	public:
		static ncsubproc_options default_options;

	public:
		explicit Subproc (Plane* n, const char* bin, bool use_path = true,
		                  char* const arg[] = nullptr, char* const env[] = nullptr,
		                  ncfdplane_callback cbfxn = nullptr, ncfdplane_done_cb donecbfxn = nullptr)
			: Subproc (n, bin, nullptr, use_path, arg, env, cbfxn, donecbfxn)
		{}

		explicit Subproc (const Plane* n, const char* bin, bool use_path = true,
		                  char* const arg[] = nullptr, char* const env[] = nullptr,
		                  ncfdplane_callback cbfxn = nullptr, ncfdplane_done_cb donecbfxn = nullptr)
			: Subproc (n, bin, nullptr, use_path, arg, env, cbfxn, donecbfxn)
		{}

		explicit Subproc (Plane* n, const char* bin, const ncsubproc_options* opts, bool use_path = true,
		                  char* const arg[] = nullptr, char* const env[] = nullptr,
		                  ncfdplane_callback cbfxn = nullptr, ncfdplane_done_cb donecbfxn = nullptr)
			: Subproc (static_cast<const Plane*>(n), bin, opts, use_path, arg, env, cbfxn, donecbfxn)
		{}

		explicit Subproc (const Plane* n, const char* bin, const ncsubproc_options* opts, bool use_path = true,
		                  char* const arg[] = nullptr, char* const env[] = nullptr,
		                  ncfdplane_callback cbfxn = nullptr, ncfdplane_done_cb donecbfxn = nullptr)
			: Root (Utilities::get_notcurses_cpp (n))
		{
			if (n == nullptr)
				throw invalid_argument ("'n' must be a valid pointer");
			create_subproc (const_cast<Plane&>(*n), bin, opts, use_path, arg, env, cbfxn, donecbfxn);
		}

		explicit Subproc (Plane const& n, const char* bin, bool use_path = true,
		                  char* const arg[] = nullptr, char* const env[] = nullptr,
		                  ncfdplane_callback cbfxn = nullptr, ncfdplane_done_cb donecbfxn = nullptr)
			: Subproc (n, bin, nullptr, use_path, arg, env, cbfxn, donecbfxn)
		{}

		explicit Subproc (Plane& n, const char* bin, bool use_path = true,
		                  char* const arg[] = nullptr, char* const env[] = nullptr,
		                  ncfdplane_callback cbfxn = nullptr, ncfdplane_done_cb donecbfxn = nullptr)
			: Subproc (n, bin, nullptr, use_path, arg, env, cbfxn, donecbfxn)
		{}

		explicit Subproc (Plane& n, const char* bin, const ncsubproc_options* opts, bool use_path = true,
		                  char* const arg[] = nullptr, char* const env[] = nullptr,
		                  ncfdplane_callback cbfxn = nullptr, ncfdplane_done_cb donecbfxn = nullptr)
			: Subproc (static_cast<Plane const&>(n), bin, opts, use_path, arg, env, cbfxn, donecbfxn)
		{}

		explicit Subproc (const Plane& n, const char* bin, const ncsubproc_options* opts, bool use_path = true,
		                  char* const arg[] = nullptr, char* const env[] = nullptr,
		                  ncfdplane_callback cbfxn = nullptr, ncfdplane_done_cb donecbfxn = nullptr)
			: Root (Utilities::get_notcurses_cpp (n))
		{
			create_subproc (const_cast<Plane&>(n), bin, opts, use_path, arg, env, cbfxn, donecbfxn);
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
				throw new init_error ("Notcurses failed to create ncsubproc instance");
		}

	private:
		ncsubproc *subproc;
	};
}
#endif // __NCPP_SUBPROC_HH

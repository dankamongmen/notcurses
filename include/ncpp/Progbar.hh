#ifndef __NCPP_PROGBAR_HH
#define __NCPP_PROGBAR_HH

#include <exception>
#include <notcurses/notcurses.h>

#include "NotCurses.hh"
#include "Plane.hh"

namespace ncpp
{
	class NCPP_API_EXPORT Progbar : public Root
	{
	public:
		Progbar (Plane& n, const ncprogbar_options* opts)
			: Root (nullptr)
		{
			if (opts == nullptr) {
				throw invalid_argument ("Argument 'opts' must be a valid pointer");
			}

			progbar = ncprogbar_create (n, opts);
			if (progbar == nullptr) {
				throw init_error ("Notcurses failed to create ncprogbar");
			}
		}

		~Progbar ()
		{
			if (!is_notcurses_stopped ()) {
				ncprogbar_destroy (progbar);
			}
		}

		Plane* get_plane () const noexcept
		{
			ncplane *ret = ncprogbar_plane (progbar);
			if (ret == nullptr) {
				return nullptr;
			}

			return Plane::map_plane (ret);
		}

		void set_progress (double p) const noexcept
		{
			ncprogbar_set_progress (progbar, p);
		}

		double get_progress () const noexcept
		{
			return ncprogbar_progress (progbar);
		}

	private:
		ncprogbar *progbar = nullptr;
	};
}

#endif // __NCPP_PROGBAR_HH

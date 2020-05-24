#ifndef __NCPP_READER_HH
#define __NCPP_READER_HH

#include <notcurses/notcurses.h>

#include "NCAlign.hh"
#include "Plane.hh"
#include "Utilities.hh"

namespace ncpp
{
	class NCPP_API_EXPORT Reader : public Root
	{
	public:
		explicit Reader (Plane *p, int y, int x, const ncreader_options *opts)
			: Reader (static_cast<const Plane*>(p), y, x, opts)
		{}

		explicit Reader (Plane const* p, int y, int x, const ncreader_options *opts)
			: Root (Utilities::get_notcurses_cpp (p))
		{
			if (p == nullptr)
				throw invalid_argument ("'plane' must be a valid pointer");

			common_init (Utilities::to_ncplane (p), y, x, opts);
		}

		explicit Reader (Plane &p, int y, int x, const ncreader_options *opts)
			: Reader (static_cast<Plane const&>(p), y, x, opts)
		{}

		explicit Reader (Plane const& p, int y, int x, const ncreader_options *opts)
			: Root (Utilities::get_notcurses_cpp (p))
		{
			common_init (Utilities::to_ncplane (p), y, x, opts);
		}

		~Reader ()
		{
			if (!is_notcurses_stopped ())
				ncreader_destroy (reader, nullptr);
		}

		bool clear () const NOEXCEPT_MAYBE
		{
			bool ret = ncreader_clear (reader) != 0;
			return error_guard_cond<bool, bool> (ret, ret);
		}

		char* get_contents () const noexcept
		{
			return ncreader_contents(reader);
		}

		Plane* get_plane () const noexcept
		{
			return Plane::map_plane (ncreader_plane (reader));
		}

	private:
		void common_init (ncplane *n, int y, int x, const ncreader_options *opts)
		{
			reader = ncreader_create (n, y, x, opts);
			if (reader == nullptr)
				throw init_error ("Notcurses failed to create a new reader");
		}

	private:
		ncreader *reader;
	};
}
#endif

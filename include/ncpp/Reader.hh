#ifndef __NCPP_READER_HH
#define __NCPP_READER_HH

#include <notcurses/notcurses.h>

#include "Root.hh"
#include "NCAlign.hh"
#include "NotCurses.hh"

namespace ncpp
{
	class Plane;

	class NCPP_API_EXPORT Reader : public Root
	{
	public:
		explicit Reader (Plane *p, int y, int x, const ncreader_options *opts)
			: Reader (reinterpret_cast<ncplane*>(p), y, x, opts)
		{}

		explicit Reader (Plane const* p, int y, int x, const ncreader_options *opts)
			: Reader (const_cast<Plane*>(p), y, x, opts)
		{}

		explicit Reader (Plane &p, int y, int x, const ncreader_options *opts)
			: Reader (reinterpret_cast<Plane*>(&p), y, x, opts)
		{}

		explicit Reader (Plane const& p, int y, int x, const ncreader_options *opts)
			: Reader (const_cast<Plane*>(&p), y, x, opts)
		{}

		explicit Reader (ncplane* n, int y, int x, const ncreader_options *opts)
		{
			reader = ncreader_create (n, y, x, opts);
			if (reader == nullptr)
				throw init_error ("Notcurses failed to create a new reader");
		}

		~Reader ()
		{
			if (!is_notcurses_stopped ())
				ncreader_destroy (reader, nullptr);
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
		ncreader *reader;
	};
}
#endif

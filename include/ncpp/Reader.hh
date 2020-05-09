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
		explicit Reader (NotCurses *nc, int y, int x, const ncreader_options *opts)
			: Reader (reinterpret_cast<notcurses*>(nc), y, x, opts)
		{}

		explicit Reader (NotCurses const* nc, int y, int x, const ncreader_options *opts)
			: Reader (const_cast<NotCurses*>(nc), y, x, opts)
		{}

		explicit Reader (NotCurses &nc, int y, int x, const ncreader_options *opts)
			: Reader (reinterpret_cast<NotCurses*>(&nc), y, x, opts)
		{}

		explicit Reader (NotCurses const& nc, int y, int x, const ncreader_options *opts)
			: Reader (const_cast<NotCurses*>(&nc), y, x, opts)
		{}

		explicit Reader (notcurses* nc, int y, int x, const ncreader_options *opts)
		{
			reader = ncreader_create (nc, y, x, opts);
			if (reader == nullptr)
				throw init_error ("notcurses failed to create a new reader");
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

#ifndef __NCPP_PALETTE256_HH
#define __NCPP_PALETTE256_HH

#include "Root.hh"
#include "_helpers.hh"

namespace ncpp
{
	class NCPP_API_EXPORT Palette256 : public Root
	{
	public:
		Palette256 ()
		{
			palette = palette256_new (get_notcurses ());
			if (palette == nullptr)
				throw init_error ("notcurses failed to create a new palette");
		}

		~Palette256 ()
		{
			palette256_free (palette);
		}

		operator palette256* () noexcept
		{
			return palette;
		}

		operator palette256 const* () const noexcept
		{
			return palette;
		}

		bool set (int idx, int r, int g, int b) const noexcept
		{
			return palette256_set_rgb (palette, idx, r, g, b) != -1;
		}

		bool set (int idx, unsigned rgb) const noexcept
		{
			return palette256_set (palette, idx, rgb) != -1;
		}

		bool get (int idx, unsigned *r, unsigned *g, unsigned *b) const
		{
			if (r == nullptr)
				throw invalid_argument ("'r' must be a valid pointer");
			if (g == nullptr)
				throw invalid_argument ("'g' must be a valid pointer");
			if (b == nullptr)
				throw invalid_argument ("'b' must be a valid pointer");

			return get (idx, *r, *g, *b);
		}

		bool get (int idx, unsigned &r, unsigned &g, unsigned &b) const noexcept
		{
			return palette256_get_rgb (palette, idx, &r, &g, &b) != -1;
		}

	private:
		palette256 *palette;
	};
}
#endif

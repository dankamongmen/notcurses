#ifndef __NCPP_TABLET_HH
#define __NCPP_TABLET_HH

#include <map>
#include <mutex>

#include <notcurses/notcurses.h>

#include "Root.hh"

namespace ncpp
{
	class Plane;
	class NotCurses;

	class NCPP_API_EXPORT NcTablet : public Root
	{
	protected:
		explicit NcTablet (nctablet *t, NotCurses *ncinst)
			: Root (ncinst),
			  _tablet (t)
		{
			if (t == nullptr)
				throw invalid_argument ("'t' must be a valid pointer");
		};

	public:
		template<typename T>
		T* get_userptr () const noexcept
		{
			return static_cast<T*>(nctablet_userptr (_tablet));
		}

		operator nctablet* () const noexcept
		{
			return _tablet;
		}

		operator nctablet const* () const noexcept
		{
			return _tablet;
		}

		Plane* get_plane () const noexcept;
		static NcTablet* map_tablet (nctablet *t, NotCurses *ncinst = nullptr) noexcept;

	protected:
		static void unmap_tablet (NcTablet *p) noexcept;

		nctablet* get_tablet () const noexcept
		{
			return _tablet;
		}

	private:
		nctablet *_tablet = nullptr;
		static std::map<nctablet*,NcTablet*> *tablet_map;
		static std::mutex tablet_map_mutex;

		friend class NcReel;
	};
}
#endif

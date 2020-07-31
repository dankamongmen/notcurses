#ifndef __NCPP_ROOT_HH
#define __NCPP_ROOT_HH

#include <type_traits>
#include <string>
#include <notcurses/notcurses.h>

#include "_helpers.hh"
#include "_exceptions.hh"

namespace ncpp {
#if defined (NCPP_EXCEPTIONS_PLEASE)
#define NOEXCEPT_MAYBE
#else
#define NOEXCEPT_MAYBE noexcept
#endif

	class NotCurses;

	class NCPP_API_EXPORT Root
	{
	protected:
		static constexpr char ncpp_invalid_state_message[] = "notcurses++ is in an invalid state (already stopped?)";

	public:
		notcurses* get_notcurses () const;

		NotCurses* get_notcurses_cpp () const
		{
			return nc;
		}

	protected:
		explicit Root (NotCurses *ncinst)
			: nc (ncinst)
		{}

		template<typename TRet = bool, typename TValue = int>
		static TRet error_guard (TValue ret, TValue error_value)
		{
			static constexpr bool ret_is_bool = std::is_same_v<TRet, bool>;

			if constexpr (!ret_is_bool) {
				static_assert (std::is_same_v<TRet, TValue>, "Both TRet and TValue must be the same type unless TValue is 'bool'");
			}

			if (ret != error_value) {
				if constexpr (ret_is_bool) {
					return true;
				} else {
					return ret;
				}
			}

#if defined (NCPP_EXCEPTIONS_PLEASE)
			throw call_error ("Call returned an error value");
#else
			if constexpr (ret_is_bool) {
				return false;
			} else {
				return ret;
			}
#endif
		}

		template<typename TRet = bool, typename TValue = int>
		static TRet error_guard_cond ([[maybe_unused]] TValue ret, bool error_value)
		{
			static constexpr bool ret_is_bool = std::is_same_v<TRet, bool>;

			if constexpr (!ret_is_bool) {
				static_assert (std::is_same_v<TRet, TValue>, "Both TRet and TValue must be the same type unless TValue is 'bool'");
			}

			if (!error_value) {
				if constexpr (ret_is_bool) {
					return true;
				} else {
					return ret;
				}
			}

#if defined (NCPP_EXCEPTIONS_PLEASE)
			throw call_error ("Call returned an error value");
#else
			if constexpr (ret_is_bool) {
				return false;
			} else {
				return ret;
			}
#endif
		}

		// All the objects which need to destroy notcurses entities (planes, panelreel etc etc) **have to** call this
		// function before calling to notcurses from their destructor. This is to prevent a segfault when
		// NotCurses::stop has been called and the app uses smart pointers holding NotCurses objects which may be
		// destructed **after** notcurses is stopped.
		bool is_notcurses_stopped () const noexcept;

	private:
		NotCurses *nc = nullptr;
	};
}
#endif

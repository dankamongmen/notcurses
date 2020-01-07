#ifndef __NCPP_FLAG_ENUM_OPERATOR_HELPERS_H
#define __NCPP_FLAG_ENUM_OPERATOR_HELPERS_H

#include <type_traits>

#include "_helpers.hh"

namespace ncpp
{
#define DECLARE_ENUM_FLAG_OPERATORS(__enum_name__)                      \
	NCPP_API_EXPORT constexpr __enum_name__ operator& (__enum_name__ x, __enum_name__ y) \
	{ \
		typedef std::underlying_type<__enum_name__>::type etype; \
		return __enum_name__ (static_cast<etype> (x) & static_cast<etype> (y)); \
	} \
	NCPP_API_EXPORT constexpr __enum_name__& operator&= (__enum_name__& x, __enum_name__ y) \
	{ \
		typedef std::underlying_type<__enum_name__>::type etype; \
		return x = __enum_name__ (static_cast<etype> (x) & static_cast<etype> (y)); \
	} \
	NCPP_API_EXPORT constexpr __enum_name__ operator| (__enum_name__ x, __enum_name__ y) \
	{ \
		typedef std::underlying_type<__enum_name__>::type etype; \
		return __enum_name__ (static_cast<etype> (x) | static_cast<etype> (y)); \
	} \
	NCPP_API_EXPORT constexpr __enum_name__& operator|= (__enum_name__& x, __enum_name__ y) \
	{ \
		typedef std::underlying_type<__enum_name__>::type etype; \
		return x = __enum_name__ (static_cast<etype> (x) | static_cast<etype> (y)); \
	} \
	NCPP_API_EXPORT constexpr __enum_name__ operator^ (__enum_name__ x, __enum_name__ y) \
	{ \
		typedef std::underlying_type<__enum_name__>::type etype; \
		return __enum_name__ (static_cast<etype> (x) ^ static_cast<etype> (y)); \
	} \
	NCPP_API_EXPORT constexpr __enum_name__& operator^= (__enum_name__& x, __enum_name__ y) \
	{ \
		typedef std::underlying_type<__enum_name__>::type etype; \
		return x = __enum_name__ (static_cast<etype> (x) ^ static_cast<etype> (y)); \
	} \
	NCPP_API_EXPORT constexpr __enum_name__& operator~ (__enum_name__& x) \
	{ \
		typedef std::underlying_type<__enum_name__>::type etype; \
		return x = __enum_name__ (~static_cast<etype> (x)); \
	} \
	NCPP_API_EXPORT constexpr __enum_name__ operator~ (__enum_name__ x) \
	{ \
		typedef std::underlying_type<__enum_name__>::type etype; \
		return __enum_name__ (~static_cast<etype> (x)); \
	}
}
#endif

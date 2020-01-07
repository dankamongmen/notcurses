#ifndef __NCPP_HELPERS_HH
#define __NCPP_HELPERS_HH

namespace ncpp
{
#define NCPP_LIKELY(expr) (__builtin_expect ((expr) != 0, 1))
#define NCPP_UNLIKELY(expr) (__builtin_expect ((expr) != 0, 0))

#define NCPP_API_EXPORT __attribute__((visibility ("default")))
#define NCPP_API_LOCAL __attribute__((visibility ("hidden")))
}
#endif

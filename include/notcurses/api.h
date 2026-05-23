#ifdef API
#undef API
#endif

#ifdef __MINGW32__
# define API __declspec(dllexport)
#else
# define API __attribute__((visibility("default")))
#endif

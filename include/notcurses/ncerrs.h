#ifndef NOTCURSES_NCERRS
#define NOTCURSES_NCERRS

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>

// Error values for more granular problem indication. We map to POSIX error
// codes when possible. We need at least the union of POSIX (errno) and FFMpeg
// (AVERROR) codes that we might see.
typedef enum {
  NCERR_SUCCESS = 0,
  NCERR_NOMEM = ENOMEM,
  NCERR_EOF = 0x20464f45, // matches AVERROR_EOF
  NCERR_DECODE,
  NCERR_UNIMPLEMENTED,
} nc_err_e;

static inline const char*
nc_strerror(nc_err_e ncerr){
  switch(ncerr){
    case NCERR_SUCCESS: return "success";
    case NCERR_NOMEM: return strerror(ENOMEM);
    case NCERR_EOF: return "end of file";
    case NCERR_DECODE: return "error decoding";
    case NCERR_UNIMPLEMENTED: return "feature not available";
  };
  return "unknown error";
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif

#include "unicode.h"

#include <limits.h>
#include <string.h>

int ncunicode_ucs32_to_utf8(const uint32_t* ucs32, unsigned ucs32count,
                            unsigned char* resultbuf, size_t buflen){
#ifdef USE_UTF8PROC
  size_t written = 0;

  for(unsigned i = 0 ; i < ucs32count ; ++i){
    utf8proc_int32_t codepoint = (utf8proc_int32_t)ucs32[i];
    unsigned char encoded[4];

    if(!utf8proc_codepoint_valid(codepoint)){
      return -1;
    }
    utf8proc_ssize_t bytes = utf8proc_encode_char(codepoint, encoded);
    if(bytes <= 0 || written + (size_t)bytes > buflen){
      return -1;
    }
    memcpy(resultbuf + written, encoded, (size_t)bytes);
    written += (size_t)bytes;
  }
  if(written < buflen){
    resultbuf[written] = '\0';
  }
  if(written > INT_MAX){
    return -1;
  }
  return (int)written;
#else
  if(u32_to_u8(ucs32, ucs32count, resultbuf, &buflen) == NULL){
    return -1;
  }
  return buflen;
#endif
}
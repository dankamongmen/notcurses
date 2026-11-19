#ifndef NOTCURSES_UNICODE
#define NOTCURSES_UNICODE

#include "builddef.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <wchar.h>

#ifdef USE_UTF8PROC
#include <utf8proc.h>
#else
#include <unigbrk.h>
#include <unictype.h>
#include <unistr.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

static inline bool
ncunicode_grapheme_break(wchar_t prev, wchar_t next){
#ifdef USE_UTF8PROC
  return utf8proc_grapheme_break((utf8proc_int32_t)prev,
                                 (utf8proc_int32_t)next);
#else
  return uc_is_grapheme_break(prev, next);
#endif
}

static inline bool
ncunicode_variation_selector(wchar_t wchar){
#ifdef USE_UTF8PROC
  const uint32_t codepoint = (uint32_t)wchar;
  return (codepoint >= 0x180b && codepoint <= 0x180d) ||
         codepoint == 0x180f ||
         (codepoint >= 0xfe00 && codepoint <= 0xfe0f) ||
         (codepoint >= 0xe0100 && codepoint <= 0xe01ef);
#else
  return uc_is_property_variation_selector(wchar);
#endif
}

static inline bool
ncunicode_islinebreak(wchar_t wchar){
#ifdef USE_UTF8PROC
  switch(utf8proc_category((utf8proc_int32_t)wchar)){
    case UTF8PROC_CATEGORY_ZL:
    case UTF8PROC_CATEGORY_ZP:
      return true;
    default:
      return false;
  }
#else
  const uint32_t mask = UC_CATEGORY_MASK_Zl | UC_CATEGORY_MASK_Zp;
  return uc_is_general_category_withtable(wchar, mask);
#endif
}

static inline bool
ncunicode_iswordbreak(wchar_t wchar){
#ifdef USE_UTF8PROC
  switch(utf8proc_category((utf8proc_int32_t)wchar)){
    case UTF8PROC_CATEGORY_ZS:
    case UTF8PROC_CATEGORY_ZL:
    case UTF8PROC_CATEGORY_ZP:
      return true;
    default:
      return false;
  }
#else
  const uint32_t mask = UC_CATEGORY_MASK_Z |
                        UC_CATEGORY_MASK_Zs;
  return uc_is_general_category_withtable(wchar, mask);
#endif
}

int ncunicode_ucs32_to_utf8(const uint32_t* ucs32, unsigned ucs32count,
                            unsigned char* resultbuf, size_t buflen);

#ifdef __cplusplus
}
#endif

#endif
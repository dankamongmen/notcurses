#include <fenv.h>
#include <string.h>
#include <locale.h>
#include <pthread.h>
#include "notcurses/notcurses.h"

const char *ncmetric(uintmax_t val, uintmax_t decimal, char *buf, int omitdec,
                     uintmax_t mult, int uprefix){
  // these two must have the same number of elements
  const wchar_t prefixes[] =    L"KMGTPEZY"; // 10^21-1 encompasses 2^64-1
  const wchar_t subprefixes[] = L"mµnpfazy"; // 10^24-1
  unsigned consumed = 0;
  uintmax_t dv;

  fesetround(FE_TONEAREST);
  if(decimal == 0 || mult == 0){
    return NULL;
  }
  if(decimal > UINTMAX_MAX / 10){
    return NULL;
  }
  dv = mult;
  if(decimal <= val || val == 0){
    // FIXME verify that input < 2^89, wish we had static_assert() :/
    while((val / decimal) >= dv && consumed < sizeof(prefixes) / sizeof(*prefixes)){
      dv *= mult;
      ++consumed;
      if(UINTMAX_MAX / dv < mult){ // near overflow--can't scale dv again
        break;
      }
    }
  }else{
    while(val < decimal && consumed < sizeof(subprefixes) / sizeof(*subprefixes)){
      val *= mult;
      ++consumed;
      if(UINTMAX_MAX / dv < mult){ // near overflow--can't scale dv again
        break;
      }
    }
  }
  int sprintfed;
  if(dv != mult){ // if consumed == 0, dv must equal mult
    if(val / dv > 0){
      ++consumed;
    }else{
      dv /= mult;
    }
    val /= decimal;
    // Remainder is val % dv, but we want a percentage as scaled integer.
    // Ideally we would multiply by 100 and divide the result by dv, for
    // maximum accuracy (dv might not be a multiple of 10--it is not for
    // 1,024). That can overflow with large 64-bit values, but we can first
    // divide both sides by mult, and then scale by 100.
    if(omitdec && (val % dv) == 0){
      sprintfed = sprintf(buf, "%ju%lc", val / dv, prefixes[consumed - 1]);
    }else{
      sprintfed = sprintf(buf, "%.2f%lc", (double)val / dv, prefixes[consumed - 1]);
    }
    if(uprefix){
      buf[sprintfed] = uprefix;
      buf[sprintfed + 1] = '\0';
    }
    return buf;
  }
  // unscaled output, consumed == 0, dv == mult
  // val / decimal < dv (or we ran out of prefixes)
  if(omitdec && val % decimal == 0){
    if(consumed){
      sprintfed = sprintf(buf, "%ju%lc", val / decimal, subprefixes[consumed - 1]);
    }else{
      sprintf(buf, "%ju", val / decimal);
    }
  }else{
    if(consumed){
      sprintfed = sprintf(buf, "%.2f%lc", (double)val / decimal, subprefixes[consumed - 1]);
    }else{
      sprintf(buf, "%.2f", (double)val / decimal);
    }
  }
  if(consumed && uprefix){
    buf[sprintfed] = uprefix;
    buf[sprintfed + 1] = '\0';
  }
  return buf;
}

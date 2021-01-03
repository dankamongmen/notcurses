#include <wchar.h>

// wcwidth() might be missing some lengths (for instance, glibc didn't get
// Unicode 13 support until 2.31). ncwidth() handles some characters on the
// wcwidth() error path. it ought generally be used rather than wcwidth().
static inline int
ncwidth(wchar_t c){
  int r = wcwidth(c);
  if(r >= 0){
    return r;
  }
  // Symbols for Legacy Computing were only added to glibc in 2.32 (along with
  // the rest of Unicode 13). Handle them explicitly if wcwidth() failed.
  if(c >= 0x1fb00 && c <= 0x1fb3b){
    return 1;
  }
  return -1;
}

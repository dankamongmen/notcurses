#include "demo.h"

int mojibake_demo(struct notcurses* nc){
  if(!notcurses_canutf8(nc)){
    return 0;
  }
  return 0;
}

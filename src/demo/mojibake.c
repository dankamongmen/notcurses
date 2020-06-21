#include "demo.h"

int mojibake_demo(struct notcurses* nc){
  if(!notcurses_canutf8(nc)){
    return 0;
  }
  int dimy, dimx;
  struct ncplane* std = notcurses_stddim_yx(nc, &dimy, &dimx);
  ncplane_set_fg(std, 0xffffff);
  if(ncplane_putstr_aligned(std, 4, NCALIGN_CENTER, "mojibake 文字化けmodʑibake") < 0){
    return -1;
  }
  DEMO_RENDER(nc);
  demo_nanosleep(nc, &demodelay);
  return 0;
}

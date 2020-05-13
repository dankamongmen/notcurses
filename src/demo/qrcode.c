#include "demo.h"
#ifdef __linux__
#include <sys/random.h>
#else
#include <sys/libkern.h>
#endif

int qrcode_demo(struct notcurses* nc){
#ifdef USE_QRCODEGEN
  char data[128];
  int dimy, dimx;
  struct ncplane* n = notcurses_stddim_yx(nc, &dimy, &dimx);
  for(int i = 0 ; i < 1024 ; ++i){
    ncplane_erase(n);
    size_t len = random() % sizeof(data) + 1;
#ifdef __linux__
    ssize_t got = getrandom(data, len, 0);
    if(got < 0 || (size_t)got != len){
      return -1;
    }
#else
    arc4rand(data, len, 0);
#endif
    if(ncplane_cursor_move_yx(n, 0, 0)){
      return -1;
    }
    if(ncplane_qrcode(n, 0, data, len) <= 0){
      return -1;
    }
    if(ncplane_cursor_move_yx(n, 0, 0)){
      return -1;
    }
    uint64_t tl = 0, bl = 0, br = 0, tr = 0;
    channels_set_fg_rgb(&tl, random() % 255 + 1, random() % 255 + 1, random() % 255 + 1);
    channels_set_fg_rgb(&tr, random() % 255 + 1, random() % 255 + 1, random() % 255 + 1);
    channels_set_fg_rgb(&bl, random() % 255 + 1, random() % 255 + 1, random() % 255 + 1);
    channels_set_fg_rgb(&br, random() % 255 + 1, random() % 255 + 1, random() % 255 + 1);
    if(ncplane_stain(n, dimy - 1, dimx - 1, tl, tr, bl, br) <= 0){
      return -1;
    }
    DEMO_RENDER(nc);
  }
#endif
  ncplane_erase(notcurses_stdplane(nc));
  DEMO_RENDER(nc);
  return 0;
}

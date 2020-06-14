#include "demo.h"
#include <sys/random.h>

int qrcode_demo(struct notcurses* nc){
  if(!notcurses_canutf8(nc)){
    return 0;
  }
#ifdef USE_QRCODEGEN
  char data[128];
  int dimy, dimx;
  struct ncplane *stdn = notcurses_stddim_yx(nc, &dimy, &dimx);
  ncplane_erase(stdn);
  struct ncplane* n = ncplane_dup(stdn, NULL);
  for(int i = 0 ; i < 1024 ; ++i){
    ncplane_erase(n);
    size_t len = random() % sizeof(data) + 1;
    size_t done = 0;
    // done this tedious way because getrandom() doesn't exist on freebsd 11
    while(done < len){
      long r = random();
      memcpy(data + done, &r, sizeof(r));
      done += sizeof(r);
    }
    ncplane_home(n);
    int y = dimy, x = dimx;
    ncplane_home(n);
    int qlen = ncplane_qrcode(n, NCBLIT_1x1, &y, &x, data, len);
    if(qlen > 0){ // FIXME can fail due to being too large for display; distinguish this
      ncplane_move_yx(n, (dimy - y) / 2, (dimx - x) / 2);
      ncplane_home(n);
      uint64_t tl = 0, bl = 0, br = 0, tr = 0;
      channels_set_fg_rgb(&tl, random() % 255 + 1, random() % 255 + 1, random() % 255 + 1);
      channels_set_fg_rgb(&tr, random() % 255 + 1, random() % 255 + 1, random() % 255 + 1);
      channels_set_fg_rgb(&bl, random() % 255 + 1, random() % 255 + 1, random() % 255 + 1);
      channels_set_fg_rgb(&br, random() % 255 + 1, random() % 255 + 1, random() % 255 + 1);
      if(ncplane_stain(n, y, x, tl, tr, bl, br) <= 0){
        ncplane_destroy(n);
        return -1;
      }
      DEMO_RENDER(nc);
    }
  }
  ncplane_destroy(n);
#endif
  DEMO_RENDER(nc);
  return 0;
}

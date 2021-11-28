#include "demo.h"

int qrcode_demo(struct notcurses* nc, uint64_t startns){
  (void)startns;
  if(!notcurses_canutf8(nc)){
    return 0;
  }
#ifdef USE_QRCODEGEN
  char data[128];
  unsigned dimy, dimx;
  struct ncplane *stdn = notcurses_stddim_yx(nc, &dimy, &dimx);
  ncplane_erase(stdn);
  struct ncplane* n = ncplane_dup(stdn, NULL);
  for(int i = 0 ; i < 1024 ; ++i){
    ncplane_erase(n);
    size_t len = rand() % sizeof(data) + 1;
    size_t done = 0;
    // done this tedious way because getrand() doesn't exist on freebsd 11
    while(done < len){
      long r = rand();
      memcpy(data + done, &r, sizeof(r));
      done += sizeof(r);
    }
    ncplane_home(n);
    unsigned y = dimy, x = dimx;
    ncplane_home(n);
    int qlen = ncplane_qrcode(n, &y, &x, data, len);
    if(qlen > 0){ // FIXME can fail due to being too large for display; distinguish this case
      ncplane_move_yx(n, (dimy - y) / 2, (dimx - x) / 2);
      ncplane_home(n);
      ncplane_set_fg_rgb8(n, rand() % 255 + 1, rand() % 255 + 1, rand() % 255 + 1); 
      DEMO_RENDER(nc);
    }
  }
  ncplane_mergedown_simple(n, stdn); // leave the last one on-screen
  ncplane_destroy(n);
#endif
  return 0;
}

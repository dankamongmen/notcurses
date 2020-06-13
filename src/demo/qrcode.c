#include "demo.h"

#ifdef USE_QRCODEGEN
#include <sys/random.h>
// FIXME duplicated--ought these just be exported?
#define QR_BASE_SIZE 17
#define PER_QR_VERSION 4
static inline int
qrcode_rows(int version){
  return (QR_BASE_SIZE + (version * PER_QR_VERSION)) / 2;
}

static inline int
qrcode_cols(int version){
  return QR_BASE_SIZE + (version * PER_QR_VERSION);
}
#endif

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
    int qlen = ncplane_qrcode(n, NCBLIT_DEFAULT, &y, &x, data, len);
    if(qlen > 0){ // can fail due to being too large for display
      ncplane_move_yx(n, dimy / 2 - qrcode_rows(qlen) / 2,
                      dimx / 2 - qrcode_cols(qlen) / 2);
      ncplane_home(n);
      uint64_t tl = 0, bl = 0, br = 0, tr = 0;
      channels_set_fg_rgb(&tl, random() % 255 + 1, random() % 255 + 1, random() % 255 + 1);
      channels_set_fg_rgb(&tr, random() % 255 + 1, random() % 255 + 1, random() % 255 + 1);
      channels_set_fg_rgb(&bl, random() % 255 + 1, random() % 255 + 1, random() % 255 + 1);
      channels_set_fg_rgb(&br, random() % 255 + 1, random() % 255 + 1, random() % 255 + 1);
      if(ncplane_stain(n, qrcode_rows(qlen), qrcode_cols(qlen), tl, tr, bl, br) <= 0){
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

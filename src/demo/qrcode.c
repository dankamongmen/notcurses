#include "demo.h"

#ifdef USE_QRCODEGEN
#include <sys/random.h>
// FIXME duplicated--ought these just be exported?
#define QR_BASE_SIZE 17
#define PER_QR_VERSION 4
static inline int
qrcode_rows(int version){
  return QR_BASE_SIZE + (version * PER_QR_VERSION / 2);
}

static inline int
qrcode_cols(int version){
  return QR_BASE_SIZE + (version * PER_QR_VERSION);
}
#endif

int qrcode_demo(struct notcurses* nc){
#ifdef USE_QRCODEGEN
  char data[128];
  int dimy, dimx;
  struct ncplane *stdn = notcurses_stddim_yx(nc, &dimy, &dimx);
  ncplane_erase(stdn);
  struct ncplane* n = ncplane_dup(stdn, NULL);
  for(int i = 0 ; i < 1024 ; ++i){
    ncplane_erase(n);
    size_t len = random() % sizeof(data) + 1;
    ssize_t got = getrandom(data, len, 0);
    if(got < 0 || (size_t)got != len){
      ncplane_destroy(n);
      return -1;
    }
    if(ncplane_cursor_move_yx(n, 0, 0)){
      ncplane_destroy(n);
      return -1;
    }
    int qlen = ncplane_qrcode(n, 0, data, len);
    if(qlen <= 0){
      ncplane_destroy(n);
      return -1;
    }
    ncplane_move_yx(n, dimy / 2 - qrcode_rows(qlen) / 2,
                    dimx / 2 - qrcode_cols(qlen) / 2);
    if(ncplane_cursor_move_yx(n, 0, 0)){
      ncplane_destroy(n);
      return -1;
    }
    uint64_t tl = 0, bl = 0, br = 0, tr = 0;
    channels_set_fg_rgb(&tl, random() % 255 + 1, random() % 255 + 1, random() % 255 + 1);
    channels_set_fg_rgb(&tr, random() % 255 + 1, random() % 255 + 1, random() % 255 + 1);
    channels_set_fg_rgb(&bl, random() % 255 + 1, random() % 255 + 1, random() % 255 + 1);
    channels_set_fg_rgb(&br, random() % 255 + 1, random() % 255 + 1, random() % 255 + 1);
    if(ncplane_stain(n, dimy - 1, dimx - 1, tl, tr, bl, br) <= 0){
      ncplane_destroy(n);
      return -1;
    }
    DEMO_RENDER(nc);
  }
  ncplane_destroy(n);
#endif
  DEMO_RENDER(nc);
  return 0;
}

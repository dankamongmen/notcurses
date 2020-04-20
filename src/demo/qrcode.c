#include "demo.h"
#include <sys/random.h>

int qrcode_demo(struct notcurses* nc){
#ifdef USE_QRCODEGEN
  char data[128];
  struct ncplane* n = notcurses_stdplane(nc);
  for(int i = 0 ; i < 1024 ; ++i){
    ncplane_erase(n);
    size_t len = random() % sizeof(data) + 1;
    ssize_t got = getrandom(data, len, 0);
    if(got < 0 || (size_t)got != len){
      return -1;
    }
    if(ncplane_cursor_move_yx(n, 0, 0)){
      return -1;
    }
    if(ncplane_qrcode(n, 0, data, len) <= 0){
      return -1;
    }
    DEMO_RENDER(nc);
  }
#else
  DEMO_RENDER(nc);
#endif
  return 0;
}

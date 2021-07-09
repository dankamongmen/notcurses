#include <stdlib.h>
#include <locale.h>
#include <notcurses/notcurses.h>

int main(void){
  setlocale(LC_ALL, "");
  notcurses_options nopts = {
    .flags = NCOPTION_INHIBIT_SETLOCALE
             | NCOPTION_NO_ALTERNATE_SCREEN
             | NCOPTION_PRESERVE_CURSOR,
  };
  struct notcurses* nc = notcurses_core_init(&nopts, NULL);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  int dimy, dimx;
  struct ncplane* n = notcurses_stddim_yx(nc, &dimy, &dimx);
  wchar_t wc = 0x4e00;
  ncplane_set_scrolling(n, true);
  ncplane_set_styles(n, NCSTYLE_BOLD);
  ncplane_putstr(n, "This program is *not* indicative of real scrolling speed.\n");
  ncplane_set_styles(n, NCSTYLE_NONE);
  while(true){
    struct timespec req = { .tv_sec = 0, .tv_nsec = 1000000, };
    nanosleep(&req, NULL);
    if(ncplane_putwc(n, wc) <= 0){
      break;
    }
    if(++wc == 0x9fa5){
      wc = 0x4e00;
    }
    if(notcurses_render(nc)){
      break;
    }
  }
  notcurses_stop(nc);
  return EXIT_FAILURE;
}

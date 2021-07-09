#include <stdlib.h>
#include <locale.h>
#include <notcurses/notcurses.h>

int main(void){
  setlocale(LC_ALL, "");
  struct notcurses_options opts = {
    .flags = NCOPTION_INHIBIT_SETLOCALE | NCOPTION_NO_ALTERNATE_SCREEN
              | NCOPTION_PRESERVE_CURSOR,
  };
  struct notcurses* nc = notcurses_core_init(&opts, NULL);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  int dimy, dimx;
  struct ncplane* n = notcurses_stddim_yx(nc, &dimy, &dimx);
  char c = 'A';
  ncplane_set_scrolling(n, true);
  ncplane_set_styles(n, NCSTYLE_BOLD);
  ncplane_putstr(n, "This program is *not* indicative of real scrolling speed.\n");
  ncplane_set_styles(n, NCSTYLE_NONE);
  while(true){
    struct timespec req = { .tv_sec = 0, .tv_nsec = 1000000, };
    nanosleep(&req, NULL);
    if(ncplane_putchar(n, c) != 1){
      break;
    }
    if(++c == '{'){
      c = 'A';
    }
    if(notcurses_render(nc)){
      break;
    }
  }
  notcurses_stop(nc);
  return EXIT_SUCCESS;
}

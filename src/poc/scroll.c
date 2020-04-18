#include <stdlib.h>
#include <locale.h>
#include <notcurses/notcurses.h>

int main(void){
  setlocale(LC_ALL, "");
  struct notcurses* nc = notcurses_init(NULL, stdout);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  int dimy, dimx, y, x;
  struct ncplane* n = notcurses_stddim_yx(nc, &dimy, &dimx);
  char c = 'A';
  ncplane_set_scrolling(n, true);
  while(true){
    struct timespec req = { .tv_sec = 0, .tv_nsec = 1000000, };
    nanosleep(&req, NULL);
    if(ncplane_putsimple(n, c) != 1){
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

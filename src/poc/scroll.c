#include <stdlib.h>
#include <locale.h>
#include <notcurses/notcurses.h>

int main(void){
  setlocale(LC_ALL, "");
  struct notcurses_options opts = {
    .flags = NCOPTION_INHIBIT_SETLOCALE
              | NCOPTION_CLI_MODE
              | NCOPTION_DRAIN_INPUT,
  };
  struct notcurses* nc = notcurses_core_init(&opts, NULL);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  unsigned dimy, dimx;
  struct ncplane* n = notcurses_stddim_yx(nc, &dimy, &dimx);
  char c = 'A';
  ncplane_set_styles(n, NCSTYLE_BOLD);
  ncplane_putstr(n, "This program is *not* indicative of real scrolling speed.\n");
  ncplane_set_styles(n, NCSTYLE_NONE);
  unsigned y = ncplane_cursor_y(n);
  while(true){
    struct timespec req = { .tv_sec = 0, .tv_nsec = 1000000, };
    nanosleep(&req, NULL);
    if(ncplane_putchar(n, c) != 1){
      break;
    }
    if(++c == '{'){
      c = 'A';
    }
    unsigned newy = ncplane_cursor_y(n);
    if(newy != y){
      y = newy;
      notcurses_render(nc);
    }
  }
  if(notcurses_render(nc)){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  notcurses_stop(nc);
  return EXIT_SUCCESS;
}

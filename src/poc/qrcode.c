#include <unistd.h>
#include <locale.h>
#include <notcurses/notcurses.h>

static int
render_qrcode(struct ncplane* n, int dimy, int dimx, const char* text){
  unsigned y = dimy, x = dimx;
  ncplane_home(n);
  int ver = ncplane_qrcode(n, &y, &x, text, strlen(text));
  if(ver < 0){
    return -1;
  }
  if(ncplane_putstr_yx(n, y + 1, 0, text) < 0){
    return -1;
  }
  if(notcurses_render(ncplane_notcurses(n))){
    return -1;
  }
  sleep(2);
  return 0;
}

int main(int argc, const char** argv){
  if(setlocale(LC_ALL, "") == NULL){
    return EXIT_FAILURE;
  }
  struct notcurses_options opts = {
    .flags = NCOPTION_INHIBIT_SETLOCALE
             | NCOPTION_NO_ALTERNATE_SCREEN
             | NCOPTION_DRAIN_INPUT,
  };
  struct notcurses* nc = notcurses_init(&opts, NULL);
  unsigned dimy, dimx;
  struct ncplane* std = notcurses_stddim_yx(nc, &dimy, &dimx);
  while(*++argv){
    if(render_qrcode(std, dimy, dimx, *argv)){
      notcurses_stop(nc);
      return EXIT_FAILURE;
    }
  }
  if(argc < 2){
    if(render_qrcode(std, dimy, dimx, "https://nick-black.com")){
      notcurses_stop(nc);
      return EXIT_FAILURE;
    }
  }
  notcurses_stop(nc);
  return EXIT_SUCCESS;
}

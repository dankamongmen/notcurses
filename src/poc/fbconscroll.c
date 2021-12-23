#include <stdio.h>
#include <notcurses/notcurses.h>

int main(int argc, char** argv){
  int rows = 1;
  if(argc > 2){
    fprintf(stderr, "usage: fbconscroll [rows]\n");
    return EXIT_FAILURE;
  }else if(argc == 2){
    rows = atoi(argv[1]);
    if(rows < 1){
      fprintf(stderr, "usage: fbconscroll [rows]\n");
      return EXIT_FAILURE;
    }
  }
  struct notcurses_options nopts = {
    .flags = NCOPTION_CLI_MODE
             | NCOPTION_SUPPRESS_BANNERS
             | NCOPTION_NO_FONT_CHANGES
             | NCOPTION_DRAIN_INPUT,
  };
  struct notcurses* nc = notcurses_init(&nopts, NULL);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  unsigned dimy, dimx;
  struct ncplane* n = notcurses_stddim_yx(nc, &dimy, &dimx);
  for(int i = 0 ; i < rows ; ++i){
    int r;
    if((r = ncplane_putchar(n, '\n')) != 0){
      notcurses_stop(nc);
      fprintf(stderr, "RET: %d\n", r);
      return EXIT_FAILURE;
    }
  }
  notcurses_render(nc);
  notcurses_stop(nc);
  return EXIT_SUCCESS;
}

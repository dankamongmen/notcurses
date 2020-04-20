#include <notcurses/notcurses.h>

int main(int argc, char** argv){
  notcurses_options opts = {};
  opts.inhibit_alternate_screen = true;
  struct notcurses* nc = notcurses_init(&opts, NULL);
  int ret = -1;
  while(*++argv){
  }

done:
  if(notcurses_stop(nc) || ret){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

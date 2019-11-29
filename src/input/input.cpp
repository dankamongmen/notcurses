#include <cstdlib>
#include <notcurses.h>

int main(void){
  notcurses_options opts{};
  opts.outfp = stdout;
  struct notcurses* nc = notcurses_init(&opts);
  if(nc == NULL){
    return EXIT_FAILURE;;
  }
  // FIXME read input
  if(notcurses_stop(nc)){
    return EXIT_FAILURE;;
  }
  return EXIT_SUCCESS;
}

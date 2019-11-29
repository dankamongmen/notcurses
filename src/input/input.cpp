#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <clocale>
#include <iostream>
#include <notcurses.h>

int main(void){
  if(setlocale(LC_ALL, "") == nullptr){
    return EXIT_FAILURE;
  }
  notcurses_options opts{};
  opts.outfp = stdout;
  struct notcurses* nc = notcurses_init(&opts);
  struct ncplane* n = notcurses_stdplane(nc);
  if(nc == nullptr){
    return EXIT_FAILURE;;
  }
  int r;
  cell c = CELL_TRIVIAL_INITIALIZER;
  if(ncplane_fg_rgb8(n, 255, 255, 255)){
    return EXIT_FAILURE;
  }
  while((r = notcurses_getc_blocking(nc, &c)) >= 0){
    if(r == 0){ // interrupted by signal
      continue;
    }
    if(ncplane_printf(n, "Got keypress: [%04x] '%c'\n", c.gcluster, c.gcluster) < 0){
      break;
    }
    if(notcurses_render(nc)){
      break;
    }
  }
  int e = errno;
  notcurses_stop(nc);
  if(r < 0){
    std::cerr << "Error reading from terminal (" << strerror(e) << "?)\n";
  }
  return EXIT_FAILURE;
}

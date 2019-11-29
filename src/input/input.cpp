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
  if(nc == nullptr){
    return EXIT_FAILURE;;
  }
  struct ncplane* n = notcurses_stdplane(nc);
  int r;
  cell c = CELL_TRIVIAL_INITIALIZER;
  if(ncplane_fg_rgb8(n, 255, 255, 255)){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  while(errno = 0, (r = notcurses_getc_blocking(nc, &c)) >= 0){
    if(r == 0){ // interrupted by signal
      continue;
    }
    if(ncplane_cursor_move_yx(n, 0, 0)){
      break;
    }
    if(cell_simple_p(&c)){
      char kp = c.gcluster;
      if(ncplane_printf(n, "Got keypress: [0x%04x (%04d)] '%c'\n", kp, kp, kp) < 0){
        break;
      }
    }else{
      ncplane_printf(n, "Curious! %d\n", c.gcluster); // FIXME
    }
    if(notcurses_render(nc)){
      break;
    }
  }
  int e = errno;
  notcurses_stop(nc);
  if(r < 0 && e){
    std::cerr << "Error reading from terminal (" << strerror(e) << "?)\n";
  }
  return EXIT_FAILURE;
}

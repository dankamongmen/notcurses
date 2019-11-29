#include <deque>
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
  int r, dimy, dimx;
  notcurses_term_dim_yx(nc, &dimy, &dimx);
  cell c = CELL_TRIVIAL_INITIALIZER;
  if(ncplane_fg_rgb8(n, 255, 255, 255)){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  int y = 1;
  std::deque<cell> cells;
  ncspecial_key special;
  while(errno = 0, (r = notcurses_getc_blocking(nc, &c, &special)) >= 0){
    if(r == 0){ // interrupted by signal
      continue;
    }
    if(ncplane_cursor_move_yx(n, y, 0)){
      break;
    }
    if(cell_simple_p(&c)){
      char kp = c.gcluster;
      if(ncplane_printf(n, "Got keypress: [0x%04x (%04d)] '%c'\n", kp, kp, kp) < 0){
        break;
      }
    }else{
      ncplane_printf(n, "Curious! %d%5s\n", c.gcluster, ""); // FIXME
    }
    // FIXME reprint all lines, fading older ones
    if(notcurses_render(nc)){
      break;
    }
    if(++y >= dimy - 2){ // leave a blank line at the bottom
      y = 1;             // and at the top
    }
    while(cells.size() >= dimy - 3u){
      cells.pop_back();
    }
    cells.push_front(c);
  }
  int e = errno;
  notcurses_stop(nc);
  if(r < 0 && e){
    std::cerr << "Error reading from terminal (" << strerror(e) << "?)\n";
  }
  return EXIT_FAILURE;
}

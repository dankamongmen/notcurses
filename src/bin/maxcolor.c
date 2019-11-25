#include <unistd.h>
#include "demo.h"

// draws a border along the perimeter, then fills the inside with position
// markers, each a slightly different color. the goal is to make sure we can
// have a great many colors, that they progress reasonably through the space,
// and that we can write to every coordinate.
int maxcolor_demo(struct notcurses* nc){
  int maxx, maxy;
  notcurses_term_dimyx(nc, &maxy, &maxx);
  struct ncplane* n = notcurses_stdplane(nc);
  cell ul = CELL_TRIVIAL_INITIALIZER;
  cell ll = CELL_TRIVIAL_INITIALIZER;
  cell lr = CELL_TRIVIAL_INITIALIZER;
  cell ur = CELL_TRIVIAL_INITIALIZER;
  cell hl = CELL_TRIVIAL_INITIALIZER;
  cell vl = CELL_TRIVIAL_INITIALIZER;
  cell_load(n, &ul, "╭");
  cell_load(n, &ur, "╮");
  cell_load(n, &ll, "╰");
  cell_load(n, &lr, "╯");
  cell_load(n, &vl, "│");
  cell_load(n, &hl, "─");
  ncplane_cursor_move_yx(n, 0, 0);
  if(ncplane_box(n, &ul, &ur, &ll, &lr, &hl, &vl, maxy, maxx)){
    return -1;
  }
  // FIXME fill all chars
  if(notcurses_render(nc)){
    return -1;
  }
  sleep(1);
  cell_release(n, &ul);
  cell_release(n, &ur);
  cell_release(n, &ll);
  cell_release(n, &lr);
  cell_release(n, &vl);
  cell_release(n, &hl);
  return 0;
}

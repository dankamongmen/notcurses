#include <unistd.h>
#include "demo.h"

// red across, blue down, green from UL to LR
int grid_demo(struct notcurses* nc){
  int maxx, maxy;
  notcurses_term_dimyx(nc, &maxy, &maxx);
  int rs = 256 / maxx;
  int gs = 256 / (maxx + maxy);
  int bs = 256 / maxy;
  int y, x;
  struct ncplane* n = notcurses_stdplane(nc);
  cell ul, ll, cl, cr, lc, lr, ur, uc, cc;
  cell_init(&ul);
  cell_init(&uc);
  cell_init(&cl);
  cell_init(&cr);
  cell_init(&ll);
  cell_init(&lc);
  cell_init(&lr);
  cell_init(&ur);
  cell_init(&cc);
  cell_load(n, &ul, "┍");
  cell_load(n, &uc, "┯");
  cell_load(n, &ur, "┑");
  cell_load(n, &cl, "┝");
  cell_load(n, &cc, "┿");
  cell_load(n, &cr, "┥");
  cell_load(n, &ll, "┕");
  cell_load(n, &lc, "┷");
  cell_load(n, &lr, "┙");
  ncplane_cursor_move_yx(n, 0, 0);
  y = 0;

  // top line
  cell_set_fg(&ul, 255, 255, 255);
  ncplane_putc(n, &ul);
  for(x = 1 ; x < maxx - 1 ; ++x){
    cell_set_fg(&uc, 255 - rs * x, 255 - gs * (x + y), 255);
    ncplane_putc(n, &uc);
  }
  cell_set_fg(&ur, 1, 255 - gs * (x + y), 255);
  ncplane_putc(n, &ur);

  // center
  for(y = 1 ; y < maxy - 1 ; ++y){
    cell_set_fg(&cl, 255, 255 - gs * (x + y), 255 - bs * y);
    ncplane_putc(n, &cl);
    for(x = 1 ; x < maxx - 1 ; ++x){
      cell_set_fg(&cc, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
      ncplane_putc(n, &cc);
    }
    cell_set_fg(&cr, 1, 255 - gs * (x + y), 255 - bs * y);
    ncplane_putc(n, &cr);
  }

  // bottom line
  cell_set_fg(&ll, 255, 255 - gs * (x + y), 1);
  ncplane_putc(n, &ll);
  for(x = 1 ; x < maxx - 1 ; ++x){
    cell_set_fg(&lc, 255 - rs * x, 255 - gs * (x + y), 1);
    ncplane_putc(n, &lc);
  }
  cell_set_fg(&lr, 1, 255 - gs * (x + y), 1);
  ncplane_putc(n, &lr);

  // render!
  notcurses_render(nc);
  sleep(1);
  return 0;
}

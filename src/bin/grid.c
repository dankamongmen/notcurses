#include <unistd.h>
#include "demo.h"

static int
gridinv_demo(struct notcurses* nc, struct ncplane *n,
             cell* ul, cell* uc, cell* ur,
             cell* cl, cell* cc, cell* cr,
             cell* ll, cell* lc, cell* lr){
  int maxx, maxy;
  notcurses_term_dimyx(nc, &maxy, &maxx);
  int rs = 256 / maxx;
  int gs = 256 / (maxx + maxy);
  int bs = 256 / maxy;
  int x = 0;
  int y = 0;
  if(ncplane_cursor_move_yx(n, y, x)){
    return -1;
  }
  // top line
  cell_set_fg(ul, 0, 0, 0);
  cell_set_bg(ul, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
  ncplane_putc(n, ul);
  for(x = 1 ; x < maxx - 1 ; ++x){
    cell_set_fg(uc, 0, 0, 0);
    cell_set_bg(uc, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
    ncplane_putc(n, uc);
  }
  cell_set_fg(ur, 0, 0, 0);
  cell_set_bg(ur, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
  ncplane_putc(n, ur);

  // center
  x = 0;
  for(y = 1 ; y < maxy - 1 ; ++y){
    cell_set_fg(cl, 0, 0, 0);
    cell_set_bg(cl, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
    ncplane_putc(n, cl);
    for(x = 1 ; x < maxx - 1 ; ++x){
      cell_set_fg(cc, 0, 0, 0);
      cell_set_bg(cc, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
      ncplane_putc(n, cc);
    }
    cell_set_fg(cr, 0, 0, 0);
    cell_set_bg(cr, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
    ncplane_putc(n, cr);
  }

  // bottom line
  x = 0;
  cell_set_fg(ll, 0, 0, 0);
  cell_set_bg(ll, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
  ncplane_putc(n, ll);
  for(x = 1 ; x < maxx - 1 ; ++x){
    cell_set_fg(lc, 0, 0, 0);
    cell_set_bg(lc, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
    ncplane_putc(n, lc);
  }
  cell_set_fg(lr, 0, 0, 0);
  cell_set_bg(lr, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
  ncplane_putc(n, lr);

  // render!
  notcurses_render(nc);
  sleep(1);
  return 0;
}

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
  x = 0;
  cell_set_fg(&ul, 255 - rs * x, 255, 255);
  ncplane_putc(n, &ul);
  for(x = 1 ; x < maxx - 1 ; ++x){
    cell_set_fg(&uc, 255 - rs * x, 255 - gs * (x + y), 255);
    ncplane_putc(n, &uc);
  }
  cell_set_fg(&ur, 255 - rs * x, 255 - gs * (x + y), 255);
  ncplane_putc(n, &ur);

  // center
  x = 0;
  for(y = 1 ; y < maxy - 1 ; ++y){
    cell_set_fg(&cl, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
    ncplane_putc(n, &cl);
    for(x = 1 ; x < maxx - 1 ; ++x){
      cell_set_fg(&cc, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
      ncplane_putc(n, &cc);
    }
    cell_set_fg(&cr, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
    ncplane_putc(n, &cr);
  }

  // bottom line
  x = 0;
  cell_set_fg(&ll, 255 - rs * x, 255 - gs * (x + y), 1);
  ncplane_putc(n, &ll);
  for(x = 1 ; x < maxx - 1 ; ++x){
    cell_set_fg(&lc, 255 - rs * x, 255 - gs * (x + y), 1);
    ncplane_putc(n, &lc);
  }
  cell_set_fg(&lr, 255 - rs * x, 255 - gs * (x + y), 1);
  ncplane_putc(n, &lr);

  // render!
  notcurses_render(nc);
  sleep(1);
  return gridinv_demo(nc, n, &ul, &uc, &ur, &cl, &cc, &cr, &ll, &lc, &lr);
}

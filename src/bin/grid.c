#include <unistd.h>
#include "demo.h"

static void
release_cells(struct ncplane* n,
              cell* ul, cell* uc, cell* ur,
              cell* cl, cell* cc, cell* cr,
              cell* ll, cell* lc, cell* lr){
  cell_release(n, ul);
  cell_release(n, uc);
  cell_release(n, ur);
  cell_release(n, cl);
  cell_release(n, cc);
  cell_release(n, cr);
  cell_release(n, ll);
  cell_release(n, lc);
  cell_release(n, lr);
}

static int
prep_cells(struct ncplane* n,
           cell* ul, cell* uc, cell* ur,
           cell* cl, cell* cc, cell* cr,
           cell* ll, cell* lc, cell* lr){
  cell_init(ul);
  cell_init(uc);
  cell_init(cl);
  cell_init(cr);
  cell_init(ll);
  cell_init(lc);
  cell_init(lr);
  cell_init(ur);
  cell_init(cc);
  int ret = 0;
  ret |= cell_load(n, ul, "┍");
  ret |= cell_load(n, uc, "┯");
  ret |= cell_load(n, ur, "┑");
  ret |= cell_load(n, cl, "┝");
  ret |= cell_load(n, cc, "┿");
  ret |= cell_load(n, cr, "┥");
  ret |= cell_load(n, ll, "┕");
  ret |= cell_load(n, lc, "┷");
  ret |= cell_load(n, lr, "┙");
  return ret;
}

static int
gridswitch_demo(struct notcurses* nc, struct ncplane *n){
  ncplane_erase(n);
  int maxx, maxy;
  notcurses_term_dimyx(nc, &maxy, &maxx);
  int rs = 256 / maxx;
  int gs = 256 / (maxx + maxy);
  int bs = 256 / maxy;
  int x = 0;
  int y = 0;
  cell ul, ll, cl, cr, lc, lr, ur, uc, cc;
  prep_cells(n, &ul, &uc, &ur, &cl, &cc, &cr, &ll, &lc, &lr);
  if(ncplane_cursor_move_yx(n, y, x)){
    return -1;
  }
  // top line
  cell_set_fg(&ul, 255 - rs * y, 255 - gs * (x + y), 255 - bs * x);
  cell_set_bg(&ul, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
  ncplane_putc(n, &ul);
  for(x = 1 ; x < maxx - 1 ; ++x){
    cell_set_fg(&uc, 255 - rs * y, 255 - gs * (x + y), 255 - bs * x);
    cell_set_bg(&uc, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
    ncplane_putc(n, &uc);
  }
  cell_set_fg(&ur, 255 - rs * y, 255 - gs * (x + y), 255 - bs * x);
  cell_set_bg(&ur, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
  ncplane_putc(n, &ur);

  // center
  for(y = 1 ; y < maxy - 1 ; ++y){
    x = 0;
    cell_set_fg(&cl, 255 - rs * y, 255 - gs * (x + y), 255 - bs * x);
    cell_set_bg(&cl, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
    ncplane_putc(n, &cl);
    for(x = 1 ; x < maxx - 1 ; ++x){
      cell_set_fg(&cc, 255 - rs * y, 255 - gs * (x + y), 255 - bs * x);
      cell_set_bg(&cc, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
      ncplane_putc(n, &cc);
    }
    cell_set_fg(&cr, 255 - rs * y, 255 - gs * (x + y), 255 - bs * x);
    cell_set_bg(&cr, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
    ncplane_putc(n, &cr);
  }

  // bottom line
  x = 0;
  cell_set_fg(&ll, 255 - rs * y, 255 - gs * (x + y), 255 - bs * x);
  cell_set_bg(&ll, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
  ncplane_putc(n, &ll);
  for(x = 1 ; x < maxx - 1 ; ++x){
    cell_set_fg(&lc, 255 - rs * y, 255 - gs * (x + y), 255 - bs * x);
    cell_set_bg(&lc, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
    ncplane_putc(n, &lc);
  }
  cell_set_fg(&lr, 255 - rs * y, 255 - gs * (x + y), 255 - bs * x);
  cell_set_bg(&lr, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
  ncplane_putc(n, &lr);

  // render!
  notcurses_render(nc);
  sleep(1);
  release_cells(n, &ul, &uc, &ur, &cl, &cc, &cr, &ll, &lc, &lr);
  return 0;
}

static int
gridinv_demo(struct notcurses* nc, struct ncplane *n){
  ncplane_erase(n);
  int maxx, maxy;
  notcurses_term_dimyx(nc, &maxy, &maxx);
  int rs = 256 / maxx;
  int gs = 256 / (maxx + maxy);
  int bs = 256 / maxy;
  int x = 0;
  int y = 0;
  cell ul, ll, cl, cr, lc, lr, ur, uc, cc;
  prep_cells(n, &ul, &uc, &ur, &cl, &cc, &cr, &ll, &lc, &lr);
  if(ncplane_cursor_move_yx(n, y, x)){
    return -1;
  }
  // top line
  cell_set_fg(&ul, 0, 0, 0);
  cell_set_bg(&ul, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
  ncplane_putc(n, &ul);
  for(x = 1 ; x < maxx - 1 ; ++x){
    cell_set_fg(&uc, 0, 0, 0);
    cell_set_bg(&uc, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
    ncplane_putc(n, &uc);
  }
  cell_set_fg(&ur, 0, 0, 0);
  cell_set_bg(&ur, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
  ncplane_putc(n, &ur);

  // center
  for(y = 1 ; y < maxy - 1 ; ++y){
    x = 0;
    cell_set_fg(&cl, 0, 0, 0);
    cell_set_bg(&cl, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
    ncplane_putc(n, &cl);
    for(x = 1 ; x < maxx - 1 ; ++x){
      cell_set_fg(&cc, 0, 0, 0);
      cell_set_bg(&cc, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
      ncplane_putc(n, &cc);
    }
    cell_set_fg(&cr, 0, 0, 0);
    cell_set_bg(&cr, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
    ncplane_putc(n, &cr);
  }

  // bottom line
  x = 0;
  cell_set_fg(&ll, 0, 0, 0);
  cell_set_bg(&ll, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
  ncplane_putc(n, &ll);
  for(x = 1 ; x < maxx - 1 ; ++x){
    cell_set_fg(&lc, 0, 0, 0);
    cell_set_bg(&lc, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
    ncplane_putc(n, &lc);
  }
  cell_set_fg(&lr, 0, 0, 0);
  cell_set_bg(&lr, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
  ncplane_putc(n, &lr);

  // render!
  notcurses_render(nc);
  sleep(1);
  release_cells(n, &ul, &uc, &ur, &cl, &cc, &cr, &ll, &lc, &lr);
  return gridswitch_demo(nc, n);
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
  ncplane_erase(n);
  cell ul, ll, cl, cr, lc, lr, ur, uc, cc;
  prep_cells(n, &ul, &uc, &ur, &cl, &cc, &cr, &ll, &lc, &lr);
  ncplane_cursor_move_yx(n, 0, 0);
  y = 0;

  // top line
  x = 0;
  cell_set_bg(&ul, y, y, y);
  cell_set_bg(&uc, y, y, y);
  cell_set_bg(&ur, y, y, y);
  cell_set_fg(&ul, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
  ncplane_putc(n, &ul);
  for(x = 1 ; x < maxx - 1 ; ++x){
    cell_set_fg(&uc, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
    ncplane_putc(n, &uc);
  }
  cell_set_fg(&ur, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
  ncplane_putc(n, &ur);

  // center
  for(y = 1 ; y < maxy - 1 ; ++y){
    x = 0;
    cell_set_bg(&cl, y, y, y);
    cell_set_bg(&cc, y, y, y);
    cell_set_bg(&cr, y, y, y);
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
  cell_set_bg(&ll, y, y, y);
  cell_set_bg(&lc, y, y, y);
  cell_set_bg(&lr, y, y, y);
  cell_set_fg(&ll, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
  ncplane_putc(n, &ll);
  for(x = 1 ; x < maxx - 1 ; ++x){
    cell_set_fg(&lc, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
    ncplane_putc(n, &lc);
  }
  cell_set_fg(&lr, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
  ncplane_putc(n, &lr);

  // render!
  notcurses_render(nc);
  sleep(1);
  release_cells(n, &ul, &uc, &ur, &cl, &cc, &cr, &ll, &lc, &lr);
  return gridinv_demo(nc, n);
}

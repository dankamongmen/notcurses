#include <unistd.h>
#include "demo.h"

// clip and set
static int
ccell_set_fg_rgb(cell* c, int r, int g, int b){
  if(r < 0) r = 0;
  if(g < 0) g = 0;
  if(b < 0) b = 0;
  return cell_set_fg_rgb(c, r, g, b);
}

static int
ccell_set_bg_rgb(cell* c, int r, int g, int b){
  if(r < 0) r = 0;
  if(g < 0) g = 0;
  if(b < 0) b = 0;
  return cell_set_bg_rgb(c, r, g, b);
}

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
prep_cells2(struct ncplane* n,
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
  ret |= cell_load(n, ul, "╔");
  ret |= cell_load(n, uc, "╦");
  ret |= cell_load(n, ur, "╗");
  ret |= cell_load(n, cl, "╠");
  ret |= cell_load(n, cc, "╬");
  ret |= cell_load(n, cr, "╣");
  ret |= cell_load(n, ll, "╚");
  ret |= cell_load(n, lc, "╩");
  ret |= cell_load(n, lr, "╝");
  return ret;
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
  notcurses_term_dim_yx(nc, &maxy, &maxx);
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
  int ret = 0;
  ret |= ccell_set_fg_rgb(&ul, 255 - rs * y, 255 - gs * (x + y), 255 - bs * x);
  ret |= ccell_set_bg_rgb(&ul, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
  ncplane_putc(n, &ul);
  for(x = 1 ; x < maxx - 1 ; ++x){
    ret |= ccell_set_fg_rgb(&uc, 255 - rs * y, 255 - gs * (x + y), 255 - bs * x);
    ret |= ccell_set_bg_rgb(&uc, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
    ncplane_putc(n, &uc);
  }
  ret |= ccell_set_fg_rgb(&ur, 255 - rs * y, 255 - gs * (x + y), 255 - bs * x);
  ret |= ccell_set_bg_rgb(&ur, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
  ncplane_putc(n, &ur);

  // center
  for(y = 1 ; y < maxy - 1 ; ++y){
    x = 0;
    if(ncplane_cursor_move_yx(n, y, x)){
      return -1;
    }
    ret |= ccell_set_fg_rgb(&cl, 255 - rs * y, 255 - gs * (x + y), 255 - bs * x);
    ret |= ccell_set_bg_rgb(&cl, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
    ncplane_putc(n, &cl);
    for(x = 1 ; x < maxx - 1 ; ++x){
      ret |= ccell_set_fg_rgb(&cc, 255 - rs * y, 255 - gs * (x + y), 255 - bs * x);
      ret |= ccell_set_bg_rgb(&cc, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
      ncplane_putc(n, &cc);
    }
    ret |= ccell_set_fg_rgb(&cr, 255 - rs * y, 255 - gs * (x + y), 255 - bs * x);
    ret |= ccell_set_bg_rgb(&cr, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
    ncplane_putc(n, &cr);
  }

  // bottom line
  x = 0;
  if(ncplane_cursor_move_yx(n, y, x)){
    return -1;
  }
  ret |= ccell_set_fg_rgb(&ll, 255 - rs * y, 255 - gs * (x + y), 255 - bs * x);
  ret |= ccell_set_bg_rgb(&ll, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
  ncplane_putc(n, &ll);
  for(x = 1 ; x < maxx - 1 ; ++x){
    ret |= ccell_set_fg_rgb(&lc, 255 - rs * y, 255 - gs * (x + y), 255 - bs * x);
    ret |= ccell_set_bg_rgb(&lc, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
    ncplane_putc(n, &lc);
  }
  ret |= ccell_set_fg_rgb(&lr, 255 - rs * y, 255 - gs * (x + y), 255 - bs * x);
  ret |= ccell_set_bg_rgb(&lr, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
  ncplane_putc(n, &lr);

  // render!
  notcurses_render(nc);
  release_cells(n, &ul, &uc, &ur, &cl, &cc, &cr, &ll, &lc, &lr);
  nanosleep(&demodelay, NULL);
  return ret;
}

static int
gridinv_demo(struct notcurses* nc, struct ncplane *n){
  ncplane_erase(n);
  int maxx, maxy;
  notcurses_term_dim_yx(nc, &maxy, &maxx);
  int rs = 256 / maxx;
  int gs = 256 / (maxx + maxy);
  int bs = 256 / maxy;
  int x = 0;
  int y = 0;
  cell ul, ll, cl, cr, lc, lr, ur, uc, cc;
  prep_cells2(n, &ul, &uc, &ur, &cl, &cc, &cr, &ll, &lc, &lr);
  if(ncplane_cursor_move_yx(n, y, x)){
    return -1;
  }
  // top line
  ccell_set_fg_rgb(&ul, 0, 0, 0);
  ccell_set_bg_rgb(&ul, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
  ncplane_putc(n, &ul);
  for(x = 1 ; x < maxx - 1 ; ++x){
    ccell_set_fg_rgb(&uc, 0, 0, 0);
    ccell_set_bg_rgb(&uc, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
    ncplane_putc(n, &uc);
  }
  ccell_set_fg_rgb(&ur, 0, 0, 0);
  ccell_set_bg_rgb(&ur, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
  ncplane_putc(n, &ur);

  // center
  for(y = 1 ; y < maxy - 1 ; ++y){
    x = 0;
    if(ncplane_cursor_move_yx(n, y, x)){
      return -1;
    }
    ccell_set_fg_rgb(&cl, 0, 0, 0);
    ccell_set_bg_rgb(&cl, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
    ncplane_putc(n, &cl);
    for(x = 1 ; x < maxx - 1 ; ++x){
      ccell_set_fg_rgb(&cc, 0, 0, 0);
      ccell_set_bg_rgb(&cc, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
      ncplane_putc(n, &cc);
    }
    ccell_set_fg_rgb(&cr, 0, 0, 0);
    ccell_set_bg_rgb(&cr, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
    ncplane_putc(n, &cr);
  }

  // bottom line
  x = 0;
  if(ncplane_cursor_move_yx(n, y, x)){
    return -1;
  }
  ccell_set_fg_rgb(&ll, 0, 0, 0);
  ccell_set_bg_rgb(&ll, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
  ncplane_putc(n, &ll);
  for(x = 1 ; x < maxx - 1 ; ++x){
    ccell_set_fg_rgb(&lc, 0, 0, 0);
    ccell_set_bg_rgb(&lc, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
    ncplane_putc(n, &lc);
  }
  ccell_set_fg_rgb(&lr, 0, 0, 0);
  ccell_set_bg_rgb(&lr, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
  ncplane_putc(n, &lr);

  // render!
  notcurses_render(nc);
  release_cells(n, &ul, &uc, &ur, &cl, &cc, &cr, &ll, &lc, &lr);
  nanosleep(&demodelay, NULL);
  return gridswitch_demo(nc, n);
}

// red across, blue down, green from UL to LR
int grid_demo(struct notcurses* nc){
  int maxx, maxy;
  notcurses_term_dim_yx(nc, &maxy, &maxx);
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
  ccell_set_bg_rgb(&ul, y, y, y);
  ccell_set_bg_rgb(&uc, y, y, y);
  ccell_set_bg_rgb(&ur, y, y, y);
  ccell_set_fg_rgb(&ul, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
  ncplane_putc(n, &ul);
  for(x = 1 ; x < maxx - 1 ; ++x){
    ccell_set_fg_rgb(&uc, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
    ncplane_putc(n, &uc);
  }
  ccell_set_fg_rgb(&ur, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
  ncplane_putc(n, &ur);

  // center
  for(y = 1 ; y < maxy - 1 ; ++y){
    x = 0;
    if(ncplane_cursor_move_yx(n, y, x)){
      return -1;
    }
    ccell_set_bg_rgb(&cl, y, y, y);
    ccell_set_bg_rgb(&cc, y, y, y);
    ccell_set_bg_rgb(&cr, y, y, y);
    ccell_set_fg_rgb(&cl, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
    ncplane_putc(n, &cl);
    for(x = 1 ; x < maxx - 1 ; ++x){
      ccell_set_fg_rgb(&cc, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
      ncplane_putc(n, &cc);
    }
    ccell_set_fg_rgb(&cr, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
    ncplane_putc(n, &cr);
  }

  // bottom line
  x = 0;
  if(ncplane_cursor_move_yx(n, y, x)){
    return -1;
  }
  ccell_set_bg_rgb(&ll, y, y, y);
  ccell_set_bg_rgb(&lc, y, y, y);
  ccell_set_bg_rgb(&lr, y, y, y);
  ccell_set_fg_rgb(&ll, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
  ncplane_putc(n, &ll);
  for(x = 1 ; x < maxx - 1 ; ++x){
    ccell_set_fg_rgb(&lc, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
    ncplane_putc(n, &lc);
  }
  ccell_set_fg_rgb(&lr, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
  ncplane_putc(n, &lr);

  // render!
  notcurses_render(nc);
  release_cells(n, &ul, &uc, &ur, &cl, &cc, &cr, &ll, &lc, &lr);
  nanosleep(&demodelay, NULL);
  return gridinv_demo(nc, n);
}

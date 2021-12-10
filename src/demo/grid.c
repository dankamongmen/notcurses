#include <unistd.h>
#include "demo.h"

// clip and set
static int
cnccell_set_fg_rgb8(nccell* c, int r, int g, int b){
  if(r < 0) r = 0;
  if(g < 0) g = 0;
  if(b < 0) b = 0;
  return nccell_set_fg_rgb8(c, r, g, b);
}

static int
cnccell_set_bg_rgb8(nccell* c, int r, int g, int b){
  if(r < 0) r = 0;
  if(g < 0) g = 0;
  if(b < 0) b = 0;
  return nccell_set_bg_rgb8(c, r, g, b);
}

static void
release_cells(struct ncplane* n,
              nccell* ul, nccell* uc, nccell* ur,
              nccell* cl, nccell* cc, nccell* cr,
              nccell* ll, nccell* lc, nccell* lr){
  nccell_release(n, ul);
  nccell_release(n, uc);
  nccell_release(n, ur);
  nccell_release(n, cl);
  nccell_release(n, cc);
  nccell_release(n, cr);
  nccell_release(n, ll);
  nccell_release(n, lc);
  nccell_release(n, lr);
}

static int
prep_cells2(struct ncplane* n,
           nccell* ul, nccell* uc, nccell* ur,
           nccell* cl, nccell* cc, nccell* cr,
           nccell* ll, nccell* lc, nccell* lr){
  nccell_init(ul);
  nccell_init(uc);
  nccell_init(cl);
  nccell_init(cr);
  nccell_init(ll);
  nccell_init(lc);
  nccell_init(lr);
  nccell_init(ur);
  nccell_init(cc);
  int ret = 0;
  ret |= nccell_load(n, ul, "╔");
  ret |= nccell_load(n, uc, "╦");
  ret |= nccell_load(n, ur, "╗");
  ret |= nccell_load(n, cl, "╠");
  ret |= nccell_load(n, cc, "╬");
  ret |= nccell_load(n, cr, "╣");
  ret |= nccell_load(n, ll, "╚");
  ret |= nccell_load(n, lc, "╩");
  ret |= nccell_load(n, lr, "╝");
  return ret;
}

static int
prep_cells(struct ncplane* n,
           nccell* ul, nccell* uc, nccell* ur,
           nccell* cl, nccell* cc, nccell* cr,
           nccell* ll, nccell* lc, nccell* lr){
  nccell_init(ul);
  nccell_init(uc);
  nccell_init(cl);
  nccell_init(cr);
  nccell_init(ll);
  nccell_init(lc);
  nccell_init(lr);
  nccell_init(ur);
  nccell_init(cc);
  int ret = 0;
  ret |= nccell_load(n, ul, "┍");
  ret |= nccell_load(n, uc, "┯");
  ret |= nccell_load(n, ur, "┑");
  ret |= nccell_load(n, cl, "┝");
  ret |= nccell_load(n, cc, "┿");
  ret |= nccell_load(n, cr, "┥");
  ret |= nccell_load(n, ll, "┕");
  ret |= nccell_load(n, lc, "┷");
  ret |= nccell_load(n, lr, "┙");
  return ret;
}

static int
bgnext(nccell* c, int* r, int* g, int* b){
  int ret = cnccell_set_bg_rgb8(c, *r, *g, *b);
  if(*g % 2){
    if(--*b <= 0){
      if(++*g >= 256){
        *g = 255;
      }
      *b = 0;
    }
  }else{
    if(++*b >= 256){
      if(++*g >= 256){
        *g = 255;
      }
      *b = 255;
    }
  }
  return ret;
}

static int
gridinv_demo(struct notcurses* nc, struct ncplane *n){
  ncplane_erase(n);
  nccell ul, ll, cl, cr, lc, lr, ur, uc, cc;
  prep_cells2(n, &ul, &uc, &ur, &cl, &cc, &cr, &ll, &lc, &lr);
  for(int i = 0 ; i < 256 ; ++i){
    unsigned maxx, maxy;
    notcurses_term_dim_yx(nc, &maxy, &maxx);
    int rs = 255 / maxx;
    int gs = 255 / (maxx + maxy);
    int bs = 255 / maxy;
    unsigned x = 0;
    unsigned y = 1;
    // top line
    cnccell_set_fg_rgb8(&ul, i / 2, i, i / 2);
    cnccell_set_bg_rgb8(&ul, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
    ncplane_putc_yx(n, y, x, &ul);
    for(x = 1 ; x < maxx - 1 ; ++x){
      cnccell_set_fg_rgb8(&uc, i / 2, i, i / 2);
      cnccell_set_bg_rgb8(&uc, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
      ncplane_putc(n, &uc);
    }
    cnccell_set_fg_rgb8(&ur, i / 2, i, i / 2);
    cnccell_set_bg_rgb8(&ur, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
    ncplane_putc(n, &ur);

    // center
    for(++y ; y < maxy - 1 ; ++y){
      x = 0;
      cnccell_set_fg_rgb8(&cl, i / 2, i, i / 2);
      cnccell_set_bg_rgb8(&cl, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
      ncplane_putc_yx(n, y, x, &cl);
      for(x = 1 ; x < maxx - 1 ; ++x){
        cnccell_set_fg_rgb8(&cc, i / 2, i, i / 2);
        cnccell_set_bg_rgb8(&cc, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
        ncplane_putc(n, &cc);
      }
      cnccell_set_fg_rgb8(&cr, i / 2, i, i / 2);
      cnccell_set_bg_rgb8(&cr, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
      ncplane_putc(n, &cr);
    }

    // bottom line
    x = 0;
    cnccell_set_fg_rgb8(&ll, i / 2, i, i / 2);
    cnccell_set_bg_rgb8(&ll, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
    ncplane_putc_yx(n, y, x, &ll);
    for(x = 1 ; x < maxx - 1 ; ++x){
      cnccell_set_fg_rgb8(&lc, i / 2, i, i / 2);
      cnccell_set_bg_rgb8(&lc, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
      ncplane_putc(n, &lc);
    }
    cnccell_set_fg_rgb8(&lr, i / 2, i, i / 2);
    cnccell_set_bg_rgb8(&lr, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
    ncplane_putc(n, &lr);

    DEMO_RENDER(nc);
  }
  release_cells(n, &ul, &uc, &ur, &cl, &cc, &cr, &ll, &lc, &lr);
  return 0;
}

static int
gridswitch_demo(struct notcurses* nc, struct ncplane *n){
  ncplane_erase(n);
  int ret = 0;
  unsigned maxx, maxy;
  nccell ul, ll, cl, cr, lc, lr, ur, uc, cc;
  prep_cells(n, &ul, &uc, &ur, &cl, &cc, &cr, &ll, &lc, &lr);
  for(int i = 0 ; i < 256 ; ++i){
    notcurses_term_dim_yx(nc, &maxy, &maxx);
    int rs = 256 / maxx;
    int gs = 256 / (maxx + maxy);
    int bs = 256 / maxy;
    unsigned x = 0;
    unsigned y = 1;
    int bgr = i;
    int bgg = 0x80;
    int bgb = i;
    // top line
    ret |= cnccell_set_fg_rgb8(&ul, 255 - rs * y, 255 - gs * (x + y), 255 - bs * y);
    ret |= bgnext(&ul, &bgr, &bgg, &bgb);
    if(ncplane_putc_yx(n, y, x, &ul) <= 0){
      return -1;
    }
    for(x = 1 ; x < maxx - 1 ; ++x){
      ret |= cnccell_set_fg_rgb8(&uc, 255 - rs * x, 255 - gs * (x + y), 255 - bs * x);
      ret |= bgnext(&uc, &bgr, &bgg, &bgb);
      if(ncplane_putc(n, &uc) <= 0){
        return -1;
      }
    }
    ret |= cnccell_set_fg_rgb8(&ur, 255 - rs * x, 255 - gs * (x + y), 255 - bs * x);
    ret |= bgnext(&ur, &bgr, &bgg, &bgb);
    if(ncplane_putc(n, &ur) < 0){
      return -1;
    }

    // center
    for(++y ; y < maxy - 1 ; ++y){
      x = 0;
      ret |= cnccell_set_fg_rgb8(&cl, 255 - rs * x, 255 - gs * (x + y), 255 - bs * x);
      ret |= bgnext(&cl, &bgr, &bgg, &bgb);
      ncplane_putc_yx(n, y, x, &cl);
      for(x = 1 ; x < maxx - 1 ; ++x){
        ret |= cnccell_set_fg_rgb8(&cc, 255 - rs * x, 255 - gs * (x + y), 255 - bs * x);
        ret |= bgnext(&cc, &bgr, &bgg, &bgb);
        ncplane_putc(n, &cc);
      }
      ret |= cnccell_set_fg_rgb8(&cr, 255 - rs * x, 255 - gs * (x + y), 255 - bs * x);
      ret |= bgnext(&cr, &bgr, &bgg, &bgb);
      ncplane_putc(n, &cr);
    }

    // bottom line
    x = 0;
    ret |= cnccell_set_fg_rgb8(&ll, 255 - rs * x, 255 - gs * (x + y), 255 - bs * x);
    ret |= bgnext(&ll, &bgr, &bgg, &bgb);
    ncplane_putc_yx(n, y, x, &ll);
    for(x = 1 ; x < maxx - 1 ; ++x){
      ret |= cnccell_set_fg_rgb8(&lc, 255 - rs * x, 255 - gs * (x + y), 255 - bs * x);
      ret |= bgnext(&lc, &bgr, &bgg, &bgb);
      ncplane_putc(n, &lc);
    }
    ret |= cnccell_set_fg_rgb8(&lr, 255 - rs * x, 255 - gs * (x + y), 255 - bs * x);
    ret |= bgnext(&lr, &bgr, &bgg, &bgb);
    ncplane_putc(n, &lr);

    // render!
    DEMO_RENDER(nc);
  }
  release_cells(n, &ul, &uc, &ur, &cl, &cc, &cr, &ll, &lc, &lr);
  if(ret){
    return ret;
  }
  return gridinv_demo(nc, n);
}

// red across, blue down, green from UL to LR
int grid_demo(struct notcurses* nc, uint64_t startns){
  (void)startns;
  unsigned y, x;
  struct ncplane* n = notcurses_stdplane(nc);
  ncplane_erase(n);
  nccell ul, uc, ur;
  nccell ll, lc, lr;
  nccell cl, cc, cr;
  prep_cells(n, &ul, &uc, &ur, &cl, &cc, &cr, &ll, &lc, &lr);

  int ret = 0;
  for(int i = 0 ; i < 256 ; ++i){
    unsigned maxx, maxy;
    notcurses_term_dim_yx(nc, &maxy, &maxx);
    int rs = 255 / maxx;
    int gs = 255 / (maxx + maxy);
    int bs = 255 / maxy;
    // top line
    x = 0;
    y = 1;
    ret |= cnccell_set_bg_rgb8(&ul, i, x * rs, y * bs);
    ret |= cnccell_set_fg_rgb8(&ul, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
    if(ncplane_putc_yx(n, y, 0, &ul) <= 0){
      return -1;
    }
    for(x = 1 ; x < maxx - 1 ; ++x){
      ret |= cnccell_set_bg_rgb8(&uc, i, x * rs, y * bs);
      ret |= cnccell_set_fg_rgb8(&uc, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
      if(ncplane_putc(n, &uc) <= 0){
        return -1;
      }
    }
    ret |= cnccell_set_bg_rgb8(&ur, i, x * rs, y * bs);
    ret |= cnccell_set_fg_rgb8(&ur, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
    if(ncplane_putc(n, &ur) <= 0){
      return -1;
    }

    // center
    for(++y ; y < maxy - 1 ; ++y){
      x = 0;
      ret |= cnccell_set_bg_rgb8(&cl, i, x * rs, y * bs);
      ret |= cnccell_set_fg_rgb8(&cl, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
      if(ncplane_putc_yx(n, y, x, &cl) <= 0){
        return -1;
      }
      for(x = 1 ; x < maxx - 1 ; ++x){
        ret |= cnccell_set_bg_rgb8(&cc, i, x * rs, y * bs);
        ret |= cnccell_set_fg_rgb8(&cc, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
        if(ncplane_putc(n, &cc) <= 0){
          return -1;
        }
      }
      ret |= cnccell_set_bg_rgb8(&cr, i, x * rs, y * bs);
      ret |= cnccell_set_fg_rgb8(&cr, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
      if(ncplane_putc(n, &cr) <= 0){
        return -1;
      }
    }

    // bottom line
    x = 0;
    ret |= cnccell_set_bg_rgb8(&ll, i, x * rs, y * bs);
    ret |= cnccell_set_fg_rgb8(&ll, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
    if(ncplane_putc_yx(n, y, x, &ll) <= 0){
      return -1;
    }
    for(x = 1 ; x < maxx - 1 ; ++x){
      ret |= cnccell_set_bg_rgb8(&lc, i, x * rs, y * bs);
      ret |= cnccell_set_fg_rgb8(&lc, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
      if(ncplane_putc(n, &lc) <= 0){
        return -1;
      }
    }
    ret |= cnccell_set_bg_rgb8(&lr, i, x * rs, y * bs);
    ret |= cnccell_set_fg_rgb8(&lr, 255 - rs * x, 255 - gs * (x + y), 255 - bs * y);
    if(ncplane_putc(n, &lr) <= 0){
      return -1;
    }
    if(ret){
      return -1;
    }
    DEMO_RENDER(nc);
  }
  release_cells(n, &ul, &uc, &ur, &cl, &cc, &cr, &ll, &lc, &lr);
  return gridswitch_demo(nc, n);
}

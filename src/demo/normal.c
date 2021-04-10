#include "demo.h"
#include <math.h>
#include <complex.h>

static int
rotate_plane(struct notcurses* nc, struct ncplane* n){
  struct timespec scaled;
  timespec_div(&demodelay, 2, &scaled);
  // we can't rotate a plane unless it has an even number of columns :/
  int nx;
  if((nx = ncplane_dim_x(n)) % 2){
    if(ncplane_resize_simple(n, ncplane_dim_y(n), --nx)){
      return -1;
    }
  }
  for(int i = 0 ; i < 16 ; ++i){
    demo_nanosleep(nc, &scaled);
    int centy, centx;
    ncplane_center_abs(n, &centy, &centx);
    if(ncplane_rotate_cw(n)){
      return -1;
    }
    int cent2y, cent2x;
    int absy, absx;
    ncplane_center_abs(n, &cent2y, &cent2x);
    ncplane_yx(n, &absy, &absx);
    ncplane_move_yx(n, absy + centy - cent2y, absx + centx - cent2x);
    DEMO_RENDER(nc);
    timespec_mul(&scaled, 2, &scaled);
    timespec_div(&scaled, 3, &scaled);
  }
  timespec_div(&demodelay, 2, &scaled);
  for(int i = 0 ; i < 16 ; ++i){
    demo_nanosleep(nc, &scaled);
    int centy, centx;
    ncplane_center_abs(n, &centy, &centx);
    if(ncplane_rotate_ccw(n)){
      return -1;
    }
    int cent2y, cent2x;
    int absy, absx;
    ncplane_center_abs(n, &cent2y, &cent2x);
    ncplane_yx(n, &absy, &absx);
    ncplane_move_yx(n, absy + centy - cent2y, absx + centx - cent2x);
    DEMO_RENDER(nc);
    timespec_mul(&scaled, 2, &scaled);
    timespec_div(&scaled, 3, &scaled);
  }
  return 0;
}

static int
rotate_visual(struct notcurses* nc, struct ncplane* n, int dy, int dx){
  struct timespec scaled;
  int r = 0;
  timespec_div(&demodelay, 8, &scaled);
  int fromy = 0, fromx = 0;
  if(dy * 2 > dx){
    fromy = (dy * 2 - dx) / 2;
    dy = dx / 2;
  }else{
    fromx = (dx - dy * 2) / 2;
    dx = dy * 2;
  }
//fprintf(stderr, "ASK %d/%d @ %d/%d: %p\n", dy, dx, fromy, fromx);
  struct ncvisual* ncv = ncvisual_from_plane(n, NCBLIT_DEFAULT, fromy, fromx, dy, dx);
//fprintf(stderr, "%d/%d @ %d/%d: %p\n", dy, dx, fromy, fromx, ncv);
  if(!ncv){
    ncvisual_destroy(ncv);
    return -1;
  }
  ncplane_destroy(n);
  int dimy, dimx;
  struct ncplane* stdn = notcurses_stddim_yx(nc, &dimy, &dimx);
  const int ROTATIONS = 32;
  timespec_div(&demodelay, ROTATIONS / 2, &scaled);
  struct ncvisual_options vopts = { };
  ncplane_erase(stdn);
  struct ncvisual* nncv = NULL;
  struct ncplane* np = NULL;
  if(notcurses_canopen_images(nc)){
    char* path = find_data("normal.png");
    if(path){
      nncv = ncvisual_from_file(path);
      if(nncv){
        struct ncplane_options nopts = {
          .y = 1,
          .rows = dimy - 1,
          .cols = dimx,
        };
        np = ncplane_create(stdn, &nopts);
        if(np == NULL){
          return -1;
        }
        struct ncvisual_options nvopts = {
          .n = np,
          .scaling = NCSCALE_STRETCH,
        };
        if(ncvisual_render(nc, nncv, &nvopts) == NULL){
          return -1;
        }
        ncplane_move_below(notcurses_stdplane(nc), np);
      }
      free(path);
    }
  }
  for(double i = 0 ; i < ROTATIONS ; ++i){
    demo_nanosleep(nc, &scaled);
    if(ncvisual_rotate(ncv, -M_PI / (i / 8 + 2))){
      r = -1;
      break;
    }
    int vy, vx, vyscale, vxscale;
    ncvisual_blitter_geom(nc, ncv, &vopts, &vy, &vx, &vyscale, &vxscale, NULL);
    vopts.x = NCALIGN_CENTER;
    vopts.y = (dimy - (vy / vyscale)) / 2;
    vopts.flags |= NCVISUAL_OPTION_HORALIGNED;
    struct ncplane* newn;
    if((newn = ncvisual_render(nc, ncv, &vopts)) == NULL){
      r = -1;
      break;
    }
    if( (r = demo_render(nc)) ){
      break;
    }
    ncplane_destroy(newn);
  }
  ncplane_destroy(np);
  ncvisual_destroy(nncv);
  ncvisual_destroy(ncv);
  return r;
}

static const int ITERMAX = 256;

static int
mandelbrot(int y, int x, int dy, int dx){
  float complex c = (x - dx / 2.0) * 4.0 / dx + I * (y - dy / 2.0) * 4.0 / dx;
  float fx = 0;
  float fy = 0;
  int iter = 0;
  while(fx * fx + fy * fy <= 4 && iter < ITERMAX){
    float ffx = fx * fx - fy * fy + crealf(c);
    fy = 2 * fx * fy + cimagf(c);
    fx = ffx;
    ++iter;
  }
  return iter;
}

// write an rgba pixel
static int
mcell(uint32_t* c, int y, int x, int dy, int dx){
  int iter = mandelbrot(y, x, dy, dx);
  int color = sqrt((double)iter / ITERMAX) * 255;
  *c = ncpixel(color, color, color);
  return 0;
}

static uint32_t*
offset(uint32_t* rgba, int y, int x, int dx){
  return rgba + y * dx + x;
}

// make a pixel array out from the center, blitting it as we go
int normal_demo(struct notcurses* nc){
  int dy, dx;
  int r = -1;
  struct ncplane* nstd = notcurses_stddim_yx(nc, &dy, &dx);
  ncplane_erase(nstd);
  nccell c = CELL_TRIVIAL_INITIALIZER;
  cell_set_fg_rgb8(&c, 0x0, 0x0, 0x0);
  cell_set_bg_rgb8(&c, 0x0, 0x0, 0x0);
  ncplane_set_base_cell(nstd, &c);
  nccell_release(nstd, &c);
  struct ncplane* n = NULL;
  struct ncvisual_options vopts = {
    .n = nstd,
  };
  int yscale, xscale;
  ncvisual_blitter_geom(nc, NULL, &vopts, NULL, NULL, &yscale, &xscale, NULL);
  dy *= yscale;
  dx *= xscale;
  uint32_t* rgba = malloc(sizeof(*rgba) * dy * dx);
  if(!rgba){
    goto err;
  }
  for(int off = 0 ; off < dy * dx ; ++off){
    rgba[off] = 0xff000000;
  }
  int y;
  if(dy / yscale % 2){
    y = dy / yscale + 1;
    for(int x = 0 ; x < dx ; ++x){
      if(mcell(offset(rgba, y, x, dx), y, x, dy / yscale, dx)){
        goto err;
      }
    }
  }
  struct timespec scaled;
  timespec_div(&demodelay, dy, &scaled);
  for(y = 0 ; y <= dy / 2 ; ++y){
    for(int x = 0 ; x < dx ; ++x){
      if(mcell(offset(rgba, dy / 2 - y, x, dx), dy / 2 - y, x, dy, dx)){
        goto err;
      }
      if(mcell(offset(rgba, dy / 2 + y - 1, x, dx), dy / 2 + y - 1, x, dy, dx)){
        goto err;
      }
    }
    vopts.leny = dy;
    vopts.lenx = dx;
    if(ncblit_rgba(rgba, dx * sizeof(*rgba), &vopts) < 0){
      goto err;
    }
    if( (r = demo_render(nc)) ){
      goto err;
    }
    r = -1;
    demo_nanosleep(nc, &scaled);
  }
  free(rgba);
  rgba = NULL;
  // we can't resize (and thus can't rotate) the standard plane, so dup it
  n = ncplane_dup(nstd, NULL);
  if(n == NULL){
    goto err;
  }
  if(notcurses_canutf8(nc)){
    ncplane_erase(nstd);
    ncplane_home(n);
    if( (r = rotate_plane(nc, n)) ){
      goto err;
    }
  }
  ncplane_home(n);
  uint64_t tl, tr, bl, br;
  tl = tr = bl = br = 0;
  channels_set_fg_rgb8(&tl, 0, 0, 0);
  channels_set_fg_rgb8(&tr, 0, 0xff, 0);
  channels_set_fg_rgb8(&bl, 0xff, 0, 0xff);
  channels_set_fg_rgb8(&br, 0, 0, 0);
  ncplane_dim_yx(n, &dy, &dx);
  if(ncplane_stain(n, dy - 1, dx - 1, tl, tr, bl, br) < 0){
    goto err;
  }
  DEMO_RENDER(nc);
  timespec_div(&demodelay, 2, &scaled);
  demo_nanosleep(nc, &scaled);
  cell_set_fg_rgb8(&c, 0, 0, 0);
  cell_set_bg_rgb8(&c, 0, 0, 0);
  ncplane_set_base_cell(nstd, &c);
  nccell_release(nstd, &c);
  r = rotate_visual(nc, n, dy, dx);
  return r;

err:
  free(rgba);
  ncplane_destroy(n);
  return r;
}

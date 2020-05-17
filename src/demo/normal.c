#include "demo.h"
#include <math.h>
#include <complex.h>

static const int VSCALE = 2;
static const int ITERMAX = 255;

static int
mandelbrot(int y, int x, int dy, int dx){
  float complex c = (x - dx/2.0) * 4.0/dx + I * (y - dy/2.0) * 4.0/dx;
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
  *c = (0xff << 24u) + ((255 - iter) << 16u) + ((255 - iter) << 8u) + (255 - iter);
  return 0;
}

static uint32_t*
offset(uint32_t* rgba, int y, int x, int dx){
  return rgba + y * dx + x;
}

// make a pixel array out from the center, blitting it as we go
int normal_demo(struct notcurses* nc){
  int dy, dx;
  struct ncplane* nstd = notcurses_stddim_yx(nc, &dy, &dx);
  ncplane_erase(nstd);
  struct ncplane* n = NULL;
  dy *= VSCALE; // double-block trick means both 2x resolution and even linecount yay
  uint32_t* rgba = malloc(sizeof(*rgba) * dy * dx);
  if(!rgba){
    goto err;
  }
  for(int off = 0 ; off < dy * dx ; ++off){
    rgba[off] = 0xff000000;
  }
  int y;
  if(dy / VSCALE % 2){
    y = dy / VSCALE + 1;
    for(int x = 0 ; x < dx ; ++x){
      if(mcell(offset(rgba, y, x, dx), y, x, dy / VSCALE, dx)){
        goto err;
      }
    }
  }
  struct timespec scaled;
  timespec_div(&demodelay, dy, &scaled);
  for(y = 0 ; y < dy / 2 ; ++y){
    for(int x = 0 ; x < dx ; ++x){
      if(mcell(offset(rgba, dy / 2 - y, x, dx), dy / 2 - y, x, dy, dx)){
        goto err;
      }
      if(mcell(offset(rgba, dy / 2 + y - 1, x, dx), dy / 2 + y - 1, x, dy, dx)){
        goto err;
      }
    }
    if(ncblit_rgba(nstd, 0, 0, dx * sizeof(*rgba), rgba, 0, 0, dy, dx) < 0){
      goto err;
    }
    DEMO_RENDER(nc);
    demo_nanosleep(nc, &scaled);
  }
  free(rgba);
  rgba = NULL;
  timespec_div(&demodelay, 8, &scaled);
  // we can't resize (and thus can't rotate) the standard plane, so dup it
  n = ncplane_dup(nstd, NULL);
  if(n == NULL){
    return -1;
  }
  for(int i = 0 ; i < 16 ; ++i){
    demo_nanosleep(nc, &scaled);
    if(ncplane_rotate_cw(n)){
      goto err;
    }
    DEMO_RENDER(nc);
  }
  for(int i = 0 ; i < 16 ; ++i){
    demo_nanosleep(nc, &scaled);
    if(ncplane_rotate_ccw(n)){
      goto err;
    }
    DEMO_RENDER(nc);
  }
  ncplane_destroy(n);
  return 0;

err:
  free(rgba);
  ncplane_destroy(n);
  return -1;
}

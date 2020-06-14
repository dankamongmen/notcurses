#include "demo.h"

// lame as fuck lol
static bool done;
static uint32_t pixel;
static int y, x, dy, dx;

static int
dragonmayer(struct ncvisual* ncv, const char* str, int iters){
  int total = 0;
  char c;
  int r;
  while( (c = *str++) ){
    switch(c){
      case 'X':
        if(iters > 1){
          if((r = dragonmayer(ncv, "X+YF+", iters - 1)) < 0){
            return r;
          }
          total += r;
        }
        break;
      case 'Y':
        if(iters > 1){
          if((r = dragonmayer(ncv, "-FX-Y", iters - 1)) < 0){
            return r;
          }
          total += r;
        }
        break;
      case '+': { int tmp = dy; dy = -dx; dx = tmp; break; }
      case '-': { int tmp = -dy; dy = dx; dx = tmp; break; }
      case 'F': // FIXME want a line
        if(ncvisual_set_yx(ncv, y, x, pixel) == 0){
          ++total;
        }
        x += dx;
        y += dy;
        break;
      default:
        return -1;
    }
  }
  return total;
}

int dragon_demo(struct notcurses* nc){
  done = false;
  int dimy, dimx;
  struct ncplane* n = notcurses_stddim_yx(nc, &dimy, &dimx);
  // we use a Lindenmayer string rewriting system. the classic dragon curve
  // system is X -> X+YF+, Y -> -FX-Y, where F is forward, - is turn left, and
  // + is turn right.
  const char LINDENSTART[] = "FX";
  const int SCALE = 1;
  dimy *= 2;
  dimx *= 2;
  int dxstart, dystart;
  if(dimy > dimx){
    dystart = 0;
    dxstart = SCALE;
  }else{
    dystart = SCALE;
    dxstart = 0;
  }
  size_t fbbytes = sizeof(uint32_t) * dimy * dimx;
  uint32_t* rgba = malloc(fbbytes);
  if(rgba == NULL){
    return -1;
  }
  memset(rgba, 0, fbbytes);
  for(int i = 0 ; i < dimy * dimx ; ++i){
    ncpixel_set_a(&rgba[i], 0xff);
  }
  struct ncvisual* ncv = ncvisual_from_rgba(rgba, dimy, dimx * sizeof(uint32_t), dimx);
  if(ncv == NULL){
    free(rgba);
    return -1;
  }
  free(rgba);
  struct timespec scaled;
  timespec_div(&demodelay, 4, &scaled);
  int lasttotal = 0;
  int iters = 0;
  int r = 0;
  do{
    ++iters;
    lasttotal = r;
    pixel = 0xffffffffull;
    ncpixel_set_rgb(&pixel, 0, 0xb * iters, 0);
    dx = dxstart;
    dy = dystart;
    x = dimx / 2;
    y = dimy / 3;
    r = dragonmayer(ncv, LINDENSTART, iters);
    if(r < 0){
      ncvisual_destroy(ncv);
      return r;
    }
    struct ncvisual_options vopts = {
      .n = n,
      .blitter = NCBLIT_2x2,
    };
    if(ncvisual_render(nc, ncv, &vopts) == NULL){
      ncvisual_destroy(ncv);
      return -1;
    }
    DEMO_RENDER(nc);
    demo_nanosleep(nc, &scaled);
  }while(lasttotal != r);
  ncvisual_destroy(ncv);
  return 0;
}

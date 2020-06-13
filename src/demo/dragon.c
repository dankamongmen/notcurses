#include "demo.h"

// lame as fuck lol
static bool done;
static uint32_t pixel;
static int y, x, dy, dx;

static int
dragonmayer(struct ncvisual* ncv, const char* str, int iters){
  char c;
  int r;
  while( (c = *str++) ){
    switch(c){
      case 'X':
        if(iters > 1){
          if( (r = dragonmayer(ncv, "X+YF+", iters - 1)) ){
            return r;
          }
        }
        break;
      case 'Y':
        if(iters > 1){
          if( (r = dragonmayer(ncv, "-FX-Y", iters - 1)) ){
            return r;
          }
        }
        break;
      case '+': { int tmp = dy; dy = -dx; dx = tmp; break; }
      case '-': { int tmp = -dy; dy = dx; dx = tmp; break; }
      case 'F': // FIXME want a line
        // FIXME some of these will fail...hella lame, check against dims
        if(ncvisual_set_yx(ncv, y, x, pixel) < 0){
          done = true;
        }
        x += dx;
        y += dy;
        break;
      default:
        return -1;
    }
  }
  return 0;
}

int dragon_demo(struct notcurses* nc){
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
  int iters = 0;
  do{
    ++iters;
    pixel = 0xffffffffull;
    ncpixel_set_rgb(&pixel, 0, 0x11 * iters, 0);
    dx = dxstart;
    dy = dystart;
    x = dimx / 2;
    y = dimy / 2;
    int r = dragonmayer(ncv, LINDENSTART, iters);
    if(r){
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
    ncplane_erase(n);
  }while(!done);
  ncvisual_destroy(ncv);
  return 0;
}

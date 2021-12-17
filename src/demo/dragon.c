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
        ncpixel_set_r(&pixel, 0xb0 - (iters * 0x10));
        ncpixel_set_a(&pixel, 0xff);
        if(ncvisual_set_yx(ncv, y, x, pixel) == 0){
          ++total;
        }
        pixel = 0;
        x += dx;
        y += dy;
        break;
      default:
        return -1;
    }
  }
  return total;
}

int dragon_demo(struct notcurses* nc, uint64_t startns){
  (void)startns;
  done = false;
  unsigned dimy, dimx;
  struct ncplane* n = notcurses_stddim_yx(nc, &dimy, &dimx);
  --dimy; // don't disturb the menu bar
  // we use a Lindenmayer string rewriting system. the classic dragon curve
  // system is X -> X+YF+, Y -> -FX-Y, where F is forward, - is turn left, and
  // + is turn right.
  const char LINDENSTART[] = "FX";
  const int vscale = 3;
  dimy *= vscale;
  dimy += (vscale - 1);
  dimx *= 2;
  int dxstart, dystart;
  if(dimy > dimx){
    dystart = 0;
    dxstart = 1;
  }else{
    dystart = 1;
    dxstart = 0;
  }
  size_t fbbytes = sizeof(uint32_t) * dimy * dimx;
  uint32_t* rgba = malloc(fbbytes);
  if(rgba == NULL){
    return -1;
  }
  memset(rgba, 0, fbbytes);
  struct ncvisual* ncv = ncvisual_from_rgba(rgba, dimy, dimx * sizeof(uint32_t), dimx);
  free(rgba);
  if(ncv == NULL){
    return -1;
  }
  struct timespec scaled;
  timespec_div(&demodelay, 8, &scaled);
  int lasttotal = 0;
  int iters = 0;
  int r = 0;
  do{
    ++iters;
    lasttotal = r;
    dx = dxstart;
    dy = dystart;
    x = dimx / 2;
    y = dimy / vscale;
    r = dragonmayer(ncv, LINDENSTART, iters);
    if(r < 0){
      ncvisual_destroy(ncv);
      return r;
    }
    struct ncvisual_options vopts = {
      .n = n,
      .y = 1,
      .scaling = NCSCALE_STRETCH,
      .flags = NCVISUAL_OPTION_CHILDPLANE,
    };
    struct ncplane* dn = ncvisual_blit(nc, ncv, &vopts);
    if(dn == NULL){
      ncvisual_destroy(ncv);
      return -1;
    }
    DEMO_RENDER(nc);
    demo_nanosleep(nc, &scaled);
    ncplane_destroy(dn);
  }while(lasttotal != r);
  ncvisual_destroy(ncv);
  return 0;
}

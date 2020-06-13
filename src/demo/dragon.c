#include "demo.h"

// lame as fuck lol
static bool done;
static int y, x, dy, dx;

static int
dragonmayer(struct ncplane* n, const char* str, int iters){
  char c;
  int r;
  while( (c = *str++) ){
    switch(c){
      case 'X':
        if(iters > 1){
          if( (r = dragonmayer(n, "X+YF+", iters - 1)) ){
            return r;
          }
        }
        break;
      case 'Y':
        if(iters > 1){
          if( (r = dragonmayer(n, "-FX-Y", iters - 1)) ){
            return r;
          }
        }
        break;
      case '+': { int tmp = dy; dy = -dx; dx = tmp; break; }
      case '-': { int tmp = -dy; dy = dx; dx = tmp; break; }
      case 'F': // FIXME want a line
        // FIXME some of these will fail...hella lame, check against dims
        if(ncplane_putsimple_yx(n, y, x, '@') <= 0){
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
  int dxstart, dystart;
  if(dimy > dimx){
    dystart = 0;
    dxstart = SCALE;
  }else{
    dystart = SCALE;
    dxstart = 0;
  }
  struct timespec scaled;
  timespec_div(&demodelay, 4, &scaled);
  int iters = 0;
  do{
    ++iters;
    if(ncplane_set_fg_rgb(n, 0, 0x11 * iters, 0)){
      break;
    }
    dx = dxstart;
    dy = dystart;
    x = dimx / 2;
    y = dimy / 2;
    int r = dragonmayer(n, LINDENSTART, iters);
    if(r){
      return r;
    }
    DEMO_RENDER(nc);
    demo_nanosleep(nc, &scaled);
    ncplane_erase(n);
  }while(!done);
  return 0;
}

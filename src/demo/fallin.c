#include "demo.h"

// Shuffle a new ncplane into the array of ncplanes having 'count' elements.
static struct ncplane**
shuffle_in(struct ncplane** arr, int count, struct ncplane* n){
  struct ncplane** tmp = realloc(arr, sizeof(*arr) * (count + 1));
  if(tmp == NULL){
    return NULL;
  }
  arr = tmp;
  // location of new element
  int pos = random() % (count + 1);
  if(pos < count){
    // move everything, starting at our new location, one spot right
    memmove(arr + pos, arr + pos + 1, sizeof(*arr) * (count - pos));
  }
  arr[pos] = n;
  return arr;
}

// ya playin' yourself
int fallin_demo(struct notcurses* nc){
  int dimx, dimy;
  ncplane_dim_yx(notcurses_stdplane(nc), &dimy, &dimx);
  size_t usesize = sizeof(bool) * dimy * dimx;
  bool* usemap = malloc(usesize);
  memset(usemap,0, sizeof(*usemap));
  // bricks are bounded in size according to the screen size.
  const int maxx = dimx / 20;
  const int maxy = dimy / 20;
  // proceed from top to bottom, left to right, and partition the existing
  // content into 'atotal' copies into small ncplanes.
  // make a copy of the standard plane so that we don't need rederive the
  // world in the event of a resize event
  struct ncplane** arr = NULL;
  int arrcount = 0;
  for(int y = 0 ; y < dimy ; ++y){
    for(int x = 0 ; x < dimx ; ++x){
      if(usemap[y * dimx + x]){
        continue;
      }
      int newy, newx;
      newy = (random() % maxy) + 1;
      newx = (random() % maxx) + 1;
      if(x + newx >= dimx){
        newx = dimx - x;
      }
      if(y + newy >= dimy){
        newy = dimy - y;
      }
      struct ncplane* n = ncplane_new(nc, newy, newx, y, x, NULL);
      if(n == NULL){
        return -1;
      }
      for(int usey = y ; usey < y + newy ; ++usey){
        for(int usex = x ; usex < x + newx ; ++usex){
          assert(!usemap[usey * dimx + usex]);
          cell c = CELL_TRIVIAL_INITIALIZER;
          if(ncplane_at_yx(notcurses_stdplane(nc), usey, usex, &c) < 0){
            return -1;
          }
          if(!cell_simple_p(&c)){
            cell_load(n, &c, cell_extended_gcluster(notcurses_stdplane(nc), &c));
          }
          if(ncplane_putc_yx(n, usey - y, usex - x, &c) < 0){
            return -1;
          }
          usemap[usey * dimx + usex] = true;
          cell_release(n, &c);
        }
      }
      struct ncplane **tmp;
      tmp = shuffle_in(arr, arrcount, n);
      if(tmp == NULL){
        return -1;
      }
      arr = tmp;
      ++arrcount;
fprintf(stderr, "box %d/%d -> %d/%d\n", y, x, y + newy - 1, x + newx - 1);
    }
  }
  ncplane_erase(notcurses_stdplane(nc));
  if(notcurses_render(nc)){
    return -1;
  }
  // FIXME shuffle up a list of all coordinates, then walk through them. each
  // one ought be turned into its own small ncplane, erased from ndup, and the
  // plane set falling.
  return 0;
}

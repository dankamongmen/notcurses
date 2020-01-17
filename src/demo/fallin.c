#include "demo.h"

static int
drop_bricks(struct notcurses* nc, struct ncplane** arr, int arrcount){
  if(arrcount == 0 || arr == NULL){
    return -1;
  }
  // an erase+render cycle ought not change the screen, as we duplicated it
  struct timespec iterdelay;
  // 5 * demodelay total
  ns_to_timespec(timespec_to_ns(&demodelay) * 5 / arrcount, &iterdelay);
  ncplane_erase(notcurses_stdplane(nc));
  for(int n = 0 ; n < arrcount ; ++n){
fprintf(stderr, "PLANE %d: %p\n", n, arr[n]);
    ncplane_erase(arr[n]);
    if(notcurses_render(nc)){
      return -1;
    }
    if(ncplane_destroy(arr[n])){
      return -1;
    }
    nanosleep(&iterdelay, NULL);
  }
  free(arr);
  return 0;
}

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
fprintf(stderr, "pos %d count %d moving %d\n", pos, count, count - pos);
  if(pos < count){
    // move everything, starting at our new location, one spot right
    memmove(arr + pos + 1, arr + pos, sizeof(*arr) * (count - pos));
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
  memset(usemap, 0, usesize);
  // brick size is relative to the screen size
  const int maxx = dimx > 39 ? dimx / 20 : 2;
  const int maxy = dimy > 19 ? dimy / 10 : 2;
  // proceed from top to bottom, left to right, and partition the existing
  // content into 'arrcount' copies into small ncplanes in 'arr'.
  struct ncplane** arr = NULL;
  int arrcount = 0;
  // There are a lot of y/x pairs at this point:
  //  * dimx/dimy: geometry of standard plane
  //  * y/x: iterators through standard plane
  //  * maxy/maxx: maximum geometry of randomly-generated bricks
  //  * newy/newx: actual geometry of current brick
  //  * usey/usex: 
  for(int y = 0 ; y < dimy ; ++y){
    int x = 0;
    while(x < dimx){
      if(usemap[y * dimx + x]){ // skip if we've already been copied
        ++x;
        continue;
      }
      int newy, newx;
      newy = random() % (maxy - 1) + 2;
      newx = random() % (maxx - 1) + 2;
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
      // copy the old content into this new ncplane
      for(int usey = y ; usey < y + newy ; ++usey){
        for(int usex = x ; usex < x + newx ; ++usex){
          assert(!usemap[usey * dimx + usex]);
          cell c = CELL_TRIVIAL_INITIALIZER;
          if(ncplane_at_yx(notcurses_stdplane(nc), usey, usex, &c) < 0){
            return -1;
          }
          if(!cell_simple_p(&c)){
            const char* cons = cell_extended_gcluster(notcurses_stdplane(nc), &c);
            c.gcluster = 0;
            cell_load(n, &c, cons);
          }
          if(ncplane_putc_yx(n, usey - y, usex - x, &c) < 0){
            return -1;
          }
          usemap[usey * dimx + usex] = true;
          cell_release(n, &c);
        }
      }
      // shuffle the new ncplane into the array
      struct ncplane **tmp;
      tmp = shuffle_in(arr, arrcount, n);
      if(tmp == NULL){
        return -1;
      }
      arr = tmp;
      ++arrcount;
      x += newx;
    }
  }
  free(usemap);
  return drop_bricks(nc, arr, arrcount);
}

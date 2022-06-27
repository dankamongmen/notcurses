#include "demo.h"
#include <pthread.h>

static int
drop_bricks(struct notcurses* nc, struct ncplane** arr, int arrcount){
  if(arrcount == 0 || arr == NULL){
    return -1;
  }
  unsigned stdy, stdx;
  notcurses_term_dim_yx(nc, &stdy, &stdx);
  // an erase+render cycle ought not change the screen, as we duplicated it
  struct timespec iterdelay;
  // 5 * demodelay total
  ns_to_timespec(timespec_to_ns(&demodelay) / arrcount / 2, &iterdelay);
  // we've got a range of up to 10% total blocks falling at any given time. they
  // accelerate as they fall. [ranges, range) covers the active range.
  int ranges = 0;
  int rangee = 0;
  const int FALLINGMAX = arrcount < 10 ? 1 : arrcount / 10;
  int* speeds = malloc(sizeof(*speeds) * FALLINGMAX);
  if(speeds == NULL){
    return -1;
  }
  while(ranges < arrcount){
    // if we don't have a full set active, and there is another available, go
    // ahead and get it kicked off
    if(rangee - ranges + 1 < FALLINGMAX){
      if(rangee < arrcount){
        int y;
        ncplane_yx(arr[ranges], &y, NULL);
        speeds[rangee - ranges] = y < (int)stdy / 2 ? 1 : -1;
        ++rangee;
      }
    }
    do{
      DEMO_RENDER(nc);
      // don't allow gaps in the active range. so long as felloff is true, we've only handled
      // planes which have fallen off the screen, and can be collected.
      bool felloff = true;
      for(int i = 0 ; i < rangee - ranges ; ++i){
        struct ncplane* ncp = arr[ranges + i];
        ncplane_move_top(ncp);
        int x, y;
        ncplane_yx(ncp, &y, &x);
        if(felloff){
          if(y + speeds[i] >= (int)stdy || y + speeds[i] + (int)ncplane_dim_y(ncp) < 0){
            ncplane_destroy(ncp);
            arr[ranges + i] = NULL;
            if(ranges + i + 1 == rangee){
              ranges += i + 1;
              break;
            }
          }else{ // transition point
            if(i){
              if(rangee - ranges - i){
                memmove(speeds, speeds + i, (rangee - ranges - i) * sizeof(*speeds));
              }
              ranges += i;
              i = 0;
            }
            felloff = false;
          }
        }
        if(!felloff){
          ncplane_move_yx(ncp, y + speeds[i], x);
          if(speeds[i] < 0){
            --speeds[i];
          }else{
            ++speeds[i];
          }
        }else if(i){
          if(rangee - ranges - i){
            memmove(speeds, speeds + i, (rangee - ranges - i) * sizeof(*speeds));
          }
          ranges += i;
          i = 0;
        }
        demo_nanosleep(nc, &iterdelay);
      }
    }while(rangee - ranges + 1 >= FALLINGMAX);
  }
  free(speeds);
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
  int pos = rand() % (count + 1);
  if(pos < count){
    // move everything, starting at our new location, one spot right
    memmove(arr + pos + 1, arr + pos, sizeof(*arr) * (count - pos));
  }
  arr[pos] = n;
  return arr;
}

// you played yourself https://genius.com/De-la-soul-fallin-lyrics
int fission_demo(struct notcurses* nc, uint64_t startns){
  (void)startns;
  struct ncplane* npl = NULL;
  unsigned dimx, dimy;
  struct ncplane* stdn = notcurses_stddim_yx(nc, &dimy, &dimx);
  size_t usesize = sizeof(bool) * dimy * dimx;
  bool* usemap = malloc(usesize);
  if(usemap == NULL){
    return -1;
  }
  memset(usemap, 0, usesize);
  // brick size is relative to the screen size
  const int maxx = dimx > 39 ? dimx / 10 : 2;
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
  ncplane_greyscale(stdn);
  for(unsigned y = 1 ; y < dimy ; ++y){
    unsigned x = 0;
    while(x < dimx){
      if(usemap[y * dimx + x]){ // skip if we've already been copied
        ++x;
        continue;
      }
      int newy, newx;
      newy = rand() % (maxy - 1) + 2;
      newx = rand() % (maxx - 1) + 2;
      if(x + newx >= dimx){
        newx = dimx - x;
      }
      if(y + newy >= dimy){
        newy = dimy - y;
      }
      struct ncplane_options nopts = {
        .y = y,
        .x = x,
        .rows = newy,
        .cols = newx,
      };
      struct ncplane* n = ncplane_create(stdn, &nopts);
      if(n == NULL){
        goto err;
      }
      // copy the old content into this new ncplane
      for(unsigned usey = y ; usey < y + newy ; ++usey){
        for(unsigned usex = x ; usex < x + newx ; ++usex){
          if(usemap[usey * dimx + usex]){
            newx = usex - x;
            ncplane_resize_simple(n, newy, newx);
            continue;
          }
          nccell c = NCCELL_TRIVIAL_INITIALIZER;
          uint16_t smask;
          uint64_t channels;
          char* egc = ncplane_at_yx(stdn, usey, usex, &smask, &channels);
          if(egc == NULL){
            goto err;
          }
          if(*egc){
            if(nccell_prime(n, &c, egc, smask, channels) <= 0){
              free(egc);
              goto err;
            }
            free(egc);
            if(ncplane_putc_yx(n, usey - y, usex - x, &c) < 0){
              // allow a fail if we were printing a wide char to the
              // last column of our plane
              if(!nccell_double_wide_p(&c) || usex + nccell_cols(&c) - 1 < x + newx){
                nccell_release(n, &c);
                goto err;
              }
            }
          }else{
            free(egc);
          }
          usemap[usey * dimx + usex] = true;
          nccell_release(n, &c);
        }
      }
      // shuffle the new ncplane into the array
      struct ncplane **tmp;
      tmp = shuffle_in(arr, arrcount, n);
      if(tmp == NULL){
        goto err;
      }
      arr = tmp;
      ++arrcount;
      x += newx;
    }
  }
  ncplane_erase(stdn);
#ifndef DFSG_BUILD
  if(notcurses_canopen_images(nc)){
    char* path = find_data("lamepatents.jpg");
    struct ncvisual* ncv = ncvisual_from_file(path);
    free(path);
    if(ncv == NULL){
      goto err;
    }
    struct ncvisual_options vopts = {
      .n = stdn,
      .scaling = NCSCALE_STRETCH,
      .flags = NCVISUAL_OPTION_CHILDPLANE,
    };
    if((npl = ncvisual_blit(nc, ncv, &vopts)) == NULL){
      ncvisual_destroy(ncv);
      goto err;
    }
    assert(ncvisual_decode(ncv) == 1);
    ncvisual_destroy(ncv);
    ncplane_move_above(npl, stdn);
  }
#endif
  int ret = drop_bricks(nc, arr, arrcount);
  free(arr);
  free(usemap);
  ncplane_destroy(npl);
  return ret;

err:
  free(usemap);
  free(arr);
  ncplane_destroy(npl);
  return -1;
}

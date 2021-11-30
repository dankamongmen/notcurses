#include "demo.h"

// assign to r/g/b the minimum permutation summing to total, inc total by step
static void
minimize(unsigned *total, unsigned *r, unsigned *g, unsigned *b, unsigned step){
  *b = *r >= 256 ? 256 : *r + step;
  *r = (*total += step) - (*b + *g);
}

// derive the next color based on current state. *'total' ranges from 0 to 768
// by 'step'. we increment it upon maximizing the permutated component colors
// summing to that value. while the 'r', 'g', and 'b', components range from 0
// to 256, the encoded values top out at 255 ({255, 256} -> 255). at all times,
// *'r' + *'g' + *'b' must sum to *'total'. on input, rgb specifies the return
// value. on output, rgb specifies the next return value, and total might be
// changed. if total was changed, r is minimized, and g is then minimized,
// subject to the total. thus we twist and shout through rgb space by 'step'.
static unsigned
generate_next_color(unsigned *total, unsigned *r, unsigned *g, unsigned *b,
                    unsigned step){
  const unsigned ret = ((((*r - (*r == 256)) << 8u) + (*g - (*g == 256))) << 8u)
                        + (*b - (*b == 256));
  if(*total <= 256){
    if(*r == *total){
      minimize(total, r, g, b, step);
    }else{
      if(*b){
        *g += step;
      }else{
        *r += step;
        *g = 0;
      }
      *b = *total - (*g + *r);
    }
  }else if(*total <= 512){
    if(*r == 256 && *r + *g == *total){
      minimize(total, r, g, b, step);
    }else{
      if(*g == 256 || *g == (*total - *r)){
        *r += step;
        *b = *total - *r > 256 ? 256 : *total - *r;
      }else{
        *b -= step;
      }
      *g = *total - (*b + *r);
    }
  }else{
    if(*r == 256 && *g == 256){
      minimize(total, r, g, b, step);
    }else{
      if(*g == 256){
        *r += step;
        *b = 256;
      }else{
        *b -= step;
      }
      *g = *total - (*r + *b);
    }
  }
  return ret;
}

int highcon_demo(struct notcurses* nc, uint64_t startns){
  (void)startns;
  const int STEP = 16;
  int ret = -1;
  unsigned dimy, dimx;
  struct ncplane* n = notcurses_stddim_yx(nc, &dimy, &dimx);
  int totcells = (dimy - 1) * dimx;
  if(totcells <= 1){
    return -1;
  }
  unsigned* scrcolors = malloc(sizeof(*scrcolors) * totcells);
  if(scrcolors == NULL){
    return -1;
  }
  const char motto[] = " high contrast text is evaluated relative to the solved background";
  nccell c = NCCELL_TRIVIAL_INITIALIZER;
  unsigned total = 0, r = 0, g = 0, b = 0;
  for(int out = 0 ; out < totcells ; ++out){ // build up the initial screen
    scrcolors[out] = generate_next_color(&total, &r, &g, &b, STEP);
    if(total > 768){
      total = r = g = b = 0;
    }
    nccell_load_char(n, &c, motto[out % strlen(motto)]);
    nccell_set_fg_alpha(&c, NCALPHA_HIGHCONTRAST);
    nccell_set_bg_rgb(&c, scrcolors[out % totcells]);
    if(ncplane_putc_yx(n, (out + dimx) / dimx, out % dimx, &c) < 0){
      free(scrcolors);
      goto err;
    }
  }
  free(scrcolors);
  // each iteration, "draw the background in" one cell from the top left and
  // bottom right.
  int offset = 0;
  struct timespec start;
  clock_gettime(CLOCK_MONOTONIC, &start);
  uint64_t totalns = timespec_to_ns(&demodelay) * 2;
  uint64_t iterns = totalns / (totcells / 2);
  do{
    if(offset){
      nccell_set_fg_alpha(&c, NCALPHA_OPAQUE);
      const int f = offset - 1 + dimx;
      const int l = totcells + dimx - offset;
      ncplane_at_yx_cell(n, f / dimx, f % dimx, &c);
      nccell_set_fg_rgb(&c, 0x004000 + (16 * offset));
      nccell_set_bg_rgb(&c, 0);
      nccell_set_fg_alpha(&c, NCALPHA_OPAQUE);
      if(ncplane_putc_yx(n, f / dimx, f % dimx, &c) < 0){
        goto err;
      }
      ncplane_at_yx_cell(n, l / dimx, l % dimx, &c);
      nccell_set_fg_rgb(&c, 0x004000 + (16 * offset));
      nccell_set_bg_rgb(&c, 0);
      nccell_set_fg_alpha(&c, NCALPHA_OPAQUE);
      if(ncplane_putc_yx(n, l / dimx, l % dimx, &c) < 0){
        goto err;
      }
    }
    DEMO_RENDER(nc);
    uint64_t targns = timespec_to_ns(&start) + offset * iterns;
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    if(targns > timespec_to_ns(&now)){
      struct timespec abstime;
      ns_to_timespec(targns, &abstime);
      if((ret = demo_nanosleep_abstime(nc, &abstime))){
          goto err;
      }
    }
  }while(++offset <= totcells / 2);
  ret = 0;

err:
  nccell_release(n, &c);
  return ret;
}

#include "demo.h"

// assign to r/g/b the minimum permutation summing to total, inc total by step
static void
minimize(unsigned *total, unsigned *r, unsigned *g, unsigned *b, unsigned step){
  *total += step;
  *r = *total > 512 ? *total - 512 : 0;
  *g = *total > 256 ? *total - 256 - *r : 0;
  *b = *total - (*r + *g);
}

// derive the next color based on current state. *'total' ranges from 0 to 768
// by 'step'. we increment it upon maximizing the permutated component colors
// summing to that value. while the 'r', 'g', and 'b', components range from 0
// to 256, the encoded values top out at 255 ({255, 256} -> 255). at all times,
// *'r' + *'g' + *'b' must sum to *'total'. on input, rgb specifies the return
// value. on output, rgb specifies the next return value, and total might be
// changed. if total was changed, r is miminimized, and g is then minimized,
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
    }else if(*g == 256){
      *r += step;
      *b = 256;
    }else{
      *b -= step;
    }
    *g = *total - (*r + *b);
  }
  return ret;
}

int highcontrast_demo(struct notcurses* nc){
  const int STEP = 4;
  int dimy, dimx;
  struct ncplane* n = notcurses_stdplane(nc);
  ncplane_dim_yx(n, &dimy, &dimx);
  int totcells = dimy * dimx;
  unsigned iter = 0;
  // circarray of current background colors, logical start at iter % totcells.
  unsigned* scrcolors = malloc(sizeof(*scrcolors) * totcells);
  if(scrcolors == NULL){
    return -1;
  }
  // FIXME fill screen with a matrix of text, all CELL_ALPHA_HIGHCONTRAST
  unsigned total = 0, r = 0, g = 0, b = 0;
  for(int out = 0 ; out < totcells ; ++out){ // build up the initial screen
    scrcolors[out] = generate_next_color(&total, &r, &g, &b, STEP);
    if(total > 768){
      total = r = g = b = 0;
    }
  }
  // each iteration, generate the next color, and introduce it at the lower
  // right. start at the upper left, from the logical beginning of the array.
  cell c = CELL_SIMPLE_INITIALIZER(' ');
  do{
    unsigned idx = iter % totcells; // first color for upper-left
    for(int yx = 0 ; yx < dimy * dimx ; ++yx){
      cell_set_bg(&c, scrcolors[idx]);
      if(ncplane_putc_yx(n, yx / dimx, yx % dimx, &c) < 0){
        goto err;
      }
      idx = (idx + 1) % totcells;
    }
    scrcolors[iter++ % totcells] = generate_next_color(&total, &r, &g, &b, STEP);
    if(notcurses_render(nc)){
      goto err;
    }
  }while(total <= 768);
  cell_release(n, &c);
  free(scrcolors);
  return 0;

err:
  cell_release(n, &c);
  free(scrcolors);
  return -1;
}

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

int highcontrast_demo(struct notcurses* nc){
  const int STEP = 16;
  int ret = -1;
  int dimy, dimx;
  struct ncplane* n = notcurses_stddim_yx(nc, &dimy, &dimx);
  int totcells = (dimy - 1) * dimx;
  unsigned* scrcolors = malloc(sizeof(*scrcolors) * totcells);
  if(scrcolors == NULL){
    return -1;
  }
  unsigned total = 0, r = 0, g = 0, b = 0;
  for(int out = 0 ; out < totcells ; ++out){ // build up the initial screen
    scrcolors[out] = generate_next_color(&total, &r, &g, &b, STEP);
    if(total > 768){
      total = r = g = b = 0;
    }
  }
  cell c = CELL_TRIVIAL_INITIALIZER;
  const char motto[] = " high contrast text ";
  // each iteration, "draw the background in" one cell from the top left and
  // bottom right.
  int offset = 0;
  do{
    if(offset){
      cell_set_fg_alpha(&c, CELL_ALPHA_OPAQUE);
      const int f = offset - 1 + dimx;
      const int l = totcells + dimx - offset;
      cell_load_simple(n, &c, motto[f % strlen(motto)]);
      cell_set_fg(&c, 0x004000 + (16 * offset));
      cell_set_bg(&c, 0);
      if(ncplane_putc_yx(n, f / dimx, f % dimx, &c) < 0){
        goto err;
      }
      cell_load_simple(n, &c, motto[l % strlen(motto)]);
      if(ncplane_putc_yx(n, l / dimx, l % dimx, &c) < 0){
        goto err;
      }
    }
    cell_set_fg_alpha(&c, CELL_ALPHA_HIGHCONTRAST);
    for(int yx = offset + dimx ; yx < totcells - offset ; ++yx){
      cell_load_simple(n, &c, motto[yx % strlen(motto)]);
      cell_set_fg_rgb(&c, (random() % 2) * 0xff, (random() % 2) * 0xff, (random() % 2) * 0xff);
      cell_set_bg(&c, scrcolors[yx % totcells]);
      if(ncplane_putc_yx(n, (yx + dimx) / dimx, yx % dimx, &c) < 0){
        goto err;
      }
    }
    DEMO_RENDER(nc);
  }while(++offset < totcells / 2);
  ret = 0;

err:
  cell_release(n, &c);
  free(scrcolors);
  return ret;
}

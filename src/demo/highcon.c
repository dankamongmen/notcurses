#include "demo.h"

// Derive the next color based on current state. *'total' ranges from 0 to 768
// in steps of 4. We increment it upon exhaustion of component colors summing
// to that value. While the 'r', 'g', and 'b', components range from 0 to 256,
// the return value caps each component at 255.
static unsigned
generate_next_color(unsigned *total, unsigned *r, unsigned *g, unsigned *b){
}

int highcontrast_demo(struct notcurses* nc){
  int dimy, dimx;
  struct ncplane* n = notcurses_stdplane(nc);
  ncplane_dim_yx(n, &dimy, &dimx);
  int totcells = dimy * dimx;
  unsigned iteration = 0;
  // circular array of current background colors, starting logically at
  // iteration % totcells.
  unsigned* scrcolors = malloc(sizeof(*scrcolors) * totcells);
  if(scrcolors == NULL){
    return -1;
  }
  // build up the initial screen
  do{
  }while(iterations < totcells);
  // we want to fill one screen (totcells) with a broad spectrum of color. rgb
  // can have between 0 and 765 total component units (0/0/0 vs 255/255/255).
  // each of those totals can result in 1 color. the total 384 will maximize 
  // the number of colors, with 200 at intervals of 16. at 512+, all components
  // must be positive; at 254-, no components can be maximized. at intervals of
  // 4, we have ~5M colors between 0..765.
  unsigned total, r, g, b;
  total = r = g = b = 0;
  unsigned rgb;
  // each iteration, generate the next color, and introduce it at the lower
  // right. start at the upper left, from the logical beginning of the array.
  cell c = CELL_SIMPLE_INITIALIZER(' ');
  do{
    int idx = iterations % totcells; // first color for upper-left
    for(int y = 0 ; y < dimy ; ++y){
      for(int x = 0 ; x < dimx ; ++x){
        cell_set_fg();
        if(ncplane_putc_yx(n, y, x, &c) < 0){
          goto err;
        }
      }
    }
    rgb = generate_next_color(&total, &r, &g, &b);
    if(notcurses_render(nc)){
      goto err;
    }
  }while(rgb < 0xffffff);
  // FIXME fill screen with a matrix of text, all high-contrast
  // FIXME cycle the background matrix
  free(scrcolors);
  return 0;

err:
  release_cell(n, &c);
  free(scrcolors);
  return -1;
}

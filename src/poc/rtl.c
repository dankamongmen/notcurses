#include <stdlib.h>
#include <notcurses/notcurses.h>

int main(void){
  struct notcurses_options opts = {
    .flags = NCOPTION_NO_ALTERNATE_SCREEN,
  };
  struct notcurses* nc = notcurses_init(&opts, NULL);
  if(!nc){
    return EXIT_FAILURE;
  }
  int dimy, dimx;
  struct ncplane* n = notcurses_stddim_yx(nc, &dimy, &dimx);
  // no matter how the following string is displayed, the first hebrew
  // character is מ (mem), followed by י (yod), ל (lamed), etc.
  if(ncplane_printf_aligned(n, dimy / 2, NCALIGN_CENTER,
      "I can write English with מילים בעברית in the same sentence.") < 0){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  if(ncplane_printf_aligned(n, dimy / 2 + 1, NCALIGN_CENTER,
      "I can write English withמיליםבעבריתin the same sentence.") < 0){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  const char* brew = "םבעברஸீரோகிரிﻧﺎﻠﻘﺻﺮﻴﻧﻖﺻﺭﺎoshitﻠﺷﻮﻗﺎﻠﺴﻛﺮﻳﺓ";
  const char *b = brew;
  for(int y = dimy / 2 + 2 ; y < dimy ; ++y){
    for(int x = 0 ; x < dimx ; ++x){
      int bytes;
      if(ncplane_putegc_yx(n, y, x, b, &bytes) <= 0){
        break;
      }
      b += bytes;
      if(!*b){
        b = brew;
      }
    }
  }
  struct ncplane_options nopts = {
    .y = dimy / 2 + 3,
    .x = 8,
    .rows = dimy - (dimy / 2 + 5),
    .cols = dimx - 8 * 2,
  };
  struct ncplane* top = ncplane_create(n, &nopts);
  ncplane_set_base(top, " ", 0, 0);
  if(notcurses_render(nc)){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  notcurses_stop(nc);
  return EXIT_SUCCESS;
}

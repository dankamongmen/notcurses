#include <stdlib.h>
#include <notcurses/notcurses.h>

int main(void){
  struct notcurses_options nopts = {
    .flags = NCOPTION_NO_ALTERNATE_SCREEN,
  };
  struct notcurses* nc = notcurses_init(&nopts, NULL);
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
  if(notcurses_render(nc)){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  notcurses_stop(nc);
  return EXIT_SUCCESS;
}

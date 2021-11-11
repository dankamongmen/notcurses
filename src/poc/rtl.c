#include <stdlib.h>
#include <notcurses/notcurses.h>

int main(void){
  struct notcurses_options opts = {
    .flags = NCOPTION_NO_ALTERNATE_SCREEN
              | NCOPTION_DRAIN_INPUT,
  };
  struct notcurses* nc = notcurses_core_init(&opts, NULL);
  if(!nc){
    return EXIT_FAILURE;
  }
  unsigned dimy, dimx;
  struct ncplane* n = notcurses_stddim_yx(nc, &dimy, &dimx);
  ncplane_printf_yx(n, dimy / 2 - 2, 0, "ࡢ‎ࡣ‎ࡤ‎ࡥ‎ࡦ‎ࡧ‎ࡨ‎ࡩ‎ࡪ‎ࢠ‎ࢡ‎ࢢ‎ࢣ‎ࢤ‎ࢥ‎ࢦ‎ࢧ‎ࢨ‎ࢩ‎ࢪ‎ࢫ‎ࢬ‎ࢭ‎ࢮ‎ࢯ‎ࢰ‎ࢱ‎ࢲ‎ࢳ‎ࢴ‎ࢶ‎ࢷ‎ࢸ‎ࢹ‎ࢺ‎ࢻ‎ࢼ‎ࢽ‎࣢ः");
  ncplane_printf_yx(n, dimy / 2 - 1, 0, "࠲‎࠳‎࠴‎࠵‎࠶‎࠷‎࠸‎࠹‎࠺‎࠻‎࠼‎࠽‎࠾‎ࡀ‎ࡁ‎ࡂ‎ࡃ‎ࡄ‎ࡅ‎ࡆ‎ࡇ‎ࡈ‎ࡉ‎ࡊ‎ࡋ‎ࡌ‎ࡍ‎ࡎ‎ࡏ‎ࡐ‎ࡑ‎ࡒ‎ࡓ‎ࡔ‎ࡕ‎ࡖ‎ࡗ‎ࡘ‎࡞‎ࡡ");
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
  for(unsigned y = dimy / 2 + 2 ; y < dimy ; ++y){
    for(unsigned x = 0 ; x < dimx ; ++x){
      size_t bytes;
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

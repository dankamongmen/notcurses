#include <notcurses/notcurses.h>

int main(void){
  // explicitly want alternate screen, so no NCOPTION_CLI_MODE
  struct notcurses_options opts = {
    .flags = NCOPTION_PRESERVE_CURSOR |
             NCOPTION_NO_CLEAR_BITMAPS |
             NCOPTION_DRAIN_INPUT,
  };
  struct notcurses* nc = notcurses_init(&opts, NULL);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  notcurses_render(nc);
  notcurses_stop(nc);
  return EXIT_SUCCESS;
}

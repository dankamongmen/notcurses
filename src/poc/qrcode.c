#include <locale.h>
#include <notcurses/notcurses.h>

int main(int argc, const char** argv){
  if(setlocale(LC_ALL, "") == NULL){
    return EXIT_FAILURE;
  }
  struct notcurses_options opts = {
    .flags = NCOPTION_INHIBIT_SETLOCALE | NCOPTION_NO_ALTERNATE_SCREEN;
  };
  struct notcurses* nc = notcurses_init(&opts, NULL);

  notcurses_stop(nc);
  return EXIT_SUCCESS;
}

#include <cstdlib>
#include <clocale>
#include <unistd.h>
#include <notcurses.h>

// dump two wide glyphs, then create a new plane and drop it atop them

int main(void){
  setlocale(LC_ALL, "");
  notcurses_options opts{};
  struct notcurses* nc = notcurses_init(&opts, stdout);
  struct ncplane* n = notcurses_stdplane(nc);

  ncplane_putstr(n, "\u5168");
  ncplane_putstr(n, "\u5f62");
  notcurses_render(nc);
  sleep(1);
  cell c = CELL_TRIVIAL_INITIALIZER;
  char* egc;
  notcurses_stop(nc);
  return EXIT_SUCCESS;
}

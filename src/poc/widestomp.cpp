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

  ncplane_set_fg_rgb(n, 0x80, 0xc0, 0x80);
  ncplane_set_bg_rgb(n, 0x00, 0x40, 0x00);
  ncplane_putstr(n, "\u5168");
  ncplane_putstr(n, "\u5f62");
  notcurses_render(nc);
  sleep(1);
  struct ncplane* nn = ncplane_new(nc, 1, 2, 0, 1, nullptr);
  ncplane_set_fg_rgb(nn, 0xc0, 0x80, 0xc0);
  ncplane_set_bg_rgb(nn, 0x20, 0x00, 0x20);
  ncplane_putstr(nn, "AB");
  notcurses_render(nc);
  sleep(1);
  cell c = CELL_TRIVIAL_INITIALIZER;
  char* egc;
  notcurses_stop(nc);
  return EXIT_SUCCESS;
}

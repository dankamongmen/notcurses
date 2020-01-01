#include <cstdlib>
#include <clocale>
#include <unistd.h>
#include <notcurses.h>

constexpr auto DELAY = 1;

// dump two wide glyphs, then create a new plane and drop it atop them

int stomper(struct notcurses* nc, struct ncplane* nn){
  ncplane_move_yx(nn, 0, 1);

  notcurses_render(nc);
  sleep(DELAY);

  // first wide glyph gone, second present
  ncplane_move_yx(nn, 1, 0);
  notcurses_render(nc);
  sleep(DELAY);

  // second wide glyph gone, first present
  ncplane_move_yx(nn, 2, 2);
  notcurses_render(nc);
  sleep(DELAY);

  ncplane_move_yx(nn, 4, 0);
  notcurses_render(nc);
  sleep(DELAY);

  ncplane_move_yx(nn, 5, 1);
  notcurses_render(nc);
  sleep(DELAY);

  ncplane_move_yx(nn, 6, 2);
  notcurses_render(nc);
  sleep(DELAY);

  return 0;
}

int main(void){
  setlocale(LC_ALL, "");
  notcurses_options opts{};
  struct notcurses* nc = notcurses_init(&opts, stdout);
  struct ncplane* n = notcurses_stdplane(nc);

  // first, a 2x1 with "AB"
  struct ncplane* nn = ncplane_new(nc, 1, 2, 1, 16, nullptr);
  ncplane_set_fg_rgb(nn, 0xc0, 0x80, 0xc0);
  ncplane_set_bg_rgb(nn, 0x20, 0x00, 0x20);
  ncplane_putstr(nn, "AB");

  ncplane_set_fg_rgb(n, 0x80, 0xc0, 0x80);
  ncplane_set_bg_rgb(n, 0x00, 0x40, 0x00);
  ncplane_putstr(n, "\xe5\xbd\xa2\xe5\x85\xa8");
  ncplane_putstr_yx(n, 1, 0, "\xe5\xbd\xa2\xe5\x85\xa8");
  ncplane_putstr_yx(n, 2, 0, "\xe5\xbd\xa2\xe5\x85\xa8");
  ncplane_putstr_yx(n, 3, 0, "\xe5\xbd\xa2\xe5\x85\xa8");
  ncplane_putstr_yx(n, 4, 0, "abcdef");
  ncplane_putstr_yx(n, 5, 0, "abcdef");
  ncplane_putstr_yx(n, 6, 0, "abcdef");
  ncplane_putstr_yx(n, 7, 0, "abcdef");
  notcurses_render(nc);
  sleep(1);

  stomper(nc, nn);
  if(ncplane_putstr_yx(nn, 0, 0, "\xe5\xbd\xa1") <= 0){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  stomper(nc, nn);

  notcurses_stop(nc);
  return EXIT_SUCCESS;
}

#include <cstdlib>
#include <clocale>
#include <notcurses.h>

int tabletfxn(struct ncplane* p, int begx, int begy, int maxx, int maxy,
              bool cliptop, void* curry){
  uint64_t channels = 0;
  channels_set_fg(&channels, 0x008040);
  ncplane_cursor_move_yx(p, 1, 1);
  ncplane_rounded_box(p, 0, channels, 3, 3, 0);
  return 3;
}

int main(void){
  if(setlocale(LC_ALL, "") == nullptr){
    return EXIT_FAILURE;
  }
  struct notcurses_options opts{};
  struct notcurses* nc = notcurses_init(&opts, stdout);
  if(!nc){
    return EXIT_FAILURE;
  }
  struct ncplane* nstd = notcurses_stdplane(nc);
  int dimy, dimx;
  ncplane_dim_yx(nstd, &dimy, &dimx);
  struct ncplane* n = notcurses_newplane(nc, dimy - 1, dimx, 1, 0, nullptr);
  if(!n){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  if(ncplane_set_fg(nstd, 0xb11bb1)){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  if(ncplane_putstr_aligned(nstd, 0, "(a)dd (q)uit", NCALIGN_CENTER) <= 0){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  struct panelreel_options popts{};
  struct panelreel* pr = panelreel_create(n, &popts, -1);
  if(!pr || notcurses_render(nc)){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  char32_t key;
  while((key = notcurses_getc_blocking(nc)) != (char32_t)-1){
    switch(key){
      case 'q':
        return notcurses_stop(nc) ? EXIT_FAILURE : EXIT_SUCCESS;
      case 'a':
        panelreel_add(pr, nullptr, nullptr, tabletfxn, nullptr);
        break;
      default:
        break;
    }
    if(notcurses_render(nc)){
      break;
    }
  }
  notcurses_stop(nc);
  return EXIT_FAILURE;
}

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
  struct panelreel_options popts{};
  struct panelreel* pr = panelreel_create(notcurses_stdplane(nc), &popts, -1);
  if(!pr){
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

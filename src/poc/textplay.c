#include <stdlib.h>
#include <notcurses/notcurses.h>

static const uint32_t LOWCOLOR = 0x004080;
static const uint32_t HICOLOR = 0x00ff80;
static const uint32_t NANOSEC = 1000000000ull / 600; // 600 cps
#define MARGIN 4

static struct notcurses*
init(void){
  struct notcurses_options opts = {
    .margin_t = MARGIN,
    .margin_r = MARGIN,
    .margin_b = MARGIN,
    .margin_l = MARGIN,
  };
  struct notcurses* nc = notcurses_init(&opts, stdout);
  return nc;
}

static int
colorize(struct ncplane* n){
  int y, x;
  ncplane_cursor_yx(n, &y, &x);
  uint32_t c = HICOLOR;
  while(y >= 0){
    while(x >= 0){
      uint16_t stylemask;
      uint64_t channels;
      char* egc = ncplane_at_yx(n, y, x, &stylemask, &channels);
      if(egc == NULL){
        return -1;
      }
      ncplane_set_fg_rgb(n, c);
      if(ncplane_putegc_yx(n, y, x, egc, NULL) < 0){
        free(egc);
        return -1;
      }
      if(c != LOWCOLOR){
        unsigned g = (c & 0xff00) >> 8u;
        --g;
        c = (c & 0xff00ff) | (g << 8u);
      }
      free(egc);
      --x;
    }
    --y;
    x = ncplane_dim_x(n) - 1;
  }
  return 0;
}

static int
textplay(struct notcurses* nc){
  char* buf = NULL;
  size_t buflen = 1;
  int c;
  struct ncplane* stdn = notcurses_stdplane(nc);
  ncplane_set_scrolling(stdn, true);
  while((c = getc(stdin)) != EOF){
    char* tmp = realloc(buf, buflen + 1);
    if(tmp == NULL){
      free(buf);
      return -1;
    }
    buf = tmp;
    buf[buflen - 1] = c;
    buf[buflen++] = '\0';
    ncplane_home(stdn);
    int pt = ncplane_puttext(stdn, 0, NCALIGN_LEFT, buf, NULL);
    if(pt < 0){
      free(buf);
      return -1;
    }
    if(colorize(stdn)){
      free(buf);
      return -1;
    }
    if(notcurses_render(nc)){
      free(buf);
      return -1;
    }
    struct timespec ts = {
      .tv_sec = 0, .tv_nsec = NANOSEC,
    };
    clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);
  }
  return 0;
}

int main(void){
  struct notcurses* nc = init();
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  textplay(nc);
  if(notcurses_stop(nc)){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

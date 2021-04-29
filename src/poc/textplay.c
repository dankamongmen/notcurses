#include <stdlib.h>
#include <notcurses/notcurses.h>

static const uint32_t LOWCOLOR = 0x004080;
static const uint32_t HICOLOR = 0x00ff80;
static const uint32_t NANOSEC = 1000000000ull / 60; // 60 cps
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
  ncplane_set_bg_rgb(n, 0x222222);
  while(y >= 0){
    while(x >= 0){
      uint16_t stylemask;
      uint64_t channels;
      char* egc = ncplane_at_yx(n, y, x, &stylemask, &channels);
      if(egc == NULL){
        return -1;
      }
      if(strcmp(egc, "") == 0){
        free(egc);
        --x;
        continue;
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
textplay(struct notcurses* nc, struct ncplane* tplane, struct ncvisual* ncv){
  char* buf = NULL;
  size_t buflen = 1;
  int c;
  struct ncplane* stdn = notcurses_stdplane(nc);
  ncplane_set_scrolling(tplane, true);
  struct ncvisual_options vopts = {
    .n = stdn,
    .scaling = NCSCALE_STRETCH,
    .blitter = NCBLIT_PIXEL,
  };
  while((c = getc(stdin)) != EOF){
    if(ncv){
      if(ncvisual_render(nc, ncv, &vopts) == NULL){
        return -1;
      }
    }
    ncplane_erase(tplane);
    char* tmp = realloc(buf, buflen + 1);
    if(tmp == NULL){
      free(buf);
      return -1;
    }
    buf = tmp;
    buf[buflen - 1] = c;
    buf[buflen++] = '\0';
    int pt = ncplane_puttext(tplane, 0, NCALIGN_LEFT, buf, NULL);
    if(pt < 0){
      free(buf);
      return -1;
    }
    if(colorize(tplane)){
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
    if(!ncv){
      clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);
    }else{
      ncvisual_decode(ncv);
    }
  }
  return 0;
}

static struct ncplane*
textplane(struct notcurses* nc){
  int dimy, dimx;
  struct ncplane* stdn = notcurses_stddim_yx(nc, &dimy, &dimx);
  struct ncplane_options nopts = {
    .y = MARGIN,
    .x = MARGIN * 2,
    .rows = dimy - MARGIN * 2,
    .cols = dimx - MARGIN * 4,
    .name = "text",
  };
  struct ncplane* n = ncplane_create(stdn, &nopts);
  uint64_t channels = 0;
  ncchannels_set_fg_alpha(&channels, CELL_ALPHA_TRANSPARENT);
  ncchannels_set_bg_alpha(&channels, CELL_ALPHA_TRANSPARENT);
  if(n){
    ncplane_set_base(n, "", 0, channels);
  }
  return n;
}

static void
usage(FILE* fp){
  fprintf(fp, "usage: textplay [ media ]\n");
}

int main(int argc, char** argv){
  const char* media = NULL;
  if(argc > 3){
    usage(stderr);
    return EXIT_FAILURE;
  }else if(argc == 2){
    media = argv[1];
  }
  struct notcurses* nc = init();
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  struct ncvisual* ncv = NULL;
  if(media){
    notcurses_check_pixel_support(nc);
    if((ncv = ncvisual_from_file(media)) == NULL){
      notcurses_stop(nc);
      return EXIT_FAILURE;
    }
  }
  struct ncplane* tplane = textplane(nc);
  if(tplane == NULL){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  textplay(nc, tplane, ncv);
  ncvisual_destroy(ncv);
  if(notcurses_stop(nc)){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

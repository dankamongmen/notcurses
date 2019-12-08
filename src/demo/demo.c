#include <time.h>
#include <wchar.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <notcurses.h>
#include "demo.h"

int timespec_subtract(struct timespec *result, const struct timespec *time0,
                      struct timespec *time1){
  if(time0->tv_nsec < time1->tv_nsec){
    int nsec = (time1->tv_nsec - time0->tv_nsec) / 1000000000 + 1;
    time1->tv_nsec -= 1000000000 * nsec;
    time1->tv_sec += nsec;
  }
  if(time0->tv_nsec - time1->tv_nsec > 1000000000){
    int nsec = (time0->tv_nsec - time1->tv_nsec) / 1000000000;
    time1->tv_nsec += 1000000000 * nsec;
    time1->tv_sec -= nsec;
  }
  result->tv_sec = time0->tv_sec - time1->tv_sec;
  result->tv_nsec = time0->tv_nsec - time1->tv_nsec;
  return time0->tv_sec < time1->tv_sec;
}

struct timespec demodelay = {
  .tv_sec = 1,
  .tv_nsec = 0,
};

static void
usage(const char* exe, int status){
  FILE* out = status == EXIT_SUCCESS ? stdout : stderr;
  fprintf(out, "usage: %s [ -h ] [ -k ] [ -d mult ] [ -f renderfile ] demospec\n", exe);
  fprintf(out, " -h: this message\n");
  fprintf(out, " -k: keep screen; do not switch to alternate\n");
  fprintf(out, " -d: delay multiplier (float)\n");
  fprintf(out, " -f: render to file in addition to stdout\n");
  fprintf(out, "all demos are run if no specification is provided\n");
  fprintf(out, " b: run box\n");
  fprintf(out, " g: run grid\n");
  fprintf(out, " i: run intro\n");
  fprintf(out, " m: run maxcolor\n");
  fprintf(out, " o: run outro\n");
  fprintf(out, " p: run panelreels\n");
  fprintf(out, " s: run shuffle\n");
  fprintf(out, " u: run uniblock\n");
  fprintf(out, " v: run view\n");
  fprintf(out, " w: run widecolors\n");
  exit(status);
}

static int
outro_message(struct notcurses* nc, int rows, int cols){
  const char str0[] = " ATL, baby! ATL! ";
  const char str1[] = " much, much more is coming ";
  const char str2[] = " hack on! —dank❤ ";
  struct ncplane* on = notcurses_newplane(nc, 5, strlen(str1) + 4, rows - 6,
                                         (cols - (strlen(str1) + 4)) / 2, NULL);
  if(on == NULL){
    return -1;
  }
  cell bgcell = CELL_TRIVIAL_INITIALIZER;
  notcurses_bg_prep(&bgcell.channels, 0x58, 0x36, 0x58);
  ncplane_set_background(on, &bgcell);
  ncplane_dim_yx(on, &rows, &cols);
  int ybase = 0;
  // bevel the upper corners
  if(ncplane_cursor_move_yx(on, ybase, 0) || ncplane_putsimple(on, ' ', 0, 0) < 0){
    return -1;
  }
  if(ncplane_cursor_move_yx(on, ybase, cols - 1) || ncplane_putsimple(on, ' ', 0, 0) < 0){
    return -1;
  }
  // ...and now the lower corners
  if(ncplane_cursor_move_yx(on, rows - 1, 0) || ncplane_putsimple(on, ' ', 0, 0) < 0){
    return -1;
  }
  if(ncplane_cursor_move_yx(on, rows - 1, cols - 1) || ncplane_putsimple(on, ' ', 0, 0) < 0){
    return -1;
  }
  if(ncplane_set_fg_rgb(on, 0, 0, 0)){
    return -1;
  }
  if(ncplane_set_bg_rgb(on, 0, 180, 180)){
    return -1;
  }
  if(ncplane_cursor_move_yx(on, ++ybase, (cols - strlen(str0)) / 2)){
    return -1;
  }
  if(ncplane_putstr(on, str0) < 0){
    return -1;
  }
  if(ncplane_cursor_move_yx(on, ++ybase, (cols - strlen(str1)) / 2)){
    return -1;
  }
  if(ncplane_putstr(on, str1) < 0){
    return -1;
  }
  if(ncplane_cursor_move_yx(on, ++ybase, (cols - (strlen(str2) - 4)) / 2)){
    return -1;
  }
  if(ncplane_putstr(on, str2) < 0){
    return -1;
  }
  if(notcurses_render(nc)){
    return -1;
  }
  cell_release(on, &bgcell);
  return 0;
}

static int
outro(struct notcurses* nc){
  struct ncplane* ncp;
  if((ncp = notcurses_stdplane(nc)) == NULL){
    return -1;
  }
  int rows, cols;
  ncplane_erase(ncp);
  ncplane_dim_yx(ncp, &rows, &cols);
  int averr = 0;
  struct ncvisual* ncv = ncplane_visual_open(ncp, "../tests/changes.jpg", &averr);
  if(ncv == NULL){
    return -1;
  }
  if(ncvisual_decode(ncv, &averr) == NULL){
    ncvisual_destroy(ncv);
    return -1;
  }
  if(ncvisual_render(ncv)){
    ncvisual_destroy(ncv);
    return -1;
  }
  int ret = outro_message(nc, rows, cols);
  if(ret == 0){
    struct timespec fade = { .tv_sec = 5, .tv_nsec = 0, };
    ret |= ncplane_fadeout(ncp, &fade);
  }
  ncvisual_destroy(ncv);
  return ret;
}

static int
intro(struct notcurses* nc){
  struct ncplane* ncp;
  if((ncp = notcurses_stdplane(nc)) == NULL){
    return -1;
  }
  ncplane_erase(ncp);
  int x, y, rows, cols;
  ncplane_dim_yx(ncp, &rows, &cols);
  cell c;
  cell_init(&c);
  const char* cstr = "Δ";
  cell_load(ncp, &c, cstr);
  cell_set_fg(&c, 200, 0, 200);
  int ys = 200 / (rows - 2);
  for(y = 5 ; y < rows - 6 ; ++y){
    cell_set_bg(&c, 0, y * ys  , 0);
    for(x = 5 ; x < cols - 6 ; ++x){
      if(ncplane_cursor_move_yx(ncp, y, x)){
        return -1;
      }
      if(ncplane_putc(ncp, &c) <= 0){
        return -1;
      }
    }
  }
  cell_release(ncp, &c);
  uint64_t channels = 0;
  notcurses_fg_prep(&channels, 90, 0, 90);
  notcurses_bg_prep(&channels, 0, 0, 180);
  if(ncplane_cursor_move_yx(ncp, 4, 4)){
    return -1;
  }
  if(ncplane_rounded_box(ncp, 0, channels, rows - 6, cols - 6, 0)){
    return -1;
  }
  const char s1[] = " Die Welt ist alles, was der Fall ist. ";
  const char str[] = " Wovon man nicht sprechen kann, darüber muss man schweigen. ";
  if(ncplane_set_fg_rgb(ncp, 192, 192, 192)){
    return -1;
  }
  if(ncplane_set_bg_rgb(ncp, 0, 40, 0)){
    return -1;
  }
  if(ncplane_cursor_move_yx(ncp, rows / 2 - 2, (cols - strlen(s1) + 4) / 2)){
    return -1;
  }
  if(ncplane_putstr(ncp, s1) != (int)strlen(s1)){
    return -1;
  }
  if(ncplane_cursor_move_yx(ncp, rows / 2, (cols - strlen(str) + 4) / 2)){
    return -1;
  }
  ncplane_styles_on(ncp, CELL_STYLE_ITALIC);
  if(ncplane_putstr(ncp, str) != (int)strlen(str)){
    return -1;
  }
  ncplane_styles_off(ncp, CELL_STYLE_ITALIC);
  const wchar_t wstr[] = L"▏▁ ▂ ▃ ▄ ▅ ▆ ▇ █ █ ▇ ▆ ▅ ▄ ▃ ▂ ▁▕";
  char mbstr[128];
  if(wcstombs(mbstr, wstr, sizeof(mbstr)) <= 0){
    return -1;
  }
  if(ncplane_cursor_move_yx(ncp, rows / 2 - 5, (cols - wcslen(wstr) + 4) / 2)){
    return -1;
  }
  if(ncplane_putstr(ncp, mbstr) != (int)strlen(mbstr)){
    return -1;
  }
  if(notcurses_render(nc)){
    return -1;
  }
  nanosleep(&demodelay, NULL);
  struct timespec fade = demodelay;
  ncplane_fadeout(ncp, &fade);
  return 0;
}

static int
ext_demos(struct notcurses* nc, const char* demos){
  while(*demos){
    int ret = 0;
    switch(*demos){
      case 'i': ret = intro(nc); break;
      case 'o': ret = outro(nc); break;
      case 's': ret = sliding_puzzle_demo(nc); break;
      case 'u': ret = unicodeblocks_demo(nc); break;
      case 'm': ret = maxcolor_demo(nc); break;
      case 'b': ret = box_demo(nc); break;
      case 'g': ret = grid_demo(nc); break;
      case 'v': ret = view_demo(nc); break;
      case 'w': ret = widecolor_demo(nc); break;
      case 'p': ret = panelreel_demo(nc); break;
      default:
        fprintf(stderr, "Unknown demo specification: %c\n", *demos);
        ret = -1;
        break;
    }
    if(ret){
      return ret;
    }
    ++demos;
  }
  return 0;
}

// returns the demos to be run as a string. on error, returns NULL. on no
// specification, also returns NULL, heh. determine this by argv[optind];
// if it's NULL, there were valid options, but no spec.
static const char*
handle_opts(int argc, char** argv, notcurses_options* opts){
  int c;
  memset(opts, 0, sizeof(*opts));
  opts->outfp = stdout;
  while((c = getopt(argc, argv, "hkd:f:")) != EOF){
    switch(c){
      case 'h':
        usage(*argv, EXIT_SUCCESS);
        break;
      case 'k':
        opts->inhibit_alternate_screen = true;
        break;
      case 'f':
        if(opts->renderfp){
          fprintf(stderr, "-f may only be supplied once\n");
          usage(*argv, EXIT_FAILURE);
        }
        if((opts->renderfp = fopen(optarg, "wb")) == NULL){
          usage(*argv, EXIT_FAILURE);
        }
        break;
      case 'd':{
        float f;
        if(sscanf(optarg, "%f", &f) != 1){
          fprintf(stderr, "Couldn't get a float from %s\n", optarg);
          usage(*argv, EXIT_FAILURE);
        }
        uint64_t ns = f * GIG;
        demodelay.tv_sec = ns / GIG;
        demodelay.tv_nsec = ns % GIG;
        break;
      }default:
        usage(*argv, EXIT_FAILURE);
    }
  }
  const char* demos = argv[optind];
  return demos;
}

// just fucking around...for now
int main(int argc, char** argv){
  srandom(time(NULL)); // a classic blunder
  struct notcurses* nc;
  notcurses_options nopts;
  struct ncplane* ncp;
  if(!setlocale(LC_ALL, "")){
    fprintf(stderr, "Couldn't set locale based on user preferences\n");
    return EXIT_FAILURE;
  }
  const char* demos;
  if((demos = handle_opts(argc, argv, &nopts)) == NULL){
    if(argv[optind] != NULL){
      usage(*argv, EXIT_FAILURE);
    }
    demos = "isumbgwvpo";
  }
  if((nc = notcurses_init(&nopts)) == NULL){
    return EXIT_FAILURE;
  }
  if((ncp = notcurses_stdplane(nc)) == NULL){
    fprintf(stderr, "Couldn't get standard plane\n");
    goto err;
  }
  // no one cares about the leaderscreen. 1s max.
  if(demodelay.tv_sec >= 1){
    sleep(1);
  }else{
    nanosleep(&demodelay, NULL);
  }
  if(ext_demos(nc, demos)){
    goto err;
  }
  if(notcurses_stop(nc)){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;

err:
  notcurses_stop(nc);
  return EXIT_FAILURE;
}

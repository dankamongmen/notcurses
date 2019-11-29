#include <wchar.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <notcurses.h>
#include "demo.h"

struct timespec demodelay = {
  .tv_sec = 1,
  .tv_nsec = 0,
};

static void
usage(const char* exe, int status){
  FILE* out = status == EXIT_SUCCESS ? stdout : stderr;
  fprintf(out, "usage: %s [ -h ] [ -k ] [ -d ns ] [ -f renderfile ] demospec\n", exe);
  fprintf(out, " -h: this message\n");
  fprintf(out, " -k: keep screen; do not switch to alternate\n");
  fprintf(out, " -d: delay in nanoseconds between demos\n");
  fprintf(out, " -f: render to file in addition to stdout\n");
  fprintf(out, "all demos are run if no specification is provided\n");
  fprintf(out, " i: run intro\n");
  fprintf(out, " s: run shuffle\n");
  fprintf(out, " u: run uniblock\n");
  fprintf(out, " m: run maxcolor\n");
  fprintf(out, " b: run box\n");
  fprintf(out, " g: run grid\n");
  fprintf(out, " w: run widecolors\n");
  exit(status);
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
      if(ncplane_putc(ncp, &c) != (int)strlen(cstr)){
        return -1;
      }
    }
  }
  cell_release(ncp, &c);
  cell ul = CELL_TRIVIAL_INITIALIZER;
  cell ur = CELL_TRIVIAL_INITIALIZER;
  cell ll = CELL_TRIVIAL_INITIALIZER;
  cell lr = CELL_TRIVIAL_INITIALIZER;
  cell vl = CELL_TRIVIAL_INITIALIZER;
  cell hl = CELL_TRIVIAL_INITIALIZER;
  if(ncplane_rounded_box_cells(ncp, &ul, &ur, &ll, &lr, &hl, &vl)){
    return -1;
  }
  cell_set_fg(&ul, 90, 0, 90);
  cell_set_fg(&ur, 90, 0, 90);
  cell_set_fg(&ll, 90, 0, 90);
  cell_set_fg(&lr, 90, 0, 90);
  cell_set_fg(&vl, 90, 0, 90);
  cell_set_fg(&hl, 90, 0, 90);
  cell_set_bg(&ul, 0, 0, 180);
  cell_set_bg(&ur, 0, 0, 180);
  cell_set_bg(&ll, 0, 0, 180);
  cell_set_bg(&lr, 0, 0, 180);
  cell_set_bg(&vl, 0, 0, 180);
  cell_set_bg(&hl, 0, 0, 180);
  if(ncplane_cursor_move_yx(ncp, 4, 4)){
    return -1;
  }
  if(ncplane_box(ncp, &ul, &ur, &ll, &lr, &hl, &vl, rows - 6, cols - 6)){
    return -1;
  }
  const char s1[] = " Die Welt ist alles, was der Fall ist. ";
  const char str[] = " Wovon man nicht sprechen kann, darüber muss man schweigen. ";
  if(ncplane_fg_rgb8(ncp, 192, 192, 192)){
    return -1;
  }
  if(ncplane_bg_rgb8(ncp, 0, 40, 0)){
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
  return 0;
}

static int
ext_demos(struct notcurses* nc, const char* demos){
  while(*demos){
    int ret = 0;
    switch(*demos){
      case 'i': ret = intro(nc); break;
      case 's': ret = sliding_puzzle_demo(nc); break;
      case 'u': ret = unicodeblocks_demo(nc); break;
      case 'm': ret = maxcolor_demo(nc); break;
      case 'b': ret = box_demo(nc); break;
      case 'g': ret = grid_demo(nc); break;
      case 'w': ret = widecolor_demo(nc); break;
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
        char* eptr;
        unsigned long ns = strtoul(optarg, &eptr, 0);
        if(*eptr){
          usage(*argv, EXIT_FAILURE);
        }
        demodelay.tv_sec = ns / 1000000000;
        demodelay.tv_nsec = ns % 1000000000;
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
    demos = "isumbgw";
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

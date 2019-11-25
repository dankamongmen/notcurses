#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <notcurses.h>
#include "demo.h"

static void
usage(const char* exe, int status){
  FILE* out = status == EXIT_SUCCESS ? stdout : stderr;
  fprintf(out, "usage: %s [ -h | -k ]\n", exe);
  fprintf(out, " h: this message\n");
  fprintf(out, " k: keep screen; do not switch to alternate\n");
  exit(status);
}

static int
handle_opts(int argc, char** argv, notcurses_options* opts){
  int c;
  memset(opts, 0, sizeof(*opts));
  opts->outfd = STDOUT_FILENO;
  while((c = getopt(argc, argv, "hk")) != EOF){
    switch(c){
      case 'h':
        usage(*argv, EXIT_SUCCESS);
        break;
      case 'k':
        opts->inhibit_alternate_screen = true;
        break;
      default:
        usage(*argv, EXIT_FAILURE);
    }
  }
  return 0;
}

// just fucking around...for now
int main(int argc, char** argv){
  struct notcurses* nc;
  notcurses_options nopts;
  struct ncplane* ncp;
  if(!setlocale(LC_ALL, "")){
    fprintf(stderr, "Couldn't set locale based on user preferences\n");
    return EXIT_FAILURE;
  }
  if(handle_opts(argc, argv, &nopts)){
    return EXIT_FAILURE;
  }
  if((nc = notcurses_init(&nopts)) == NULL){
    return EXIT_FAILURE;
  }
  if((ncp = notcurses_stdplane(nc)) == NULL){
    fprintf(stderr, "Couldn't get standard plane\n");
    goto err;
  }
  sleep(1);
  int x, y, rows, cols;
  ncplane_dimyx(ncp, &rows, &cols);
  cell c;
  memset(&c, 0, sizeof(c));
  const char* cstr = "✓";
  cell_load(ncp, &c, cstr);
  cell_set_fg(&c, 200, 0, 200);
  for(y = 2 ; y < rows - 2 ; ++y){
    if(ncplane_cursor_move_yx(ncp, y, 2)){
      goto err;
    }
    for(x = 2 ; x < cols - 2 ; ++x){
      if(ncplane_putc(ncp, &c) != (int)strlen(cstr)){
        goto err;
      }
    }
  }
  cell_release(ncp, &c);
  if(notcurses_render(nc)){
    goto err;
  }
  sleep(1);
  const char str[] = " Wovon man nicht sprechen kann, darüber muss man schweigen. ";
  if(ncplane_cursor_move_yx(ncp, y / 2, (x - strlen(str) + 4) / 2)){
    goto err;
  }
  if(ncplane_fg_rgb8(ncp, 176, 121, 176)){
    goto err;
  }
  if(ncplane_bg_rgb8(ncp, 255, 255, 255)){
    goto err;
  }
  if(ncplane_putstr(ncp, str) != (int)strlen(str)){
    goto err;
  }
  if(notcurses_render(nc)){
    goto err;
  }
  sleep(1);
  if(grid_demo(nc)){
    goto err;
  }
  if(box_demo(nc)){
    goto err;
  }
  if(widecolor_demo(nc, ncp)){
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

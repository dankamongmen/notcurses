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
ext_demos(struct notcurses* nc){
  if(unicodeblocks_demo(nc)){
    return -1;
  }
  if(maxcolor_demo(nc)){
    return -1;
  }
  if(box_demo(nc)){
    return -1;
  }
  if(grid_demo(nc)){
    return -1;
  }
  if(widecolor_demo(nc)){
    return -1;
  }
  return 0;
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
  cell_init(&c);
  const char* cstr = "Δ";
  cell_load(ncp, &c, cstr);
  cell_set_fg(&c, 200, 0, 200);
  int ys = 200 / (rows - 2);
  for(y = 2 ; y < rows - 2 ; ++y){
    cell_set_bg(&c, 0, y * ys  , 0);
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
  cell ul = CELL_TRIVIAL_INITIALIZER;
  cell ur = CELL_TRIVIAL_INITIALIZER;
  cell ll = CELL_TRIVIAL_INITIALIZER;
  cell lr = CELL_TRIVIAL_INITIALIZER;
  cell vl = CELL_TRIVIAL_INITIALIZER;
  cell hl = CELL_TRIVIAL_INITIALIZER;
  cell_load(ncp, &ul, "╭");
  cell_load(ncp, &ur, "╮");
  cell_load(ncp, &ll, "╰");
  cell_load(ncp, &lr, "╯");
  cell_load(ncp, &vl, "│");
  cell_load(ncp, &hl, "─");
  if(ncplane_cursor_move_yx(ncp, 1, 1)){
    goto err;
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
  if(ncplane_box(ncp, &ul, &ur, &ll, &lr, &hl, &vl, rows - 1, cols - 2)){
    goto err;
  }
  if(notcurses_render(nc)){
    goto err;
  }
  sleep(1);
  const char s1[] = " Die Welt ist alles, was der Fall ist. ";
  const char str[] = " Wovon man nicht sprechen kann, darüber muss man schweigen. ";
  if(ncplane_fg_rgb8(ncp, 192, 192, 192)){
    goto err;
  }
  if(ncplane_bg_rgb8(ncp, 0, 40, 0)){
    goto err;
  }
  if(ncplane_cursor_move_yx(ncp, rows / 2 - 2, (cols - strlen(s1) + 4) / 2)){
    goto err;
  }
  if(ncplane_putstr(ncp, s1) != (int)strlen(s1)){
    goto err;
  }
  if(ncplane_cursor_move_yx(ncp, rows / 2, (cols - strlen(str) + 4) / 2)){
    goto err;
  }
  if(ncplane_putstr(ncp, str) != (int)strlen(str)){
    goto err;
  }
  const char bstr[] = "▏▁ ▂ ▃ ▄ ▅ ▆ ▇ █ █ ▇ ▆ ▅ ▄ ▃ ▂ ▁▕";
  if(ncplane_cursor_move_yx(ncp, rows / 2 + 3, (cols - strlen(bstr) + 4) / 2)){
    goto err;
  }
  if(ncplane_putstr(ncp, bstr) != (int)strlen(bstr)){
    goto err;
  }
  if(notcurses_render(nc)){
    goto err;
  }
  sleep(1);
  if(ext_demos(nc)){
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

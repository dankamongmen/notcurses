#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <unistd.h>
#include <stdlib.h>
#include <notcurses.h>

// just fucking around...for now
int main(void){
  struct notcurses* nc;
  notcurses_options nopts = {
    .inhibit_alternate_screen = false,
    .outfd = STDOUT_FILENO,
    .termtype = NULL,
  };
  struct ncplane* ncp;
  if(!setlocale(LC_ALL, "")){
    fprintf(stderr, "Couldn't set locale based on user preferences\n");
    return EXIT_FAILURE;
  }
  if((nc = notcurses_init(&nopts)) == NULL){
    return EXIT_FAILURE;
  }
  if((ncp = notcurses_stdplane(nc)) == NULL){
    fprintf(stderr, "Couldn't get standard plane\n");
    goto err;
  }
  int x, y, rows, cols;
  ncplane_dimyx(ncp, &rows, &cols);
  cell c;
  const char* cstr = "✓";
  cell_load(ncp, &c, cstr);
  cell_set_fg(&c, 200, 0, 200);
  for(y = 1 ; y < rows - 1 ; ++y){
    if(ncplane_cursor_move_yx(ncp, y, 1)){
      goto err;
    }
    for(x = 1 ; x < cols - 1 ; ++x){
      if(ncplane_putc(ncp, &c, cstr) != (int)strlen(cstr)){
        goto err;
      }
    }
  }
  if(notcurses_render(nc)){
    goto err;
  }
  sleep(1);
  const char str[] = " Wovon man nicht sprechen kann, darüber muss man schweigen. ";
  if(ncplane_cursor_move_yx(ncp, y / 2, (x - strlen(str)) / 2)){
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
  if(notcurses_stop(nc)){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;

err:
  notcurses_stop(nc);
  return EXIT_FAILURE;
}

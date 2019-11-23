#include <stdio.h>
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
  load_cell(&c, /*L"💣*/L"X");
  cell_set_fg(&c, 200, 0, 200);
  for(y = 1 ; y < rows - 1 ; ++y){
    if(ncplane_cursor_move_yx(ncp, y, 1)){
      goto err;
    }
    for(x = 1 ; x < cols - 1 ; ++x){
      if(ncplane_putwc(ncp, &c)){
        goto err;
      }
    }
  }
  if(notcurses_render(nc)){
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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <notcurses.h>

int main(void){
  struct notcurses* nc;
  notcurses_options nopts = {
    .inhibit_alternate_screen = false,
    .outfd = STDOUT_FILENO,
    .termtype = NULL,
  };
  struct ncplane* ncp;
  if((nc = notcurses_init(&nopts)) == NULL){
    return EXIT_FAILURE;
  }
  if((ncp = notcurses_stdplane(nc)) == NULL){
    fprintf(stderr, "Couldn't get standard plane\n");
    goto err;
  }
  if(notcurses_fg_rgb8(nc, 200, 0, 200)){
    goto err;
  }
  if(notcurses_render(nc)){
    goto err;
  }
  if(ncplane_move(ncp, 1, 1)){
    goto err;
  }
  if(notcurses_render(nc)){
    goto err;
  }
  sleep(5);
  if(notcurses_stop(nc)){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;

err:
  notcurses_stop(nc);
  return EXIT_FAILURE;
}

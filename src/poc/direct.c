#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <sys/ioctl.h>
#include <notcurses.h>

// can we leave what was already on the screen there? (narrator: it seems not)
int main(void){
  if(!setlocale(LC_ALL, "")){
    fprintf(stderr, "Couldn't set locale\n");
    return EXIT_FAILURE;
  }
  struct winsize geom;
  if(ioctl(fileno(stdout), TIOCGWINSZ, &geom)){
    fprintf(stderr, "Couldn't get terminal geometry (%s)\n", strerror(errno));
    return EXIT_FAILURE;
  }
  for(int y = 0 ; y < geom.ws_row ; ++y){
    for(int x = 0 ; x < geom.ws_col ; ++x){
      printf("X");
    }
  }
  fflush(stdout);
  notcurses_options opts;
  memset(&opts, 0, sizeof(opts));
  opts.inhibit_alternate_screen = true;
  struct notcurses* nc = notcurses_init(&opts, stdout);
  if(!nc){
    fprintf(stderr, "Couldn't initialize notcurses\n");
    return EXIT_FAILURE;
  }
  ncplane_set_fg(notcurses_stdplane(nc), 0x00ff00);
  if(ncplane_putstr_aligned(notcurses_stdplane(nc), geom.ws_row - 2, NCALIGN_CENTER, " erperperp ") <= 0){
    notcurses_stop(nc);
    fprintf(stderr, "Error printing\n");
    return EXIT_FAILURE;
  }
  if(notcurses_render(nc)){
    notcurses_stop(nc);
    fprintf(stderr, "Error rendering\n");
    return EXIT_FAILURE;
  }
  sleep(2);
  if(notcurses_stop(nc)){
    fprintf(stderr, "Error stopping notcurses\n");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

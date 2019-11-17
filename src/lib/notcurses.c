#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include "notcurses.h"
#include "version.h"

typedef struct notcurses {
  int ttyfd;  // file descriptor for controlling tty (takes stdin)
} notcurses;

static const char NOTCURSES_VERSION[] =
 notcurses_VERSION_MAJOR "."
 notcurses_VERSION_MINOR "."
 notcurses_VERSION_PATCH;

const char* notcurses_version(void){
  return NOTCURSES_VERSION;
}

int notcurses_term_dimensions(notcurses* n, int* rows, int* cols){
  struct winsize ws;
  int i = ioctl(n->ttyfd, TIOCGWINSZ, &ws);
  if(i < 0){
    fprintf(stderr, "TIOCGWINSZ failed on %d (%s)\n", n->ttyfd, strerror(errno));
    return -1;
  }
  *rows = ws.ws_row;
  *cols = ws.ws_col;
  return 0;
}

notcurses* notcurses_init(void){
  notcurses* ret = malloc(sizeof(*ret));
  if(ret == NULL){
    return ret;
  }
  // FIXME should we maybe use stdout if stdin was redirected?
  ret->ttyfd = STDIN_FILENO;
  return ret;
}

int notcurses_stop(notcurses* nc){
  int ret = 0;
  if(nc){
    /*if(close(nc->ttyfd)){
      fprintf(stderr, "Error closing TTY file descriptor %d (%s)\n",
              nc->ttyfd, strerror(errno));
      ret = -1;
    }*/
    free(nc);
  }
  return ret;
}

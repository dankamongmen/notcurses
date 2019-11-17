#include <curses.h> // needed for some definitions, see terminfo(3ncurses)
#include <term.h>
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
  int colors;
  char* smcup;  // enter alternate mode
  char* rmcup;  // restore primary mode
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

static int
term_get_seq(char** seq, const char* name){
  *seq = tigetstr(name);
  if(*seq == NULL || *seq == (char*)-1){
    fprintf(stderr, "Capability not defined for terminal: %s\n", name);
    return -1;
  }
  return 0;
}

static int
term_emit(const char* seq){
  int ret = printf("%s", seq);
  if(ret < 0){
    fprintf(stderr, "Error emitting %zub sequence (%s)\n", strlen(seq), strerror(errno));
    return -1;
  }
  if((size_t)ret != strlen(seq)){
    fprintf(stderr, "Short write (%db) for %zub sequence\n", ret, strlen(seq));
    return -1;
  }
  return 0;
}

notcurses* notcurses_init(void){
  notcurses* ret = malloc(sizeof(*ret));
  if(ret == NULL){
    return ret;
  }
  int termerr;
  if(setupterm(NULL, STDERR_FILENO, &termerr) != OK){
    fprintf(stderr, "Terminfo error %d (see terminfo(3ncurses))\n", termerr);
    free(ret);
    return NULL;
  }
  if((ret->colors = tigetnum("colors")) <= 0){
    fprintf(stderr, "This terminal doesn't appear to support colors\n");
    free(ret);
    return NULL;
  }
  printf("Colors: %d\n", ret->colors);
  int fails = 0;
  fails |= term_get_seq(&ret->smcup, "smcup");
  fails |= term_get_seq(&ret->rmcup, "rmcup");
  if(fails){
    free(ret);
    return NULL;
  }
  // FIXME should we maybe use stdout if stdin was redirected?
  ret->ttyfd = STDIN_FILENO;
  if(term_emit(ret->smcup)){
    free(ret);
    return NULL;
  }
  return ret;
}

int notcurses_stop(notcurses* nc){
  int ret = 0;
  if(nc){
    ret |= term_emit(nc->rmcup);
    free(nc);
  }
  return ret;
}

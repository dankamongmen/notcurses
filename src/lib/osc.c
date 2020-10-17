#include "internal.h"

// use the OSC escapes

char* notcurses_title(const notcurses* nc){
fprintf(stderr, "TTYFD: %d\n", nc->ttyfd);
  if(nc->ttyfd < 0){
    return NULL;
  }
  const char GETTITLE[] = "\x1b[21t";
  if(tty_emit("gettitle", GETTITLE, nc->ttyfd)){
fprintf(stderr, "WE FAILED!\n");
    return NULL;
  }
  char c;
  ssize_t r;
  while((r = read(nc->ttyfd, &c, 1)) == 1 || errno == EAGAIN){
    if(r == 1){
      fprintf(stderr, "GOT %c\n", c);
    }
  }
  fprintf(stderr, "ERROR: %s\n", strerror(errno)); 
  return NULL; // FIXME read it! through newline
}

int notcurses_set_title(notcurses* nc, const char* title){
  return 0;
}

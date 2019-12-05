#include <sys/poll.h>
#include "internal.h"

sig_atomic_t resize_seen = 0;

static int
handle_getc(const notcurses* nc __attribute__ ((unused)), cell* c, int kpress,
            ncspecial_key* special){
// fprintf(stderr, "KEYPRESS: %d\n", kpress);
  if(kpress < 0){
    return -1;
  }
  *special = 0;
  if(kpress == 0x04){ // ctrl-d
    return -1;
  }
  // FIXME look for keypad
  if(kpress < 0x80){
    c->gcluster = kpress;
  }else{
    // FIXME
  }
  return 1;
}

int notcurses_getc(const notcurses* nc, cell* c, ncspecial_key* special){
  if(resize_seen){
    resize_seen = 0;
    c->gcluster = 0;
    *special = NCKEY_RESIZE;
    return 1;
  }
  int r = getc(nc->ttyinfp);
  if(r < 0){
    return r;
  }
  return handle_getc(nc, c, r, special);
}

// we set our infd to non-blocking on entry, so to do a blocking call (without
// burning cpu) we'll need to set up a poll().
int notcurses_getc_blocking(const notcurses* nc, cell* c, ncspecial_key* special){
  struct pollfd pfd = {
    .fd = fileno(nc->ttyinfp),
    .events = POLLIN | POLLRDHUP,
    .revents = 0,
  };
  int pret, r;
  sigset_t smask;
  sigfillset(&smask);
  sigdelset(&smask, SIGWINCH);
  while((pret = ppoll(&pfd, 1, NULL, &smask)) >= 0){
    if(pret == 0){
      continue;
    }
    r = getc(nc->ttyinfp);
    if(r < 0){
      break; // want EINTR handling below
    }
    return handle_getc(nc, c, r, special);
  }
  if(errno == EINTR){
    if(resize_seen){
      resize_seen = 0;
      c->gcluster = 0;
      *special = NCKEY_RESIZE;
      return 1;
    }
  }
  return -1;
}

#include <stdio.h>
#include <signal.h>
#include <stdatomic.h>
#include "internal.h"

// only one notcurses object can be the target of signal handlers, due to their
// process-wide nature.
static void* _Atomic signal_nc = ATOMIC_VAR_INIT(NULL); // ugh
static void (*signal_sa_handler)(int); // stashed signal handler we replaced
static int(*fatal_callback)(void*); // fatal handler callback provided

int drop_signals(void* nc){
  void* old = nc;
  if(!atomic_compare_exchange_strong(&signal_nc, &old, NULL)){
    fprintf(stderr, "Couldn't drop signals: %p != %p\n", old, nc);
    return -1;
  }
  return 0;
}

// this wildly unsafe handler will attempt to restore the screen upon receipt
// of SIG{INT, SEGV, ABRT, QUIT, TERM}. godspeed you, black emperor!
static void
fatal_handler(int signo){
  notcurses* nc = atomic_load(&signal_nc);
  if(nc){
    fatal_callback(nc);
    if(signal_sa_handler){
      signal_sa_handler(signo);
    }
    raise(signo);
  }
}

int setup_signals(void* nc, bool no_quit_sigs, bool no_winch_sig,
                  int(*handler)(void*)){
  struct sigaction oldact;
  void* expected = NULL;
  struct sigaction sa;

  if(!atomic_compare_exchange_strong(&signal_nc, &expected, nc)){
    fprintf(stderr, "%p is already registered for signals\n", expected);
    return -1;
  }
  if(!no_winch_sig){
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigwinch_handler;
    if(sigaction(SIGWINCH, &sa, NULL)){
      atomic_store(&signal_nc, NULL);
      fprintf(stderr, "Error installing SIGWINCH handler (%s)\n",
              strerror(errno));
      return -1;
    }
  }
  if(!no_quit_sigs){
    memset(&sa, 0, sizeof(sa));
    fatal_callback = handler;
    sa.sa_handler = fatal_handler;
    sigaddset(&sa.sa_mask, SIGINT);
    sigaddset(&sa.sa_mask, SIGQUIT);
    sigaddset(&sa.sa_mask, SIGSEGV);
    sigaddset(&sa.sa_mask, SIGABRT);
    sigaddset(&sa.sa_mask, SIGTERM);
    sa.sa_flags = SA_RESETHAND; // don't try twice
    int ret = 0;
    ret |= sigaction(SIGINT, &sa, &oldact);
    ret |= sigaction(SIGQUIT, &sa, &oldact);
    ret |= sigaction(SIGSEGV, &sa, &oldact);
    ret |= sigaction(SIGABRT, &sa, &oldact);
    ret |= sigaction(SIGTERM, &sa, &oldact);
    if(ret){
      atomic_store(&signal_nc, NULL);
      fprintf(stderr, "Error installing fatal signal handlers (%s)\n",
              strerror(errno));
      return -1;
    }
    signal_sa_handler = oldact.sa_handler;
  }
  return 0;
}

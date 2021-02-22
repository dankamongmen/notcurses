#include <stdio.h>
#include <signal.h>
#include <stdatomic.h>
#include "internal.h"

// only one notcurses object can be the target of signal handlers, due to their
// process-wide nature. hold this lock over any of the shared data below.
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
// we still need an atomic signal_nc, despite guarding with the mutex, so that
// it can be safely looked up within the signal handler.
static void* _Atomic signal_nc = ATOMIC_VAR_INIT(NULL); // ugh
static bool handling_winch;
static bool handling_fatals;

// saved signal actions, restored in drop_signals() FIXME make an array
static struct sigaction old_winch;
static struct sigaction old_cont;
static struct sigaction old_term;
static struct sigaction old_int;
static struct sigaction old_ill;
static struct sigaction old_quit;
static struct sigaction old_segv;
static struct sigaction old_abrt;
static struct sigaction old_term;

static int(*fatal_callback)(void*); // fatal handler callback

int drop_signals(void* nc){
  int ret = -1;
  void* expected = nc;
  pthread_mutex_lock(&lock);
  if(atomic_compare_exchange_strong(&signal_nc, &expected, nc)){
    if(handling_winch){
      sigaction(SIGWINCH, &old_winch, NULL);
      sigaction(SIGCONT, &old_cont, NULL);
      handling_winch = false;
    }
    if(handling_fatals){
      sigaction(SIGINT, &old_int, NULL);
      sigaction(SIGILL, &old_ill, NULL);
      sigaction(SIGTERM, &old_term, NULL);
      sigaction(SIGSEGV, &old_segv, NULL);
      sigaction(SIGABRT, &old_abrt, NULL);
      sigaction(SIGQUIT, &old_quit, NULL);
      handling_fatals = false;
    }
    ret = !atomic_compare_exchange_strong(&signal_nc, &expected, NULL);
  }
  pthread_mutex_unlock(&lock);
  if(ret){
    fprintf(stderr, "Couldn't drop signals with %p\n", nc);
  }
  return ret;
}

static void
invoke_old(const struct sigaction* old, int signo, siginfo_t* sinfo, void* v){
  if(old->sa_sigaction){
    old->sa_sigaction(signo, sinfo, v);
  }else if(old->sa_handler){
    old->sa_handler(signo);
  }
}

// this wildly unsafe handler will attempt to restore the screen upon receipt
// of SIG{ILL, INT, SEGV, ABRT, QUIT, TERM}. godspeed you, black emperor!
static void
fatal_handler(int signo, siginfo_t* siginfo, void* v){
  notcurses* nc = atomic_load(&signal_nc);
  if(nc){
    fatal_callback(nc);
    switch(signo){
      case SIGTERM: invoke_old(&old_term, signo, siginfo, v); break;
      case SIGQUIT: invoke_old(&old_quit, signo, siginfo, v); break;
      case SIGSEGV: invoke_old(&old_segv, signo, siginfo, v); break;
      case SIGINT: invoke_old(&old_int, signo, siginfo, v); break;
      case SIGILL: invoke_old(&old_ill, signo, siginfo, v); break;
      case SIGABRT: invoke_old(&old_abrt, signo, siginfo, v); break;
    }
    raise(signo); // FIXME does this invoke twice? hrmmm
  }
}

int setup_signals(void* nc, bool no_quit_sigs, bool no_winch_sig,
                  int(*handler)(void*)){
  void* expected = NULL;
  struct sigaction sa;

  if(!atomic_compare_exchange_strong(&signal_nc, &expected, nc)){
    fprintf(stderr, "%p is already registered for signals\n", expected);
    return -1;
  }
  if(!no_winch_sig){
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigwinch_handler;
    sigaddset(&sa.sa_mask, SIGWINCH);
    sigaddset(&sa.sa_mask, SIGCONT);
    int ret = 0;
    ret |= sigaction(SIGWINCH, &sa, &old_winch);
    ret |= sigaction(SIGCONT, &sa, &old_cont);
    if(ret){
      atomic_store(&signal_nc, NULL);
      fprintf(stderr, "Error installing term signal handler (%s)\n",
              strerror(errno));
      return -1;
    }
    handling_winch = true;
  }
  if(!no_quit_sigs){
    memset(&sa, 0, sizeof(sa));
    fatal_callback = handler;
    sa.sa_sigaction = fatal_handler;
    sigaddset(&sa.sa_mask, SIGILL);
    sigaddset(&sa.sa_mask, SIGINT);
    sigaddset(&sa.sa_mask, SIGQUIT);
    sigaddset(&sa.sa_mask, SIGSEGV);
    sigaddset(&sa.sa_mask, SIGABRT);
    sigaddset(&sa.sa_mask, SIGTERM);
    sa.sa_flags = SA_SIGINFO | SA_RESETHAND; // don't try fatal signals twice
    int ret = 0;
    ret |= sigaction(SIGILL, &sa, &old_ill);
    ret |= sigaction(SIGINT, &sa, &old_int);
    ret |= sigaction(SIGQUIT, &sa, &old_quit);
    ret |= sigaction(SIGSEGV, &sa, &old_segv);
    ret |= sigaction(SIGABRT, &sa, &old_abrt);
    ret |= sigaction(SIGTERM, &sa, &old_term);
    if(ret){
      atomic_store(&signal_nc, NULL);
      fprintf(stderr, "Error installing fatal signal handlers (%s)\n",
              strerror(errno));
      return -1;
    }
    handling_fatals = true;
  }
  return 0;
}

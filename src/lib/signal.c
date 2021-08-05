#include <stdio.h>
#include <signal.h>
#include <stdatomic.h>
#include "internal.h"

// primarily drive ownership off an atomic, safely used within a signal handler
static void* _Atomic signal_nc = ATOMIC_VAR_INIT(NULL);

#ifdef __MINGW64__
int drop_signals(void* nc){
  int ret = -1;
  void* expected = nc;
  pthread_mutex_lock(&lock);
  if(atomic_compare_exchange_strong(&signal_nc, &expected, nc)){
    ret = 0;
  }
  pthread_mutex_unlock(&lock);
  return ret;
}

// this both sets up our signal handlers (unless that behavior has been
// inhibited), and ensures that only one notcurses/ncdirect context is active
// at any given time.
int setup_signals(void* vnc, bool no_quit_sigs, bool no_winch_sigs,
                  int(*handler)(void*)){
  (void)no_quit_sigs;
  (void)no_winch_sigs;
  void* expected = NULL;
  // don't register ourselves if we don't intend to set up signal handlers
  // we expect NULL (nothing registered), and want to register nc
  if(!atomic_compare_exchange_strong(&signal_nc, &expected, vnc)){
    loginfo("%p is already registered for signals (provided %p)\n", expected, vnc);
    return -1;
  }
  return ret;
}
#else
// only one notcurses object can be the target of signal handlers, due to their
// process-wide nature. hold this lock over any of the shared data below.
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static bool handling_winch;
static bool handling_fatals;

// saved signal actions, restored in drop_signals() FIXME make an array
static struct sigaction old_winch;
static struct sigaction old_cont;
static struct sigaction old_abrt;
static struct sigaction old_fpe;
static struct sigaction old_ill;
static struct sigaction old_int;
static struct sigaction old_quit;
static struct sigaction old_segv;
static struct sigaction old_term;

// Signals we block when we start writing out a frame, so as not to be
// interrupted in media res (interrupting an escape can lock up a terminal).
// Prepared in setup_signals(), and never changes across our lifetime.
static sigset_t wblock_signals;

static int(*fatal_callback)(void*); // fatal handler callback

int block_signals(sigset_t* old_blocked_signals){
  if(pthread_sigmask(SIG_BLOCK, &wblock_signals, old_blocked_signals)){
    return -1;
  }
  return 0;
}

int unblock_signals(const sigset_t* old_blocked_signals){
  if(pthread_sigmask(SIG_SETMASK, old_blocked_signals, NULL)){
    return -1;
  }
  return 0;
}

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
      sigaction(SIGABRT, &old_abrt, NULL);
      sigaction(SIGFPE, &old_fpe, NULL);
      sigaction(SIGILL, &old_ill, NULL);
      sigaction(SIGINT, &old_int, NULL);
      sigaction(SIGQUIT, &old_quit, NULL);
      sigaction(SIGSEGV, &old_segv, NULL);
      sigaction(SIGTERM, &old_term, NULL);
      handling_fatals = false;
    }
    ret = !atomic_compare_exchange_strong(&signal_nc, &expected, NULL);
  }
  pthread_mutex_unlock(&lock);
  if(ret){
    fprintf(stderr, "Signals weren't registered for %p (had %p)\n", nc, expected);
  }
  // we might not have established any handlers in setup_signals(); always
  // return 0 here, for now...
  return 0;
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
      case SIGSEGV: invoke_old(&old_segv, signo, siginfo, v); break;
      case SIGQUIT: invoke_old(&old_quit, signo, siginfo, v); break;
      case SIGINT: invoke_old(&old_int, signo, siginfo, v); break;
      case SIGILL: invoke_old(&old_ill, signo, siginfo, v); break;
      case SIGFPE: invoke_old(&old_fpe, signo, siginfo, v); break;
      case SIGABRT: invoke_old(&old_abrt, signo, siginfo, v); break;
    }
    raise(signo); // FIXME does this invoke twice? hrmmm
  }
}

// this both sets up our signal handlers (unless that behavior has been
// inhibited), and ensures that only one notcurses/ncdirect context is active
// at any given time.
int setup_signals(void* vnc, bool no_quit_sigs, bool no_winch_sigs,
                  int(*handler)(void*)){
  notcurses* nc = vnc;
  void* expected = NULL;
  struct sigaction sa;
  // don't register ourselves if we don't intend to set up signal handlers
  // we expect NULL (nothing registered), and want to register nc
  if(!atomic_compare_exchange_strong(&signal_nc, &expected, nc)){
    loginfo("%p is already registered for signals (provided %p)\n", expected, nc);
    return -1;
  }
  pthread_mutex_lock(&lock);
  if(!no_winch_sigs){
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigwinch_handler;
    sigaddset(&sa.sa_mask, SIGWINCH);
    sigaddset(&sa.sa_mask, SIGCONT);
    int ret = 0;
    ret |= sigaction(SIGWINCH, &sa, &old_winch);
    ret |= sigaction(SIGCONT, &sa, &old_cont);
    if(ret){
      atomic_store(&signal_nc, NULL);
      pthread_mutex_unlock(&lock);
      fprintf(stderr, "Error installing term signal handler (%s)\n", strerror(errno));
      return -1;
    }
    handling_winch = true;
  }
  if(!no_quit_sigs){
    memset(&sa, 0, sizeof(sa));
    fatal_callback = handler;
    sa.sa_sigaction = fatal_handler;
    sigaddset(&sa.sa_mask, SIGFPE);
    sigaddset(&sa.sa_mask, SIGILL);
    sigaddset(&sa.sa_mask, SIGINT);
    sigaddset(&sa.sa_mask, SIGQUIT);
    sigaddset(&sa.sa_mask, SIGSEGV);
    sigaddset(&sa.sa_mask, SIGABRT);
    sigaddset(&sa.sa_mask, SIGTERM);
    sa.sa_flags = SA_SIGINFO | SA_RESETHAND; // don't try fatal signals twice
    int ret = 0;
    ret |= sigaction(SIGFPE, &sa, &old_fpe);
    ret |= sigaction(SIGILL, &sa, &old_ill);
    ret |= sigaction(SIGINT, &sa, &old_int);
    ret |= sigaction(SIGQUIT, &sa, &old_quit);
    ret |= sigaction(SIGSEGV, &sa, &old_segv);
    ret |= sigaction(SIGABRT, &sa, &old_abrt);
    ret |= sigaction(SIGTERM, &sa, &old_term);
    if(ret){
      atomic_store(&signal_nc, NULL);
      pthread_mutex_unlock(&lock);
      fprintf(stderr, "Error installing fatal signal handlers (%s)\n", strerror(errno));
      return -1;
    }
    handling_fatals = true;
  }
  // we don't really want to go blocking SIGSEGV etc while we write, but
  // we *do* temporarily block user-inititated signals.
  sigaddset(&wblock_signals, SIGINT);
  sigaddset(&wblock_signals, SIGTERM);
  sigaddset(&wblock_signals, SIGQUIT);
  pthread_mutex_unlock(&lock);
  return 0;
}
#endif

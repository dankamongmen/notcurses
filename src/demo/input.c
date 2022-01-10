#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include "demo.h"

typedef struct nciqueue {
  ncinput ni;
  struct nciqueue *next;
} nciqueue;

// a pipe on which we write upon receipt of input, so that demos
// can reliably multiplex against other fds. osx doesn't have
// eventfd, alas (freebsd added it in 13.0).
static int input_pipefds[2] = {-1, -1};

static pthread_t tid;
static nciqueue* queue;
static nciqueue** enqueue = &queue;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond; // use pthread_condmonotonic_init()

static int
handle_mouse(const ncinput* ni){
  if(ni->id != NCKEY_BUTTON1){
    return 0;
  }
  int ret;
  if(ni->evtype == NCTYPE_RELEASE){
    ret = hud_release();
    if(ret < 0){
      ret = fpsplot_release();
    }
  }else{
    ret = hud_grab(ni->y, ni->x);
    if(ret < 0){
      ret = fpsplot_grab(ni->y);
    }
  }
  // do not render here. the demos, if coded properly, will be regularly
  // rendering (if via demo_nanosleep() if nothing else). rendering based off
  // HUD movements can cause disruptions due to the main thread being unready.
  return ret;
}

// incoming timespec is relative (or even NULL, for blocking), but we need
// absolute deadline, so convert it up.
uint32_t demo_getc(struct notcurses* nc, const struct timespec* ts, ncinput* ni){
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  uint64_t ns;
  // abstime shouldn't be further out than our maximum sleep time -- this can
  // lead to 0 frames output during the wait
  if(ts){
    ns = timespec_to_ns(ts);
  }else{
    ns = MAXSLEEP;
  }
  if(ns > MAXSLEEP){
    ns = MAXSLEEP;
  }
  struct timespec abstime;
  ns_to_timespec(ns + timespec_to_ns(&now), &abstime);
  bool handoff = false; // does the input go back to the user?
  uint32_t id;
  do{
    pthread_mutex_lock(&lock);
    while(!queue){
      clock_gettime(CLOCK_MONOTONIC, &now);
      if(timespec_to_ns(&now) > timespec_to_ns(&abstime)){
        pthread_mutex_unlock(&lock);
        return 0;
      }
      pthread_cond_timedwait(&cond, &lock, &abstime);
    }
    nciqueue* q = queue;
    queue = queue->next;
    if(queue == NULL){
      enqueue = &queue;
    }
    pthread_mutex_unlock(&lock);
    id = q->ni.id;
    // if this was about the menu or HUD, pass to them, and continue
    if(!menu_or_hud_key(nc, &q->ni)){
      if(nckey_mouse_p(q->ni.id)){
        if(!handle_mouse(&q->ni)){
          handoff = true;
        }
      }else{
        handoff = true;
      }
    }
    if(handoff && ni){
      memcpy(ni, &q->ni, sizeof(*ni));
    }
    free(q);
  }while(!handoff);
  return id;
}

static int
pass_along(const ncinput* ni){
  pthread_mutex_lock(&lock);
  nciqueue *nq = malloc(sizeof(*nq));
  memcpy(&nq->ni, ni, sizeof(*ni));
  nq->next = NULL;
  *enqueue = nq;
  enqueue = &nq->next;
  pthread_mutex_unlock(&lock);
  const uint64_t eventcount = 1;
  int ret = 0;
  if(write(input_pipefds[1], &eventcount, sizeof(eventcount)) < 0){
    ret = -1;
  }
  pthread_cond_signal(&cond);
  return ret;
}

static void *
ultramegaok_demo(void* vnc){
  ncinput ni;
  struct notcurses* nc = vnc;
  uint32_t id;
  while((id = notcurses_get_blocking(nc, &ni)) != (uint32_t)-1){
    if(id == 0){
      continue;
    }
    if(id == NCKEY_EOF){
      break;
    }
    // go ahead and pass keyboard through to demo, even if it was a 'q' (this
    // might cause the demo to exit immediately, as is desired). we can't just
    // mess with the menu/HUD in our own context, as the demo thread(s) might
    // be rendering, or otherwise fucking with things we musn't fuck with
    // concurrentwise (z-axis manipulations, etc.).
    pass_along(&ni);
  }
  return NULL;
}

int demo_input_fd(void){
  return input_pipefds[1];
}

// listens for events, handling mouse events directly and making other ones
// available to demos. returns -1 if already spawned or resource failures.
int input_dispatcher(struct notcurses* nc){
  if(input_pipefds[0] >= 0){
    return -1;
  }
  if(pthread_condmonotonic_init(&cond)){
    fprintf(stderr, "error creating monotonic condvar\n");
    return -1;
  }
  // freebsd doesn't have eventfd :/ and apple doesn't even have pipe2() =[ =[
  // omg windows doesn't have pipe() fml FIXME
#ifndef __MINGW32__
#if defined(__APPLE__)
  if(pipe(input_pipefds)){
#else
  if(pipe2(input_pipefds, O_CLOEXEC | O_NONBLOCK)){
#endif
    fprintf(stderr, "Error creating pipe (%s)\n", strerror(errno));
    return -1;
  }
#endif
  if(pthread_create(&tid, NULL, ultramegaok_demo, nc)){
    close(input_pipefds[0]);
    close(input_pipefds[1]);
    input_pipefds[0] = input_pipefds[1] = -1;
    return -1;
  }
  return 0;
}

int stop_input(void){
  int ret = 0;
  if(input_pipefds[0] >= 0){
    ret |= pthread_cancel(tid);
    ret |= pthread_join(tid, NULL);
    ret |= close(input_pipefds[0]);
    ret |= close(input_pipefds[1]);
    input_pipefds[0] = input_pipefds[1] = -1;
    ret |= pthread_cond_destroy(&cond);
  }
  return ret;
}

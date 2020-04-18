#include <pthread.h>
#include <sys/eventfd.h>
#include "demo.h"

typedef struct nciqueue {
  ncinput ni;
  struct nciqueue *next;
} nciqueue;

// an eventfd or pipe on which we write upon receipt of input, so that demos
// can multiplex against other fds.
static int input_eventfd = -1;

static pthread_t tid;
static nciqueue* queue;
static nciqueue** enqueue = &queue;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static int
handle_mouse(const ncinput* ni){
  if(ni->id != NCKEY_BUTTON1 && ni->id != NCKEY_RELEASE){
    return 0;
  }
  int ret;
  if(ni->id == NCKEY_RELEASE){
    ret = hud_release();
  }else{
    ret = hud_grab(ni->y, ni->x);
  }
  // do not render here. the demos, if coded properly, will be regularly
  // rendering (if via demo_nanosleep() if nothing else). rendering based off
  // HUD movements can cause disruptions due to the main thread being unready.
  return ret;
}

// incoming timespec is relative (or even NULL, for blocking), but we need
// absolute deadline, so convert it up.
char32_t demo_getc(struct notcurses* nc, const struct timespec* ts, ncinput* ni){
  struct timespec now;
  uint64_t ns;
  struct timespec abstime;
  // yes, i'd like CLOCK_MONOTONIC too, but pthread_cond_timedwait() is based off
  // of crappy CLOCK_REALTIME :/
  if(ts){
    clock_gettime(CLOCK_REALTIME, &now);
    ns = timespec_to_ns(&now) + timespec_to_ns(ts);
    ns_to_timespec(ns, &abstime);
  }else{
    abstime.tv_sec = ~0;
    abstime.tv_nsec = ~0;
  }
  bool handoff = false; // does the input go back to the user?
  char32_t id;
  do{
    pthread_mutex_lock(&lock);
    while(!queue){
      clock_gettime(CLOCK_REALTIME, &now);
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
          if(id == 'L' && q->ni.ctrl){
            notcurses_refresh(nc, NULL, NULL);
          }else{
            handoff = false;
          }
        }
      }else if(id == 'L' && q->ni.ctrl){
        notcurses_refresh(nc, NULL, NULL);
      }else{
        handoff = false;
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
  if(write(input_eventfd, &eventcount, sizeof(eventcount)) < 0){
    ret = -1;
  }
  pthread_cond_signal(&cond);
  return ret;
}

static void *
ultramegaok_demo(void* vnc){
  ncinput ni;
  struct notcurses* nc = vnc;
  char32_t id;
  while((id = notcurses_getc_blocking(nc, &ni)) != (char32_t)-1){
    if(id == 0){
      continue;
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
  return input_eventfd;
}

// listens for events, handling mouse events directly and making other ones
// available to demos. returns -1 if already spawned or resource failures.
int input_dispatcher(struct notcurses* nc){
  if(input_eventfd >= 0){
    return -1;
  }
  if((input_eventfd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK)) < 0){
    return -1;
  }
  if(pthread_create(&tid, NULL, ultramegaok_demo, nc)){
    close(input_eventfd);
    input_eventfd = -1;
    return -1;
  }
  return 0;
}

int stop_input(void){
  int ret = 0;
  if(input_eventfd >= 0){
    ret |= pthread_cancel(tid);
    ret |= pthread_join(tid, NULL);
    ret |= close(input_eventfd);
    input_eventfd = -1;
  }
  return ret;
}

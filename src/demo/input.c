#include <pthread.h>
#include "demo.h"

typedef struct nciqueue {
  ncinput ni;
  struct nciqueue *next;
} nciqueue;

static bool spawned;
static pthread_t tid;
static nciqueue* queue;
static nciqueue** enqueue = &queue;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

// incoming timespec is relative (or even NULL, for blocking), but we need
// absolute deadline, so convert it up.
char32_t demo_getc(const struct timespec* ts, ncinput* ni){
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
  char32_t id = q->ni.id;
  if(ni){
    memcpy(ni, &q->ni, sizeof(*ni));
  }
  free(q);
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
  pthread_cond_signal(&cond);
  return 0;
}

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

static void *
ultramegaok_demo(void* vnc){
  ncinput ni;
  struct notcurses* nc = vnc;
  char32_t id;
  while((id = notcurses_getc_blocking(nc, &ni)) != (char32_t)-1){
    if(id == 0){
      continue;
    }
    if(nckey_mouse_p(ni.id)){
      handle_mouse(&ni);
    }else{
      // if this was about the menu or HUD, pass to them, and continue
      if(ni.id == NCKEY_UP){
        continue;
      }else if(ni.id == NCKEY_DOWN){
        continue;
      }else if(ni.id == NCKEY_LEFT){
        continue;
      }else if(ni.id == NCKEY_RIGHT){
        continue;
      }
      if(ni.id == 'q'){
        interrupt_demo();
      }
      // go ahead and pass keyboard through to demo, even if it was a 'q'
      // (this might cause the demo to exit immediately, as is desired)
      pass_along(&ni);
    }
  }
  return NULL;
}

// listens for events, handling mouse events directly and making other ones
// available to demos
int input_dispatcher(struct notcurses* nc){
  spawned = true;
  if(pthread_create(&tid, NULL, ultramegaok_demo, nc)){
    spawned = false;
    return -1;
  }
  return 0;
}

int stop_input(void){
  int ret = 0;
  if(spawned){
    ret |= pthread_cancel(tid);
    ret |= pthread_join(tid, NULL);
    spawned = false;
  }
  return ret;
}

#include <pthread.h>
#include "demo.h"

typedef struct nciqueue {
  ncinput ni;
  struct nciqueue *next;
} nciqueue;

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
  if(ts){
    clock_gettime(CLOCK_MONOTONIC, &now);
    ns = timespec_to_ns(&now) + timespec_to_ns(ts);
    ns_to_timespec(ns, &abstime);
  }else{
    abstime.tv_sec = ~0;
    abstime.tv_nsec = ~0;
  }
  pthread_mutex_lock(&lock);
  while(!queue){
    clock_gettime(CLOCK_MONOTONIC, &now);
    if(timespec_to_ns(&now) > timespec_to_ns(&abstime)){
      pthread_mutex_unlock(&lock);
      return 0;
    }
    pthread_cond_timedwait(&cond, &lock, &abstime);
  }
  char32_t id = queue->ni.id;
  if(ni){
    memcpy(ni, &queue->ni, sizeof(*ni));
  }
  queue = queue->next;
  if(queue == NULL){
    enqueue = &queue;
  }
  pthread_mutex_unlock(&lock);
  return id;
}

static int
pass_along(const ncinput* ni){
  nciqueue *nq = malloc(sizeof(*nq));
  memcpy(&nq->ni, ni, sizeof(*ni));
  nq->next = NULL;
  *enqueue = nq;
  enqueue = &nq->next;
  return 0;
}

static int
handle_mouse(struct notcurses* nc, const ncinput* ni){
  if(ni->id != NCKEY_BUTTON1 && ni->id != NCKEY_RELEASE){
    return 0;
  }
  int ret;
  if(ni->id == NCKEY_RELEASE){
    ret = hud_release();
  }else{
    ret = hud_grab(ni->y, ni->x);
  }
  if(ret == 0){
    ret = demo_render(nc);
  }
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
      handle_mouse(nc, &ni);
    }else{
      if(ni.id == 'q'){
        interrupt_demo();
      }
      // go ahead and pass through the keyboard press, even if it was a 'q'
      // (this might cause the demo to exit immediately, as is desired)
      pass_along(&ni);
    }
  }
  return NULL;
}

// listens for events, handling mouse events directly and making other ones
// available to demos
int input_dispatcher(struct notcurses* nc){
  if(pthread_create(&tid, NULL, ultramegaok_demo, nc)){
    return -1;
  }
  return 0;
}

int stop_input(void){
  int ret = 0;
  ret |= pthread_cancel(tid);
  ret |= pthread_join(tid, NULL);
  return ret;
}

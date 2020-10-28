#ifndef NOTCURSES_DEMO
#define NOTCURSES_DEMO

#include <time.h>
#include <assert.h>
#include <unistd.h>
#include <limits.h>
#include <stdatomic.h>
#include <notcurses/notcurses.h>
#include <builddef.h>
#include <version.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CLOCK_MONOTONIC_RAW
#define CLOCK_MONOTONIC_RAW CLOCK_MONOTONIC
#endif

// configured via command line option -- the base number of ns between demos
extern struct timespec demodelay;
extern float delaymultiplier; // scales demodelay (applied internally)

// checked in demo_render() and between demos
extern atomic_bool interrupted;

// heap-allocated, caller must free. locates data files per command line args.
char* find_data(const char* datum);

int allglyphs_demo(struct notcurses* nc);
int unicodeblocks_demo(struct notcurses* nc);
int witherworm_demo(struct notcurses* nc);
int box_demo(struct notcurses* nc);
int trans_demo(struct notcurses* nc);
int chunli_demo(struct notcurses* nc);
int dragon_demo(struct notcurses* nc);
int qrcode_demo(struct notcurses* nc);
int grid_demo(struct notcurses* nc);
int fallin_demo(struct notcurses* nc);
int highcontrast_demo(struct notcurses* nc);
int jungle_demo(struct notcurses* nc);
int yield_demo(struct notcurses* nc);
int mojibake_demo(struct notcurses* nc);
int normal_demo(struct notcurses* nc);
int sliding_puzzle_demo(struct notcurses* nc);
int view_demo(struct notcurses* nc);
int eagle_demo(struct notcurses* nc);
int reel_demo(struct notcurses* nc);
int xray_demo(struct notcurses* nc);
int keller_demo(struct notcurses* nc);
int luigi_demo(struct notcurses* nc);
int zoo_demo(struct notcurses* nc);
int intro(struct notcurses* nc);
int outro(struct notcurses* nc);

/*------------------------------- demo input API --------------------------*/
int input_dispatcher(struct notcurses* nc);
int stop_input(void);

// if 'q' is pressed at any time during the demo, gracefully interrupt/exit
void interrupt_demo(void);
void interrupt_and_restart_demos(void);

// demos should not call notcurses_getc() directly, as it's being monitored by
// the toplevel event listener. instead, call this intermediate API. just
// replace 'notcurses' with 'demo'.
char32_t demo_getc(struct notcurses* nc, const struct timespec* ts, ncinput* ni);

// 'ni' may be NULL if the caller is uninterested in event details. If no event
// is ready, returns 0.
static inline char32_t
demo_getc_nblock(struct notcurses* nc, ncinput* ni){
  struct timespec ts = { .tv_sec = 0, .tv_nsec = 0 };
  return demo_getc(nc, &ts, ni);
}

// Get a fd which can be poll()ed to check for outstanding input. Do not close
// this file descriptor.
int demo_input_fd(void);

// 'ni' may be NULL if the caller is uninterested in event details. Blocks
// until an event is processed or a signal is received.
static inline char32_t
demo_getc_blocking(struct notcurses* nc, ncinput* ni){
  return demo_getc(nc, NULL, ni);
}
/*----------------------------- end demo input API -------------------------*/

/*-------------------------------time helpers----------------------------*/
#define GIG 1000000000ul

static inline uint64_t
timespec_to_ns(const struct timespec* ts){
  return ts->tv_sec * GIG + ts->tv_nsec;
}

static inline struct timespec*
ns_to_timespec(uint64_t ns, struct timespec* ts){
  ts->tv_sec = ns / GIG;
  ts->tv_nsec = ns % GIG;
  return ts;
}

static inline int64_t
timespec_subtract_ns(const struct timespec* time1, const struct timespec* time0){
  int64_t ns = timespec_to_ns(time1);
  ns -= timespec_to_ns(time0);
  return ns;
}

static inline int
timespec_subtract(struct timespec *result, const struct timespec *time0,
                  struct timespec *time1){
  ns_to_timespec(timespec_subtract_ns(time0, time1), result);
  return timespec_to_ns(time0) < timespec_to_ns(time1);
}

static inline uint64_t
timespec_add(struct timespec *result, const struct timespec *time0,
             struct timespec *time1){
  uint64_t ns = timespec_to_ns(time0) + timespec_to_ns(time1);
  ns_to_timespec(ns, result);
  return ns;
}

// divide the provided timespec 'ts' by 'divisor' into 'quots'
static inline void
timespec_div(const struct timespec* ts, unsigned divisor, struct timespec* quots){
  uint64_t ns = timespec_to_ns(ts);
  ns /= divisor;
  quots->tv_nsec = ns % GIG;
  quots->tv_sec = ns / GIG;
}

// divide the provided timespec 'ts' by 'multiplier' into 'product'
static inline void
timespec_mul(const struct timespec* ts, unsigned multiplier, struct timespec* product){
  uint64_t ns = timespec_to_ns(ts);
  ns *= multiplier;
  product->tv_nsec = ns % GIG;
  product->tv_sec = ns / GIG;
}
/*-------------------------------time helpers----------------------------*/

/*---------------------------------FPS plot-------------------------------*/
int fpsgraph_init(struct notcurses* nc);
int fpsgraph_stop(struct notcurses* nc);
int fpsplot_grab(int y);
int fpsplot_release(void);
/*----------------------------------HUD----------------------------------*/
extern struct ncplane* hud;
struct ncplane* hud_create(struct notcurses* nc);
struct ncmenu* menu_create(struct notcurses* nc);
int hud_destroy(void);

// let the HUD know about an upcoming demo
int hud_schedule(const char* demoname);

// demos should not call notcurses_render() themselves, but instead call
// demo_render(), which will ensure the HUD stays on the top of the z-stack.
// returns -1 on error, 1 if the demo has been aborted, and 0 on success.
// this result ought be propagated out so that the demo is reported as having
// been aborted, rather than having failed.
int demo_render(struct notcurses* nc);

#define DEMO_RENDER(nc) { int demo_render_err = demo_render(nc); if(demo_render_err){ return demo_render_err; }}

// if you won't be doing things, and it's a long sleep, consider using
// demo_nanosleep(). it updates the HUD, which looks better to the user, and
// dispatches input to the menu/HUD. don't use it if you have other threads
// rendering or otherwise manipulating state, as it calls notcurses_render().
int demo_nanosleep(struct notcurses* nc, const struct timespec *ts);

// the same as demo_nanosleep, but using TIMER_ABSTIME (deadline, not interval)
int demo_nanosleep_abstime(struct notcurses* nc, const struct timespec* ts);

static inline int
demo_simple_streamer(struct ncvisual* ncv __attribute__ ((unused)),
                     struct ncvisual_options* vopts,
                     const struct timespec* tspec, void* curry __attribute__ ((unused))){
  DEMO_RENDER(ncplane_notcurses(vopts->n));
  return demo_nanosleep_abstime(ncplane_notcurses(vopts->n), tspec);
}

// simple fadecb that makes proper use of demo_render() and demo_nanosleep()
static inline int
demo_fader(struct notcurses* nc, struct ncplane* ncp __attribute__ ((unused)),
           const struct timespec* abstime, void* curry __attribute__ ((unused))){
  DEMO_RENDER(nc);
  return demo_nanosleep_abstime(nc, abstime);
}


// grab the hud with the mouse
int hud_grab(int y, int x);

// release the hud
int hud_release(void);

typedef struct demoresult {
  char selector;
  struct ncstats stats;
  uint64_t timens;
  int result; // positive == aborted, negative == failed
} demoresult;

// let the HUD know that a demo has completed, reporting the stats
int hud_completion_notify(const demoresult* result);

// HUD retrieves results on demand from core
const demoresult* demoresult_lookup(int idx);
/*----------------------------------HUD----------------------------------*/

// destroy about popup
void about_destroy(struct notcurses* nc);

// returns true if the input was handled by the menu/HUD
bool menu_or_hud_key(struct notcurses *nc, const struct ncinput *ni);

// returns 2 if we've successfully passed the deadline, 1 if we've been aborted
// (as returned by demo_render(), 0 if we ought keep going, or -1 if there was
// an error. in general, propagate out -1 or 1, keep going on 2, and don't
// expect to ever see 0.
static inline int
pulser(struct notcurses* nc, struct ncplane* ncp __attribute__ ((unused)),
       const struct timespec* ts __attribute__ ((unused)), void* curry){
  struct timespec* start = curry;
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  if(timespec_to_ns(&now) - timespec_to_ns(start) >= timespec_to_ns(&demodelay) * 10 / 3){
    return 2;
  }
  return demo_render(nc);
}

#ifdef __cplusplus
}
#endif

#endif

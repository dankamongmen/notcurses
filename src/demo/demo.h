#ifndef NOTCURSES_DEMO
#define NOTCURSES_DEMO

#include <time.h>
#include <assert.h>
#include <unistd.h>
#include <limits.h>
#include <notcurses.h>
#ifndef DISABLE_FFMPEG
#include <libavutil/pixdesc.h>
#include <libavutil/avconfig.h>
#include <libavcodec/avcodec.h> // ffmpeg doesn't reliably "C"-guard itself
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CLOCK_MONOTONIC_RAW
#define CLOCK_MONOTONIC_RAW CLOCK_MONOTONIC
#endif

// configured via command line option -- the base number of ns between demos
extern struct timespec demodelay;
extern float delaymultiplier; // scales demodelay (applied internally)

// heap-allocated, caller must free. locates data files per command line args.
char* find_data(const char* datum);

int unicodeblocks_demo(struct notcurses* nc);
int witherworm_demo(struct notcurses* nc);
int box_demo(struct notcurses* nc);
int trans_demo(struct notcurses* nc);
int chunli_demo(struct notcurses* nc);
int grid_demo(struct notcurses* nc);
int fallin_demo(struct notcurses* nc);
int jungle_demo(struct notcurses* nc);
int sliding_puzzle_demo(struct notcurses* nc);
int view_demo(struct notcurses* nc);
int eagle_demo(struct notcurses* nc);
int panelreel_demo(struct notcurses* nc);
int xray_demo(struct notcurses* nc);
int luigi_demo(struct notcurses* nc);
int intro(struct notcurses* nc);
int outro(struct notcurses* nc);

/*------------------------------- demo input API --------------------------*/
int input_dispatcher(struct notcurses* nc);
int stop_input(void);

// if 'q' is pressed at any time during the demo, gracefully interrupt/exit
void interrupt_demo(void);

// demos should not call notcurses_getc() directly, as it's being monitored by
// the toplevel event listener. instead, call this intermediate API. just
// replace 'notcurses' with 'demo'.
char32_t demo_getc(const struct timespec* ts, ncinput* ni);

// 'ni' may be NULL if the caller is uninterested in event details. If no event
// is ready, returns 0.
static inline char32_t
demo_getc_nblock(ncinput* ni){
  struct timespec ts = { .tv_sec = 0, .tv_nsec = 0 };
  return demo_getc(&ts, ni);
}

// 'ni' may be NULL if the caller is uninterested in event details. Blocks
// until an event is processed or a signal is received.
static inline char32_t
demo_getc_blocking(ncinput* ni){
  return demo_getc(NULL, ni);
}
/*----------------------------- end demo input API -------------------------*/

/*-------------------------------time helpers----------------------------*/
int timespec_subtract(struct timespec *result, const struct timespec *time1,
                      struct timespec *time0);

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

/*----------------------------------HUD----------------------------------*/
extern struct ncplane* hud;
struct ncplane* hud_create(struct notcurses* nc);
int hud_destroy(void);

// let the HUD know about an upcoming demo
int hud_schedule(const char* demoname);

// demos should not call notcurses_render() themselves, but instead call
// demo_render(), which will ensure the HUD stays on the top of the z-stack.
int demo_render(struct notcurses* nc);

// if you won't be doing things, and it's a long sleep, consider using
// demo_nanosleep(). it updates the HUD, which looks better to the user.
int demo_nanosleep(struct notcurses* nc, const struct timespec *ts);

int demo_fader(struct notcurses* nc, struct ncplane* ncp, void* curry);

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

static inline int
pulser(struct notcurses* nc, struct ncplane* ncp __attribute__ ((unused)), void* curry){
  struct timespec* start = curry;
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  if(timespec_to_ns(&now) - timespec_to_ns(start) >= timespec_to_ns(&demodelay) * 10 / 3){
    return 1;
  }
  return demo_render(nc);
}

#ifdef __cplusplus
}
#endif

#endif

#ifndef NOTCURSES_DEMO
#define NOTCURSES_DEMO

#include <time.h>
#include <assert.h>
#include <limits.h>
#include <notcurses.h>

#ifdef __cplusplus
extern "C" {
#endif

// configured via command line option -- the base number of ns between demos
extern struct timespec demodelay;

// heap-allocated, caller must free. locates data files per command line args.
char* find_data(const char* datum);

int unicodeblocks_demo(struct notcurses* nc);
int witherworm_demo(struct notcurses* nc);
int box_demo(struct notcurses* nc);
int maxcolor_demo(struct notcurses* nc);
int grid_demo(struct notcurses* nc);
int sliding_puzzle_demo(struct notcurses* nc);
int view_demo(struct notcurses* nc);
int eagle_demo(struct notcurses* nc);
int panelreel_demo(struct notcurses* nc);
int xray_demo(struct notcurses* nc);
int luigi_demo(struct notcurses* nc);
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

#ifdef __cplusplus
}
#endif

#endif

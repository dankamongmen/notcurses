#include <time.h>
#include <wchar.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <locale.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <notcurses.h>
#include "version.h"
#include "demo.h"

// ansi terminal definition-4-life
static const int MIN_SUPPORTED_ROWS = 24;
static const int MIN_SUPPORTED_COLS = 80;

static int democount;
static demoresult* results;
static char datadir[PATH_MAX];

#ifdef DISABLE_FFMPEG
static const char DEFAULT_DEMO[] = "ithbgrwus";
#else
#ifdef DFSG_BUILD
static const char DEFAULT_DEMO[] = "ixthbgrwuso";
#else
static const char DEFAULT_DEMO[] = "ixethbcgrwuvlfsjo";
#endif
#endif

atomic_bool interrupted = ATOMIC_VAR_INIT(false);
// checked following demos, whether aborted, failed, or otherwise
static atomic_bool restart_demos = ATOMIC_VAR_INIT(false);

void interrupt_demo(void){
  atomic_store(&interrupted, true);
}

void interrupt_and_restart_demos(void){
  atomic_store(&restart_demos, true);
  atomic_store(&interrupted, true);
}

const demoresult* demoresult_lookup(int idx){
  if(idx < 0 || idx >= democount){
    return NULL;
  }
  return &results[idx];
}

char* find_data(const char* datum){
  char* path = malloc(strlen(datadir) + 1 + strlen(datum) + 1);
  strcpy(path, datadir);
  strcat(path, "/");
  strcat(path, datum);
  return path;
}

int timespec_subtract(struct timespec *result, const struct timespec *time0,
                      struct timespec *time1){
  if(time0->tv_nsec < time1->tv_nsec){
    int nsec = (time1->tv_nsec - time0->tv_nsec) / 1000000000 + 1;
    time1->tv_nsec -= 1000000000 * nsec;
    time1->tv_sec += nsec;
  }
  if(time0->tv_nsec - time1->tv_nsec > 1000000000){
    int nsec = (time0->tv_nsec - time1->tv_nsec) / 1000000000;
    time1->tv_nsec += 1000000000 * nsec;
    time1->tv_sec -= nsec;
  }
  result->tv_sec = time0->tv_sec - time1->tv_sec;
  result->tv_nsec = time0->tv_nsec - time1->tv_nsec;
  return time0->tv_sec < time1->tv_sec;
}

float delaymultiplier = 1;

// scaled in getopt() by delaymultiplier
struct timespec demodelay = {
  .tv_sec = 1,
  .tv_nsec = 0,
};

// anything that's dfsg non-free requires ncvisual (i.e. it's all multimedia),
// so also check for FFMPEG_DISABLE here in DFSG_BUILD
#ifndef DFSG_BUILD
#ifndef DISABLE_FFMPEG
#define NONFREE(name, fxn) { name, fxn, }
#endif
#endif
#ifndef NONFREE
#define NONFREE(name, fxn) { NULL, NULL, }
#endif

#ifndef DISABLE_FFMPEG
#define FREEFFMPEG(name, fxn) { name, fxn, }
#else
#define FREEFFMPEG(name, fxn) { NULL, NULL, }
#endif

// define with NONFREE() to exempt from any DFSG or non-FFmpeg build. define
// with FREEFFMPEG() to exempt from any non-FFmpeg build.
static struct {
  const char* name;
  int (*fxn)(struct notcurses*);
} demos[26] = {
  { NULL, NULL, },
  { "box", box_demo, },
  NONFREE("chunli", chunli_demo),
  { NULL, NULL, },
  NONFREE("eagle", eagle_demo),
  NONFREE("fallin'", fallin_demo),
  { "grid", grid_demo, },
  { "highcon", highcontrast_demo, },
  { "intro", intro, },
  NONFREE("jungle", jungle_demo),
  { NULL, NULL, },
  NONFREE("luigi", luigi_demo),
  { NULL, NULL, },
  { NULL, NULL, },
  FREEFFMPEG("outro", outro),
  { NULL, NULL, },
  { NULL, NULL, },
  { "reel", reel_demo, },
  { "sliders", sliding_puzzle_demo, },
  { "trans", trans_demo, },
  { "uniblock", unicodeblocks_demo, },
  NONFREE("view", view_demo),
  { "whiteout", witherworm_demo, },
  FREEFFMPEG("xray", xray_demo),
  { NULL, NULL, },
  { NULL, NULL, },
};

static void
usage(const char* exe, int status){
  FILE* out = status == EXIT_SUCCESS ? stdout : stderr;
  fprintf(out, "usage: %s [ -hVkc ] [ -p path ] [ -l loglevel ] [ -d mult ] [ -f renderfile ] demospec\n", exe);
  fprintf(out, " -h: this message\n");
  fprintf(out, " -V: print program name and version\n");
  fprintf(out, " -l: logging level (%d: silent..%d: manic)\n", NCLOGLEVEL_SILENT, NCLOGLEVEL_TRACE);
  fprintf(out, " -k: keep screen; do not switch to alternate\n");
  fprintf(out, " -d: delay multiplier (non-negative float)\n");
  fprintf(out, " -f: render to file in addition to stdout\n");
  fprintf(out, " -c: constant PRNG seed, useful for benchmarking\n");
  fprintf(out, " -p: data file path (default: %s)\n", NOTCURSES_SHARE);
  fprintf(out, "if no specification is provided, run %s\n", DEFAULT_DEMO);
  for(size_t i = 0 ; i < sizeof(demos) / sizeof(*demos) ; ++i){
    if(demos[i].name){
      fprintf(out, " %c: run %s\n", (unsigned char)i + 'a', demos[i].name);
    }
  }
  exit(status);
}

static demoresult*
ext_demos(struct notcurses* nc, const char* spec, bool ignore_failures){
  int ret = 0;
  results = malloc(sizeof(*results) * strlen(spec));
  if(results == NULL){
    return NULL;
  }
  memset(results, 0, sizeof(*results) * strlen(spec));
  democount = strlen(spec);
  struct timespec start, now;
  clock_gettime(CLOCK_MONOTONIC, &start);
  uint64_t prevns = timespec_to_ns(&start);
  for(size_t i = 0 ; i < strlen(spec) ; ++i){
    results[i].selector = spec[i];
  }
  for(size_t i = 0 ; i < strlen(spec) ; ++i){
    if(interrupted){
      break;
    }
    int idx = spec[i] - 'a';
    hud_schedule(demos[idx].name);
    ret = demos[idx].fxn(nc);
    notcurses_reset_stats(nc, &results[i].stats);
    clock_gettime(CLOCK_MONOTONIC, &now);
    uint64_t nowns = timespec_to_ns(&now);
    results[i].timens = nowns - prevns;
    prevns = nowns;
    results[i].result = ret;
    if(ret && !ignore_failures){
      break;
    }
    hud_completion_notify(&results[i]);
  }
  return results;
}

// returns the demos to be run as a string. on error, returns NULL. on no
// specification, also returns NULL, heh. determine this by argv[optind];
// if it's NULL, there were valid options, but no spec.
static const char*
handle_opts(int argc, char** argv, notcurses_options* opts, bool* ignore_failures){
  strcpy(datadir, NOTCURSES_SHARE);
  char renderfile[PATH_MAX] = "";
  bool constant_seed = false;
  *ignore_failures = false;
  int c;
  memset(opts, 0, sizeof(*opts));
  while((c = getopt(argc, argv, "Vhickl:r:d:f:p:")) != EOF){
    switch(c){
      case 'h':
        usage(*argv, EXIT_SUCCESS);
        break;
      case 'l':{
        int loglevel;
        if(sscanf(optarg, "%d", &loglevel) != 1){
          fprintf(stderr, "Couldn't get an int from %s\n", optarg);
          usage(*argv, EXIT_FAILURE);
        }
        opts->loglevel = loglevel;
        if(opts->loglevel < NCLOGLEVEL_SILENT || opts->loglevel > NCLOGLEVEL_TRACE){
          fprintf(stderr, "Invalid log level: %d\n", opts->loglevel);
          usage(*argv, EXIT_FAILURE);
        }
        break;
      }case 'V':
        printf("notcurses-demo version %s\n", notcurses_version());
        exit(EXIT_SUCCESS);
      case 'c':
        constant_seed = true;
        break;
      case 'i':
        *ignore_failures = true;
        break;
      case 'k':
        opts->inhibit_alternate_screen = true;
        break;
      case 'f':
        if(opts->renderfp){
          fprintf(stderr, "-f may only be supplied once\n");
          usage(*argv, EXIT_FAILURE);
        }
        if((opts->renderfp = fopen(optarg, "wb")) == NULL){
          usage(*argv, EXIT_FAILURE);
        }
        break;
      case 'p':
        strcpy(datadir, optarg);
        break;
      case 'r':
        strcpy(renderfile, optarg);
        break;
      case 'd':{
        float f;
        if(sscanf(optarg, "%f", &f) != 1){
          fprintf(stderr, "Couldn't get a float from %s\n", optarg);
          usage(*argv, EXIT_FAILURE);
        }
        if(f < 0){
          fprintf(stderr, "Invalid multiplier: %f\n", f);
          usage(*argv, EXIT_FAILURE);
        }
        delaymultiplier = f;
        uint64_t ns = f * GIG;
        demodelay.tv_sec = ns / GIG;
        demodelay.tv_nsec = ns % GIG;
        break;
      }default:
        usage(*argv, EXIT_FAILURE);
    }
  }
  if(!constant_seed){
    srand(time(NULL)); // a classic blunder lol
  }
  if(strlen(renderfile)){
    opts->renderfp = fopen(renderfile, "wb");
    if(opts->renderfp == NULL){
      fprintf(stderr, "Error opening %s for write\n", renderfile);
      usage(*argv, EXIT_FAILURE);
    }
  }
  const char* spec = argv[optind];
  return spec;
}

static int
table_segment_color(struct ncdirect* nc, const char* str, const char* delim, unsigned color){
  ncdirect_fg(nc, color);
  fputs(str, stdout);
  ncdirect_fg_rgb8(nc, 178, 102, 255);
  fputs(delim, stdout);
  return 0;
}

static int
table_segment(struct ncdirect* nc, const char* str, const char* delim){
  return table_segment_color(nc, str, delim, 0xffffff);
}

static int
table_printf(struct ncdirect* nc, const char* delim, const char* fmt, ...){
  ncdirect_fg_rgb8(nc, 0xD4, 0xAF, 0x37);
  va_list va;
  va_start(va, fmt);
  vfprintf(stdout, fmt, va);
  va_end(va);
  ncdirect_fg_rgb8(nc, 178, 102, 255);
  fputs(delim, stdout);
  return 0;
}

static int
summary_table(struct ncdirect* nc, const char* spec){
  bool failed = false;
  uint64_t totalbytes = 0;
  long unsigned totalframes = 0;
  uint64_t totalrenderns = 0;
  printf("\n");
  table_segment(nc, "              runtime", "│");
  table_segment(nc, "frames", "│");
  table_segment(nc, "output(B)", "│");
  table_segment(nc, "rendering", "│");
  table_segment(nc, " %r", "│");
  table_segment(nc, "    FPS", "│");
  table_segment(nc, "TheoFPS", "║\n══╤═════════╤════════╪══════╪═════════╪═════════╪═══╪═══════╪═══════╣\n");
  char timebuf[PREFIXSTRLEN + 1];
  char totalbuf[BPREFIXSTRLEN + 1];
  char rtimebuf[PREFIXSTRLEN + 1];
  uint64_t nsdelta = 0;
  for(size_t i = 0 ; i < strlen(spec) ; ++i){
    nsdelta += results[i].timens;
    qprefix(results[i].timens, GIG, timebuf, 0);
    qprefix(results[i].stats.render_ns, GIG, rtimebuf, 0);
    bprefix(results[i].stats.render_bytes, 1, totalbuf, 0);
    uint32_t rescolor;
    if(results[i].result != 0){
      rescolor = 0xff303c;
    }else if(!results[i].stats.renders){
      rescolor = 0xbbbbbb;
    }else{
      rescolor = 0x32CD32;
    }
    ncdirect_fg(nc, rescolor);
    printf("%2zu", i);
    ncdirect_fg_rgb8(nc, 178, 102, 255);
    printf("│");
    ncdirect_fg(nc, rescolor);
    printf("%9s", demos[results[i].selector - 'a'].name);
    ncdirect_fg_rgb8(nc, 178, 102, 255);
    printf("│%*ss│%6lu│%*s│ %*ss│%3ld│%7.1f│%7.1f║",
           PREFIXSTRLEN, timebuf,
           results[i].stats.renders,
           BPREFIXSTRLEN, totalbuf,
           PREFIXSTRLEN, rtimebuf,
           results[i].timens ?
            results[i].stats.render_ns * 100 / results[i].timens : 0,
           results[i].timens ?
            results[i].stats.renders / ((double)results[i].timens / GIG) : 0.0,
           results[i].stats.renders ?
            GIG * (double)results[i].stats.renders / results[i].stats.render_ns : 0.0);
    ncdirect_fg(nc, rescolor);
    printf("%s\n", results[i].result < 0 ? "FAILED" :
            results[i].result > 0 ? "ABORTED" :
             !results[i].stats.renders ? "SKIPPED"  : "");
    if(results[i].result < 0){
      failed = true;
    }
    totalframes += results[i].stats.renders;
    totalbytes += results[i].stats.render_bytes;
    totalrenderns += results[i].stats.render_ns;
  }
  free(results);
  qprefix(nsdelta, GIG, timebuf, 0);
  bprefix(totalbytes, 1, totalbuf, 0);
  qprefix(totalrenderns, GIG, rtimebuf, 0);
  table_segment(nc, "", "══╧═════════╧════════╪══════╪═════════╪═════════╪═══╪═══════╪═══════╝\n");
  table_printf(nc, "│", "             %*ss", PREFIXSTRLEN, timebuf);
  table_printf(nc, "│", "%6lu", totalframes);
  table_printf(nc, "│", "%*s", BPREFIXSTRLEN, totalbuf);
  table_printf(nc, "│", " %*ss", PREFIXSTRLEN, rtimebuf);
  table_printf(nc, "│", "%3ld", nsdelta ? totalrenderns * 100 / nsdelta : 0);
  table_printf(nc, "│", "%7.1f", nsdelta ? totalframes / ((double)nsdelta / GIG) : 0);
  printf("\n");
  ncdirect_fg_rgb8(nc, 0xff, 0xb0, 0xb0);
  fflush(stdout); // in case we print to stderr below, we want color from above
  if(failed){
    fprintf(stderr, "\nError running demo. Is \"%s\" the correct data path?\n", datadir);
  }
#ifdef DFSG_BUILD
  ncdirect_fg_rgb8(nc, 0xfe, 0x20, 0x76); // PANTONE Strong Red C + 3x0x20
  fflush(stdout); // in case we print to stderr below, we want color from above
  fprintf(stderr, "\nDFSG version. Some demos are missing.\n");
#endif
  return failed;
}

int main(int argc, char** argv){
  if(!setlocale(LC_ALL, "")){
    fprintf(stderr, "Couldn't set locale based on user preferences\n");
    return EXIT_FAILURE;
  }
  sigset_t sigmask;
  sigemptyset(&sigmask);
  sigaddset(&sigmask, SIGWINCH);
  pthread_sigmask(SIG_SETMASK, &sigmask, NULL);
  const char* spec;
  bool ignore_failures;
  notcurses_options nopts;
  if((spec = handle_opts(argc, argv, &nopts, &ignore_failures)) == NULL){
    if(argv[optind] != NULL){
      usage(*argv, EXIT_FAILURE);
    }
    spec = DEFAULT_DEMO;
  }
  for(size_t i = 0 ; i < strlen(spec) ; ++i){
    int nameidx = spec[i] - 'a';
    if(nameidx < 0 || nameidx > 25 || !demos[nameidx].name){
      fprintf(stderr, "Invalid demo specification: %c\n", spec[i]);
      return EXIT_FAILURE;
    }
  }
  struct timespec starttime;
  clock_gettime(CLOCK_MONOTONIC_RAW, &starttime);
  struct notcurses* nc;
  if((nc = notcurses_init(&nopts, stdout)) == NULL){
    return EXIT_FAILURE;
  }
  if(notcurses_mouse_enable(nc)){
    goto err;
  }
  if(input_dispatcher(nc)){
    goto err;
  }
  int dimx, dimy;
  notcurses_term_dim_yx(nc, &dimy, &dimx);
  if(dimy < MIN_SUPPORTED_ROWS || dimx < MIN_SUPPORTED_COLS){
    goto err;
  }
  if(nopts.inhibit_alternate_screen){ // no one cares. 1s max.
    if(demodelay.tv_sec >= 1){
      sleep(1);
    }else{
      nanosleep(&demodelay, NULL);
    }
  }
  do{
    restart_demos = false;
    interrupted = false;
    notcurses_drop_planes(nc);
    if(menu_create(nc) == NULL){
      goto err;
    }
    if(ext_demos(nc, spec, ignore_failures) == NULL){
      goto err;
    }
    if(hud_destroy()){ // destroy here since notcurses_drop_planes will kill it
      goto err;
    }
  }while(restart_demos);
  if(stop_input()){
    goto err;
  }
  if(notcurses_stop(nc)){
    return EXIT_FAILURE;
  }
  if(nopts.renderfp){
    if(fclose(nopts.renderfp)){
      fprintf(stderr, "Warning: error closing renderfile\n");
    }
  }
  struct ncdirect* ncd = notcurses_directmode(NULL, stdout);
  // reinitialize without alternate screen to do some coloring
  if(!ncd){
    return EXIT_FAILURE;
  }
  if(summary_table(ncd, spec)){
    ncdirect_stop(ncd);
    return EXIT_FAILURE;
  }
  ncdirect_stop(ncd);
  return EXIT_SUCCESS;

err:
  notcurses_term_dim_yx(nc, &dimy, &dimx);
  notcurses_stop(nc);
  if(dimy < MIN_SUPPORTED_ROWS || dimx < MIN_SUPPORTED_COLS){
    fprintf(stderr, "At least an %dx%d terminal is required (current: %dx%d)\n",
            MIN_SUPPORTED_COLS, MIN_SUPPORTED_ROWS, dimx, dimy);
  }
  return EXIT_FAILURE;
}

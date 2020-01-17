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
static atomic_bool interrupted = ATOMIC_VAR_INIT(false);

static const char DEFAULT_DEMO[] = "ixetcgpwubvlfso";

void interrupt_demo(void){
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

static void
usage(const char* exe, int status){
  FILE* out = status == EXIT_SUCCESS ? stdout : stderr;
  fprintf(out, "usage: %s [ -hHVkc ] [ -p path ] [ -l loglevel ] [ -d mult ] [ -f renderfile ] demospec\n", exe);
  fprintf(out, " -h: this message\n");
  fprintf(out, " -V: print program name and version\n");
  fprintf(out, " -l: logging level (%d: silent..%d: manic)\n", NCLOGLEVEL_SILENT, NCLOGLEVEL_TRACE);
  fprintf(out, " -H: deploy the HUD\n");
  fprintf(out, " -k: keep screen; do not switch to alternate\n");
  fprintf(out, " -d: delay multiplier (float)\n");
  fprintf(out, " -f: render to file in addition to stdout\n");
  fprintf(out, " -c: constant PRNG seed, useful for benchmarking\n");
  fprintf(out, " -p: data file path\n");
  fprintf(out, "if no specification is provided, run %s\n", DEFAULT_DEMO);
  fprintf(out, " b: run box\n");
  fprintf(out, " c: run chunli\n");
  fprintf(out, " e: run eagles\n");
  fprintf(out, " g: run grid\n");
  fprintf(out, " i: run intro\n");
  fprintf(out, " l: run luigi\n");
  fprintf(out, " o: run outro\n");
  fprintf(out, " p: run panelreels\n");
  fprintf(out, " s: run sliders\n");
  fprintf(out, " t: run trans\n");
  fprintf(out, " u: run uniblock\n");
  fprintf(out, " v: run view\n");
  fprintf(out, " w: run witherworm\n");
  fprintf(out, " x: run x-ray\n");
  exit(status);
}

static struct {
  const char* name;
  int (*fxn)(struct notcurses*);
} demos[26] = {
  { NULL, NULL, },
  { "box", box_demo, },
  { "chunli", chunli_demo, },
  { NULL, NULL, },
  { "eagle", eagle_demo, },
  { "fallin", fallin_demo, },
  { "grid", grid_demo, },
  { NULL, NULL, },
  { "intro", intro, },
  { NULL, NULL, },
  { NULL, NULL, },
  { "luigi", luigi_demo, },
  { NULL, NULL, },
  { NULL, NULL, },
  { "outro", outro, },
  { "panelreels", panelreel_demo, },
  { NULL, NULL, },
  { NULL, NULL, },
  { "sliders", sliding_puzzle_demo, },
  { "trans", trans_demo, },
  { "uniblock", unicodeblocks_demo, },
  { "view", view_demo, },
  { "witherworms", witherworm_demo, },
  { "xray", xray_demo, },
  { NULL, NULL, },
  { NULL, NULL, },
};

static demoresult*
ext_demos(struct notcurses* nc, const char* spec){
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
    if(ret){
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
handle_opts(int argc, char** argv, notcurses_options* opts, bool* use_hud){
  strcpy(datadir, NOTCURSES_SHARE);
  char renderfile[PATH_MAX] = "";
  bool constant_seed = false;
  int c;
  *use_hud = false;
  memset(opts, 0, sizeof(*opts));
  while((c = getopt(argc, argv, "HVhckl:r:d:f:p:")) != EOF){
    switch(c){
      case 'H':
        *use_hud = true;
        break;
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
        if(f <= 0){
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

// just fucking around...for now
int main(int argc, char** argv){
  bool use_hud;
  struct notcurses* nc;
  notcurses_options nopts;
  if(!setlocale(LC_ALL, "")){
    fprintf(stderr, "Couldn't set locale based on user preferences\n");
    return EXIT_FAILURE;
  }
  struct timespec starttime;
  clock_gettime(CLOCK_MONOTONIC_RAW, &starttime);
  sigset_t sigmask;
  sigemptyset(&sigmask);
  sigaddset(&sigmask, SIGWINCH);
  pthread_sigmask(SIG_SETMASK, &sigmask, NULL);
  const char* spec;
  if((spec = handle_opts(argc, argv, &nopts, &use_hud)) == NULL){
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
  if((nc = notcurses_init(&nopts, stdout)) == NULL){
    return EXIT_FAILURE;
  }
  if(notcurses_mouse_enable(nc)){
    goto err;
  }
  if(use_hud){
    if(hud_create(nc) == NULL){
      goto err;
    }
  }
  if(input_dispatcher(nc)){
    goto err;
  }
  int dimx, dimy;
  notcurses_term_dim_yx(nc, &dimy, &dimx);
  if(dimy < MIN_SUPPORTED_ROWS || dimx < MIN_SUPPORTED_COLS){
    goto err;
  }
  // no one cares about the leaderscreen. 1s max.
  if(demodelay.tv_sec >= 1){
    sleep(1);
  }else{
    nanosleep(&demodelay, NULL);
  }
  if(ext_demos(nc, spec) == NULL){
    goto err;
  }
  if(hud_destroy()){
    goto err;
  }
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
  bool failed = false;
  uint64_t totalbytes = 0;
  long unsigned totalframes = 0;
  uint64_t totalrenderns = 0;
  printf("\n");
  printf("      runtime│frames│output(B)│rendering│%%r│%7s║\n", "FPS");
  printf("══╤═╤════════╪══════╪═════════╪═════════╪══╪═══════╣\n");
  char timebuf[PREFIXSTRLEN + 1];
  char totalbuf[BPREFIXSTRLEN + 1];
  char rtimebuf[PREFIXSTRLEN + 1];
  uint64_t nsdelta = 0;
  for(size_t i = 0 ; i < strlen(spec) ; ++i){
    nsdelta += results[i].timens;
    qprefix(results[i].timens, GIG, timebuf, 0);
    qprefix(results[i].stats.render_ns, GIG, rtimebuf, 0);
    bprefix(results[i].stats.render_bytes, 1, totalbuf, 0);
    double avg = results[i].stats.render_ns / (double)results[i].stats.renders;
    printf("%2zu│%c│%*ss│%6lu│%*s│ %*ss│%2ld│%7.1f║%s\n", i,
           results[i].selector,
           PREFIXSTRLEN, timebuf,
           results[i].stats.renders,
           BPREFIXSTRLEN, totalbuf,
           PREFIXSTRLEN, rtimebuf,
           results[i].timens ?
            results[i].stats.render_ns * 100 / results[i].timens : 0,
           GIG / avg,
           results[i].result < 0 ? "***FAILED" :
            results[i].result > 0 ? "***ABORTED" :
             results[i].stats.renders ? ""  : "***NOT RUN");
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
  printf("══╧═╧════════╪══════╪═════════╪═════════╪══╪═══════╝\n");
  printf("     %*ss│%6lu│%*s│ %*ss│%2ld│\n", PREFIXSTRLEN, timebuf,
         totalframes, BPREFIXSTRLEN, totalbuf, PREFIXSTRLEN, rtimebuf,
         nsdelta ? totalrenderns * 100 / nsdelta : 0);
  if(failed){
    fprintf(stderr, " Error running demo. Is \"%s\" the correct data path?\n", datadir);
  }
  return failed ? EXIT_FAILURE : EXIT_SUCCESS;

err:
  notcurses_term_dim_yx(nc, &dimy, &dimx);
  notcurses_stop(nc);
  if(dimy < MIN_SUPPORTED_ROWS || dimx < MIN_SUPPORTED_COLS){
    fprintf(stderr, "At least an %dx%d terminal is required (current: %dx%d)\n",
            MIN_SUPPORTED_COLS, MIN_SUPPORTED_ROWS, dimx, dimy);
  }
  return EXIT_FAILURE;
}

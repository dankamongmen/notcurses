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

static int
intro(struct notcurses* nc){
  struct ncplane* ncp;
  if((ncp = notcurses_stdplane(nc)) == NULL){
    return -1;
  }
  cell c = CELL_TRIVIAL_INITIALIZER;
  cell_set_bg_rgb(&c, 0x20, 0x20, 0x20);
  ncplane_set_base(ncp, &c);
  int x, y, rows, cols;
  ncplane_dim_yx(ncp, &rows, &cols);
  cell ul = CELL_TRIVIAL_INITIALIZER, ur = CELL_TRIVIAL_INITIALIZER;
  cell ll = CELL_TRIVIAL_INITIALIZER, lr = CELL_TRIVIAL_INITIALIZER;
  cell hl = CELL_TRIVIAL_INITIALIZER, vl = CELL_TRIVIAL_INITIALIZER;
  if(ncplane_cursor_move_yx(ncp, 0, 0)){
    return -1;
  }
  if(cells_rounded_box(ncp, CELL_STYLE_BOLD, 0, &ul, &ur, &ll, &lr, &hl, &vl)){
    return -1;
  }
  channels_set_fg_rgb(&ul.channels, 0xff, 0, 0);
  channels_set_fg_rgb(&ur.channels, 0, 0xff, 0);
  channels_set_fg_rgb(&ll.channels, 0, 0, 0xff);
  channels_set_fg_rgb(&lr.channels, 0xff, 0xff, 0xff);
  if(ncplane_box_sized(ncp, &ul, &ur, &ll, &lr, &hl, &vl, rows, cols,
                       NCBOXGRAD_TOP | NCBOXGRAD_BOTTOM |
                        NCBOXGRAD_RIGHT | NCBOXGRAD_LEFT)){
    return -1;
  }
  cell_release(ncp, &ul); cell_release(ncp, &ur);
  cell_release(ncp, &ll); cell_release(ncp, &lr);
  cell_release(ncp, &hl); cell_release(ncp, &vl);
  const char* cstr = "Δ";
  cell_load(ncp, &c, cstr);
  cell_set_fg_rgb(&c, 200, 0, 200);
  int ys = 200 / (rows - 2);
  for(y = 5 ; y < rows - 6 ; ++y){
    cell_set_bg_rgb(&c, 0, y * ys  , 0);
    for(x = 5 ; x < cols - 6 ; ++x){
      if(ncplane_putc_yx(ncp, y, x, &c) <= 0){
        return -1;
      }
    }
  }
  cell_release(ncp, &c);
  uint64_t channels = 0;
  channels_set_fg_rgb(&channels, 90, 0, 90);
  channels_set_bg_rgb(&channels, 0, 0, 180);
  if(ncplane_cursor_move_yx(ncp, 4, 4)){
    return -1;
  }
  if(ncplane_rounded_box(ncp, 0, channels, rows - 6, cols - 6, 0)){
    return -1;
  }
  const char s1[] = " Die Welt ist alles, was der Fall ist. ";
  const char str[] = " Wovon man nicht sprechen kann, darüber muss man schweigen. ";
  if(ncplane_set_fg_rgb(ncp, 192, 192, 192)){
    return -1;
  }
  if(ncplane_set_bg_rgb(ncp, 0, 40, 0)){
    return -1;
  }
  if(ncplane_putstr_aligned(ncp, rows / 2 - 2, NCALIGN_CENTER, s1) != (int)strlen(s1)){
    return -1;
  }
  ncplane_styles_on(ncp, CELL_STYLE_ITALIC | CELL_STYLE_BOLD);
  if(ncplane_putstr_aligned(ncp, rows / 2, NCALIGN_CENTER, str) != (int)strlen(str)){
    return -1;
  }
  ncplane_styles_off(ncp, CELL_STYLE_ITALIC);
  ncplane_set_fg_rgb(ncp, 0xff, 0xff, 0xff);
  if(ncplane_printf_aligned(ncp, rows - 3, NCALIGN_CENTER, "notcurses %s. press 'q' to quit.", notcurses_version()) < 0){
    return -1;
  }
  ncplane_styles_off(ncp, CELL_STYLE_BOLD);
  const wchar_t wstr[] = L"▏▁ ▂ ▃ ▄ ▅ ▆ ▇ █ █ ▇ ▆ ▅ ▄ ▃ ▂ ▁▕";
  if(ncplane_putwstr_aligned(ncp, rows / 2 - 5, NCALIGN_CENTER, wstr) < 0){
    return -1;
  }
  if(rows < 45){
    ncplane_set_fg_rgb(ncp, 0xc0, 0, 0x80);
    ncplane_set_bg_rgb(ncp, 0x20, 0x20, 0x20);
    ncplane_styles_on(ncp, CELL_STYLE_BLINK); // heh FIXME replace with pulse
    if(ncplane_putstr_aligned(ncp, 2, NCALIGN_CENTER, "demo runs best with at least 45 lines") < 0){
      return -1;
    }
    ncplane_styles_off(ncp, CELL_STYLE_BLINK); // heh FIXME replace with pulse
  }
  if(demo_render(nc)){
    return -1;
  }
  nanosleep(&demodelay, NULL);
  struct timespec fade = demodelay;
  ncplane_fadeout(ncp, &fade, demo_fader);
  return 0;
}

static const char* demonames[26] = {
  "",
  "box",
  "chunli",
  "",
  "eagle",
  "fallin",
  "grid",
  "",
  "intro",
  "",
  "",
  "luigi",
  "",
  "",
  "outro",
  "panelreels",
  "",
  "",
  "sliders",
  "trans",
  "uniblock",
  "view",
  "witherworms",
  "xray",
  "",
  ""
};

static demoresult*
ext_demos(struct notcurses* nc, const char* demos){
  int ret = 0;
  results = malloc(sizeof(*results) * strlen(demos));
  if(results == NULL){
    return NULL;
  }
  memset(results, 0, sizeof(*results) * strlen(demos));
  democount = strlen(demos);
  struct timespec start, now;
  clock_gettime(CLOCK_MONOTONIC, &start);
  uint64_t prevns = timespec_to_ns(&start);
  for(size_t i = 0 ; i < strlen(demos) ; ++i){
    results[i].selector = demos[i];
  }
  for(size_t i = 0 ; i < strlen(demos) ; ++i){
    if(interrupted){
      break;
    }
    int nameidx = demos[i] - 'a';
    if(nameidx < 0 || nameidx > 25 || !demonames[nameidx]){
      fprintf(stderr, "Invalid demo specification: %c\n", demos[i]);
      ret = -1;
    }
    hud_schedule(demonames[nameidx]);
    switch(demos[i]){
      case 'i': ret = intro(nc); break;
      case 'o': ret = outro(nc); break;
      case 's': ret = sliding_puzzle_demo(nc); break;
      case 'u': ret = unicodeblocks_demo(nc); break;
      case 't': ret = trans_demo(nc); break;
      case 'b': ret = box_demo(nc); break;
      case 'c': ret = chunli_demo(nc); break;
      case 'g': ret = grid_demo(nc); break;
      case 'l': ret = luigi_demo(nc); break;
      case 'f': ret = fallin_demo(nc); break;
      case 'v': ret = view_demo(nc); break;
      case 'e': ret = eagle_demo(nc); break;
      case 'x': ret = xray_demo(nc); break;
      case 'w': ret = witherworm_demo(nc); break;
      case 'p': ret = panelreel_demo(nc); break;
      default:
        fprintf(stderr, "Unknown demo specification: %c\n", demos[i]);
        ret = -1;
        break;
    }
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
  const char* demos = argv[optind];
  return demos;
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
  const char* demos;
  if((demos = handle_opts(argc, argv, &nopts, &use_hud)) == NULL){
    if(argv[optind] != NULL){
      usage(*argv, EXIT_FAILURE);
    }
    demos = DEFAULT_DEMO;
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
  if(ext_demos(nc, demos) == NULL){
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
  printf("        total│frames│output(B)│rendering│%%r│%7s║\n", "FPS");
  printf("══╤═╤════════╪══════╪═════════╪═════════╪══╪═══════╣\n");
  char timebuf[PREFIXSTRLEN + 1];
  char totalbuf[BPREFIXSTRLEN + 1];
  char rtimebuf[PREFIXSTRLEN + 1];
  uint64_t nsdelta = 0;
  for(size_t i = 0 ; i < strlen(demos) ; ++i){
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

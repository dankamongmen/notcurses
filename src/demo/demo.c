#include <time.h>
#include <wchar.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <locale.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdatomic.h>
#include <notcurses/direct.h>
#include "demo.h"

// (non-)ansi terminal definition-4-life
static const int MIN_SUPPORTED_ROWS = 24;
static const int MIN_SUPPORTED_COLS = 76; // allow a bit of margin, sigh

static int democount;
static demoresult* results;
static char *datadir = NOTCURSES_SHARE;

static const char DEFAULT_DEMO[] = "ixetunchmdbkywgarvlsfjqzo";

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

float delaymultiplier = 1;

// scaled in getopt() by delaymultiplier
struct timespec demodelay = {
  .tv_sec = 1,
  .tv_nsec = 0,
};

// the "jungle" demo has non-free material embedded into it, and is thus
// entirely absent (can't just be disabled). supply a stub here.
#ifdef DFSG_BUILD
int jungle_demo(struct notcurses* nc){
  (void)nc;
  return -1;
}
#endif

// Demos can be disabled either due to a DFSG build (any non-free material must
// be disabled) or a non-multimedia build (all images/videos must be disabled).
static struct {
  const char* name;
  int (*fxn)(struct notcurses*);
  bool dfsg_disabled;             // disabled for DFSG builds
} demos[26] = {
  { "animate", animate_demo, false, },
  { "box", box_demo, false, },
  { "chunli", chunli_demo, true, },
  { "dragon", dragon_demo, false, },
  { "eagle", eagle_demo, true, },
  { "fission", fission_demo, false, },
  { "grid", grid_demo, false, },
  { "highcon", highcontrast_demo, false, },
  { "intro", intro, false, },
  { "jungle", jungle_demo, true, },
  { "keller", keller_demo, true, },
  { "luigi", luigi_demo, true, },
  { "mojibake", mojibake_demo, false, },
  { "normal", normal_demo, false, },
  { "outro", outro, false, },
  { NULL, NULL, false, }, // pango
  { "qrcode", qrcode_demo, false, }, // is blank without USE_QRCODEGEN
  { "reel", reel_demo, false, },
  { "sliders", sliding_puzzle_demo, false, },
  { "trans", trans_demo, false, },
  { "uniblock", unicodeblocks_demo, false, },
  { "view", view_demo, true, },
  { "whiteout", witherworm_demo, false, },
  { "xray", xray_demo, false, },
  { "yield", yield_demo, false, },
  { "zoo", zoo_demo, false, },
};

static void
usage_option(FILE* out, struct ncdirect* n, const char* op){
  if(n) ncdirect_set_fg_rgb8(n, 0x80, 0x80, 0x80);
  fprintf(out, " [ ");
  if(n) ncdirect_set_fg_rgb8(n, 0xff, 0xff, 0x80);
  fprintf(out, "%s", op);
  if(n) ncdirect_set_fg_rgb8(n, 0x80, 0x80, 0x80);
  fprintf(out, " ] ");
  if(n) ncdirect_set_fg_rgb8(n, 0xff, 0xff, 0xff);
}

static void
usage_expo(FILE* out, struct ncdirect* n, const char* op, const char* expo){
  if(n) ncdirect_set_fg_rgb8(n, 0xff, 0xff, 0x80);
  fprintf(out, " %s: ", op);
  if(n) ncdirect_set_fg_rgb8(n, 0xff, 0xff, 0xff);
  fprintf(out, "%s\n", expo);
}

// FIXME stylize this a little
static void
usage(const char* exe, int status){
  FILE* out = status == EXIT_SUCCESS ? stdout : stderr;
  struct ncdirect* n = ncdirect_init(NULL, out, 0);
  if(n) ncdirect_set_fg_rgb8(n, 0x00, 0xc0, 0xc0);
  fprintf(out, "usage: ");
  if(n) ncdirect_set_fg_rgb8(n, 0x80, 0xff, 0x80);
  fprintf(out, "%s ", exe);
  const char* options[] = { "-hVkc", "-m margins", "-p path", "-l loglevel",
                            "-d mult", "-J jsonfile", "demospec",
                            NULL };
  for(const char** op = options ; *op ; ++op){
    usage_option(out, n, *op);
  }
  fprintf(out, "\n\n");
  if(n) ncdirect_set_fg_rgb8(n, 0xff, 0xff, 0xff);
  const char* optexpo[] = {
    "-h|--help", "this message",
    "-V|--version", "print program name and version",
    "-k", "keep screen; do not switch to alternate",
    "-d", "delay multiplier (non-negative float)",
    "-J", "emit JSON summary to file",
    "-c", "constant PRNG seed, useful for benchmarking",
    "-m", "margin, or 4 comma-separated margins",
    NULL
  };
  for(const char** op = optexpo ; *op ; op += 2){
    const char* expo = op[1];
    usage_expo(out, n, *op, expo);
  }
  if(n) ncdirect_set_fg_rgb8(n, 0xff, 0xff, 0x80);
  fprintf(out, " -l:");
  if(n) ncdirect_set_fg_rgb8(n, 0xff, 0xff, 0xff);
  fprintf(out, " logging level (%d: silent..%d: manic)\n", NCLOGLEVEL_SILENT, NCLOGLEVEL_TRACE);
  if(n) ncdirect_set_fg_rgb8(n, 0xff, 0xff, 0x80);
  fprintf(out, " -p:");
  if(n) ncdirect_set_fg_rgb8(n, 0xff, 0xff, 0xff);
  fprintf(out, " data file path (default: %s)\n", NOTCURSES_SHARE);
  fprintf(out, "\nspecify demos via their first letter. repetitions are allowed.\n");
  if(n) ncdirect_set_fg_rgb8(n, 0x80, 0xff, 0x80);
  fprintf(out, " default spec: %s\n\n", DEFAULT_DEMO);
  if(n) ncdirect_set_fg_rgb8(n, 0xff, 0xff, 0xff);
  int printed = 0;
  for(size_t i = 0 ; i < sizeof(demos) / sizeof(*demos) ; ++i){
    if(demos[i].name){
      if(printed % 5 == 0){
        fprintf(out, " ");
      }
      // U+24D0: CIRCLED LATIN SMALL LETTER A
      if(n) ncdirect_set_fg_rgb8(n, 0xff, 0xff, 0x80);
      fprintf(out, "%lc ", *demos[i].name - 'a' + 0x24d0);
      if(n) ncdirect_set_fg_rgb8(n, 0xff, 0xff, 0xff);
      fprintf(out, "%-*.*s", 8, 8, demos[i].name + 1);
      if(++printed % 5 == 0){
        fprintf(out, "\n");
      }
    }
  }
  if(printed % 5){
    fprintf(out, "\n");
  }
  ncdirect_stop(n);
  exit(status);
}

static int
ext_demos(struct notcurses* nc, const char* spec){
  int ret = 0;
  results = malloc(sizeof(*results) * strlen(spec));
  if(results == NULL){
    return -1;
  }
  memset(results, 0, sizeof(*results) * strlen(spec));
  democount = strlen(spec);
  struct timespec start, now;
  clock_gettime(CLOCK_MONOTONIC, &start);
  uint64_t prevns = timespec_to_ns(&start);
  for(size_t i = 0 ; i < strlen(spec) ; ++i){
    results[i].selector = spec[i];
  }
  struct ncplane* n = notcurses_stdplane(nc);
  for(size_t i = 0 ; i < strlen(spec) ; ++i){
    if(interrupted){
      break;
    }
    int idx = spec[i] - 'a';
#ifdef DFSG_BUILD
    if(demos[idx].dfsg_disabled){
      continue;
    }
#endif
    // set the standard plane's base character to an opaque black, but don't
    // erase the plane (we let one demo bleed through to the next, an effect
    // we exploit in a few transitions).
    uint64_t stdc = NCCHANNELS_INITIALIZER(0, 0, 0, 0, 0, 0);
    ncplane_set_base(n, "", 0, stdc);

    hud_schedule(demos[idx].name);
    ret = demos[idx].fxn(nc);
    notcurses_stats_reset(nc, &results[i].stats);
    clock_gettime(CLOCK_MONOTONIC, &now);
    uint64_t nowns = timespec_to_ns(&now);
    results[i].timens = nowns - prevns;
    prevns = nowns;
    results[i].result = ret;
    hud_completion_notify(&results[i]);
    if(ret){
      break;
    }
  }
  return 0;
}

// returns the demos to be run as a string. on error, returns NULL. on no
// specification, also returns NULL, heh. determine this by argv[optind];
// if it's NULL, there were valid options, but no spec.
static const char*
handle_opts(int argc, char** argv, notcurses_options* opts, FILE** json_output){
  bool constant_seed = false;
  *json_output = NULL;
  int c;
  const struct option longopts[] = {
    { .name = "help", .has_arg = 0, .flag = NULL, .val = 'h', },
    { .name = "version", .has_arg = 0, .flag = NULL, .val = 'V', },
    { .name = NULL, .has_arg = 0, .flag = NULL, .val = 0, },
  };
  int lidx;
  while((c = getopt_long(argc, argv, "VhckJ:l:d:p:m:", longopts, &lidx)) != EOF){
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
      }case 'm':{
        if(opts->margin_t || opts->margin_r || opts->margin_b || opts->margin_l){
          fprintf(stderr, "Provided margins twice!\n");
          usage(*argv, EXIT_FAILURE);
        }
        if(notcurses_lex_margins(optarg, opts)){
          usage(*argv, EXIT_FAILURE);
        }
        break;
      }case 'V':
        printf("notcurses-demo version %s\n", notcurses_version());
        exit(EXIT_SUCCESS);
      case 'J':
        if(*json_output){
          fprintf(stderr, "Supplied -J twice: %s\n", optarg);
          usage(*argv, EXIT_FAILURE);
        }
        if((*json_output = fopen(optarg, "wb")) == NULL){
          fprintf(stderr, "Error opening %s for JSON (%s?)\n", optarg, strerror(errno));
          usage(*argv, EXIT_FAILURE);
        }
        break;
      case 'c':
        constant_seed = true;
        break;
      case 'k':
        opts->flags |= NCOPTION_NO_ALTERNATE_SCREEN;
        break;
      case 'p':
        datadir = optarg;
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
        uint64_t ns = f * NANOSECS_IN_SEC;
        demodelay.tv_sec = ns / NANOSECS_IN_SEC;
        demodelay.tv_nsec = ns % NANOSECS_IN_SEC;
        break;
      }default:
        usage(*argv, EXIT_FAILURE);
    }
  }
  if(!constant_seed){
    srand(time(NULL)); // a classic blunder lol
  }
  if(optind < argc - 1){
    fprintf(stderr, "Extra argument: %s\n", argv[optind + 1]);
    usage(*argv, EXIT_FAILURE);
  }
  const char* spec = argv[optind];
  return spec;
}

static int
table_segment_color(struct ncplane* n, const char* str, const char* delim, unsigned color){
  ncplane_set_fg_rgb(n, color);
  if(ncplane_putstr(n, str) < 0){
    return -1;
  }
  ncplane_set_fg_rgb8(n, 178, 102, 255);
  if(ncplane_putstr(n, delim) < 0){
    return -1;
  }
  return 0;
}

static int
table_segment(struct ncplane* n, const char* str, const char* delim){
  return table_segment_color(n, str, delim, 0xffffff);
}

static int
table_printf(struct ncplane* n, const char* delim, const char* fmt, ...){
  ncplane_set_fg_rgb8(n, 0xD4, 0xAF, 0x37);
  va_list va;
  va_start(va, fmt);
  int r = ncplane_vprintf(n, fmt, va);
  va_end(va);
  ncplane_set_fg_rgb8(n, 178, 102, 255);
  ncplane_putstr(n, delim);
  return r;
}

static int
summary_json(FILE* f, const char* spec, int rows, int cols){
  int ret = 0;
  ret |= (fprintf(f, "{\"notcurses-demo\":{\"spec\":\"%s\",\"TERM\":\"%s\",\"rows\":\"%d\",\"cols\":\"%d\",\"runs\":{",
                  spec, getenv("TERM"), rows, cols) < 0);
  for(size_t i = 0 ; i < strlen(spec) ; ++i){
    if(results[i].result || !results[i].stats.renders){
      continue;
    }
    ret |= (fprintf(f, "\"%s\":{\"bytes\":\"%"PRIu64"\",\"frames\":\"%"PRIu64"\",\"ns\":\"%"PRIu64"\"}%s",
                    demos[results[i].selector - 'a'].name, results[i].stats.render_bytes,
                    results[i].stats.renders, results[i].timens, i < strlen(spec) - 1 ? "," : "") < 0);
  }
  ret |= (fprintf(f, "}}}\n") < 0);
  return ret;
}

static int
summary_table(struct notcurses* nc, const char* spec, bool canimage, bool canvideo){
  notcurses_leave_alternate_screen(nc);
  struct ncplane* n = notcurses_stdplane(nc);
  ncplane_set_scrolling(n, true);
  bool failed = false;
  uint64_t totalbytes = 0;
  long unsigned totalframes = 0;
  uint64_t totalrenderns = 0;
  uint64_t totalwriteoutns = 0;
  ncplane_putchar(n, '\n');
  table_segment(n, "             runtime", "│");
  table_segment(n, " frames", "│");
  table_segment(n, "output(B)", "│");
  table_segment(n, "    FPS", "│");
  table_segment(n, "%r", "│");
  table_segment(n, "%a", "│");
  table_segment(n, "%w", "│");
  table_segment(n, "TheoFPS", "║\n══╤════════╤════════╪═══════╪═════════╪═══════╪══╪══╪══╪═══════╣\n");
  char timebuf[PREFIXSTRLEN + 1];
  char tfpsbuf[PREFIXSTRLEN + 1];
  char totalbuf[BPREFIXSTRLEN + 1];
  uint64_t nsdelta = 0;
  for(size_t i = 0 ; i < strlen(spec) ; ++i){
    nsdelta += results[i].timens;
    qprefix(results[i].timens, NANOSECS_IN_SEC, timebuf, 0);
    bprefix(results[i].stats.render_bytes, 1, totalbuf, 0);
    uint64_t divisor = results[i].stats.render_ns + results[i].stats.writeout_ns + results[i].stats.raster_ns;
    if(divisor){
      qprefix((uintmax_t)results[i].stats.writeouts * NANOSECS_IN_SEC * 1000 / divisor,
              1000, tfpsbuf, 0);
    }else{
      qprefix(0, NANOSECS_IN_SEC, tfpsbuf, 0);
    }
    uint32_t rescolor;
    if(results[i].result < 0){
      rescolor = 0xff303c;
    }else if(results[i].result > 0){
      rescolor = 0xffaa22;
    }else if(!results[i].stats.renders){
      rescolor = 0xbbbbbb;
    }else{
      rescolor = 0x32CD32;
    }
    ncplane_set_fg_rgb(n, rescolor);
    ncplane_printf(n, "%2llu", (unsigned long long)(i + 1)); // windows has %zu problems
    ncplane_set_fg_rgb8(n, 178, 102, 255);
    ncplane_putwc(n, L'│');
    ncplane_set_fg_rgb(n, rescolor);
    ncplane_printf(n, "%8s", demos[results[i].selector - 'a'].name);
    ncplane_set_fg_rgb8(n, 178, 102, 255);
    ncplane_printf(n, "│%*ss│%7ju│%*s│%7.1f│%2jd│%2jd│%2jd│%*s║",
           PREFIXFMT(timebuf), (uintmax_t)(results[i].stats.renders),
           BPREFIXFMT(totalbuf),
           results[i].timens ?
            results[i].stats.renders / ((double)results[i].timens / NANOSECS_IN_SEC) : 0.0,
           (uintmax_t)(results[i].timens ?
            results[i].stats.render_ns * 100 / results[i].timens : 0),
           (uintmax_t)(results[i].timens ?
            results[i].stats.raster_ns * 100 / results[i].timens : 0),
           (uintmax_t)(results[i].timens ?
            results[i].stats.writeout_ns * 100 / results[i].timens : 0),
           PREFIXFMT(tfpsbuf));
    ncplane_set_fg_rgb(n, rescolor);
    ncplane_printf(n, "%s\n", results[i].result < 0 ? "FAILED" :
                   results[i].result > 0 ? "ABORTED" :
                   !results[i].stats.renders ? "SKIPPED"  : "");
    if(results[i].result < 0){
      failed = true;
    }
    totalframes += results[i].stats.renders;
    totalbytes += results[i].stats.render_bytes;
    totalrenderns += results[i].stats.render_ns;
    totalwriteoutns += results[i].stats.writeout_ns;
  }
  qprefix(nsdelta, NANOSECS_IN_SEC, timebuf, 0);
  bprefix(totalbytes, 1, totalbuf, 0);
  table_segment(n, "", "══╧════════╧════════╪═══════╪═════════╪═══════╧══╧══╧══╧═══════╝\n");
  ncplane_putstr(n, "            ");
  table_printf(n, "│", "%*ss", PREFIXFMT(timebuf));
  table_printf(n, "│", "%7lu", totalframes);
  table_printf(n, "│", "%*s", BPREFIXFMT(totalbuf));
  //table_printf(nc, "│", "%7.1f", nsdelta ? totalframes / ((double)nsdelta / NANOSECS_IN_SEC) : 0);
  ncplane_putchar(n, '\n');
  ncplane_set_fg_rgb8(n, 0xfe, 0x20, 0x76); // PANTONE Strong Red C + 3x0x20
#ifdef DFSG_BUILD
  ncplane_putstr(n, "\nDFSG version. Some demos are unavailable.\n");
#endif
  if(!canimage){
    ncplane_putstr(n, "\nNo multimedia support. Some demos are unavailable.\n");
  }else if(!canvideo){
    ncplane_putstr(n, "\nNo video support. Some demos are unavailable.\n");
  }
  ncplane_set_fg_rgb8(n, 0xff, 0xb0, 0xb0);
  if(failed){
    ncplane_printf(n, "\nError running demo.\nIs \"%s\" the correct data path? Supply it with -p.\n", datadir);
  }
  return failed;
}

static int
scrub_stdplane(struct notcurses* nc){
  struct ncplane* n = notcurses_stdplane(nc);
  uint64_t channels = 0;
  ncchannels_set_fg_rgb(&channels, 0); // explicit black + opaque
  ncchannels_set_bg_rgb(&channels, 0);
  if(ncplane_set_base(n, "", 0, channels)){
    return -1;
  }
  ncplane_erase(n);
  return 0;
}

int main(int argc, char** argv){
  sigset_t sigmask;
  // ensure SIGWINCH is delivered only to a thread doing input
  sigemptyset(&sigmask);
  sigaddset(&sigmask, SIGWINCH);
  pthread_sigmask(SIG_BLOCK, &sigmask, NULL);
  const char* spec;
  FILE* json = NULL; // emit JSON summary to this file? (-J)
  notcurses_options nopts = {};
  if((spec = handle_opts(argc, argv, &nopts, &json)) == NULL){
    if(argv[optind] != NULL){
      usage(*argv, EXIT_FAILURE);
    }
    spec = DEFAULT_DEMO;
  }
  for(size_t i = 0 ; i < strlen(spec) ; ++i){
    int nameidx = spec[i] - 'a';
    if(nameidx < 0 || nameidx > 25 || !demos[nameidx].name){
      fprintf(stderr, "Invalid demo specification: %c\n", spec[i]);
      usage(*argv, EXIT_FAILURE);
      return EXIT_FAILURE;
    }
  }
  struct timespec starttime;
  clock_gettime(CLOCK_MONOTONIC, &starttime);
  struct notcurses* nc;
  if((nc = notcurses_init(&nopts, NULL)) == NULL){
    return EXIT_FAILURE;
  }
  notcurses_mouse_enable(nc);
  const bool canimage = notcurses_canopen_images(nc);
  const bool canvideo = notcurses_canopen_videos(nc);
  int dimx, dimy;
  ncplane_dim_yx(notcurses_stdplane(nc), &dimy, &dimx);
  if(input_dispatcher(nc)){
    goto err;
  }
  if(dimy < MIN_SUPPORTED_ROWS || dimx < MIN_SUPPORTED_COLS){
    goto err;
  }
  if((nopts.flags & NCOPTION_NO_ALTERNATE_SCREEN)){ // no one cares. 1s max.
    if(demodelay.tv_sec >= 1){
      sleep(1);
    }else{
      nanosleep(&demodelay, NULL);
    }
  }
  struct ncmenu* menu = NULL;
  do{
    restart_demos = false;
    interrupted = false;
    notcurses_drop_planes(nc);
    if(scrub_stdplane(nc)){
      goto err;
    }
    if(!hud_create(nc)){
      goto err;
    }
    if(fpsgraph_init(nc)){
      goto err;
    }
    if((menu = menu_create(nc)) == NULL){
      goto err;
    }
    if(notcurses_render(nc)){
      goto err;
    }
    notcurses_stats_reset(nc, NULL);
    if(ext_demos(nc, spec)){
      goto err;
    }
    if(hud_destroy()){ // destroy here since notcurses_drop_planes will kill it
      goto err;
    }
    if(fpsgraph_stop()){
      goto err;
    }
    about_destroy(nc); // also kills debug window
  }while(restart_demos);
  ncmenu_destroy(menu);
  stop_input();
  notcurses_render(nc); // rid ourselves of any remaining demo output
  if(summary_table(nc, spec, canimage, canvideo)){
    goto err;
  }
  notcurses_render(nc); // render our summary table
  free(results);
  if(notcurses_stop(nc)){
    return EXIT_FAILURE;
  }
  if(json && summary_json(json, spec, dimy, dimx)){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;

err:
  stop_input();
  notcurses_stop(nc);
  if(dimy < MIN_SUPPORTED_ROWS || dimx < MIN_SUPPORTED_COLS){
    fprintf(stderr, "At least a %dx%d terminal is required (current: %dx%d)\n",
            MIN_SUPPORTED_ROWS, MIN_SUPPORTED_COLS, dimy, dimx);
  }
  return EXIT_FAILURE;
}

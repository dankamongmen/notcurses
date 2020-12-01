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
#include <stdatomic.h>
#include <notcurses/direct.h>
#include "demo.h"

// (non-)ansi terminal definition-4-life
static const int MIN_SUPPORTED_ROWS = 24;
static const int MIN_SUPPORTED_COLS = 76; // allow a bit of margin, sigh

static int democount;
static demoresult* results;
static char *datadir = NOTCURSES_SHARE;

static const char DEFAULT_DEMO[] = "ixezcydthnmbkgarwuvlsfjqo";

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
  { "allglyph", allglyphs_demo, false, },
  { "box", box_demo, false, },
  { "chunli", chunli_demo, true, },
  { "dragon", dragon_demo, false, },
  { "eagle", eagle_demo, true, },
  { "fallin'", fallin_demo, false, },
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
  if(n) ncdirect_fg_rgb8(n, 0x80, 0x80, 0x80);
  fprintf(out, " [ ");
  if(n) ncdirect_fg_rgb8(n, 0xff, 0xff, 0x80);
  fprintf(out, "%s", op);
  if(n) ncdirect_fg_rgb8(n, 0x80, 0x80, 0x80);
  fprintf(out, " ] ");
  if(n) ncdirect_fg_rgb8(n, 0xff, 0xff, 0xff);
}

static void
usage_expo(FILE* out, struct ncdirect* n, const char* op, const char* expo){
  if(n) ncdirect_fg_rgb8(n, 0xff, 0xff, 0x80);
  fprintf(out, " %s: ", op);
  if(n) ncdirect_fg_rgb8(n, 0xff, 0xff, 0xff);
  fprintf(out, "%s\n", expo);
}

// FIXME stylize this a little
static void
usage(const char* exe, int status){
  FILE* out = status == EXIT_SUCCESS ? stdout : stderr;
  struct ncdirect* n = ncdirect_init(NULL, out, 0);
  if(n) ncdirect_fg_rgb8(n, 0x00, 0xc0, 0xc0);
  fprintf(out, "usage: ");
  if(n) ncdirect_fg_rgb8(n, 0x80, 0xff, 0x80);
  fprintf(out, "%s ", exe);
  const char* options[] = { "-hVikc", "-m margins", "-p path", "-l loglevel",
                            "-d mult", "-J jsonfile", "-f renderfile", "demospec",
                            NULL };
  for(const char** op = options ; *op ; ++op){
    usage_option(out, n, *op);
  }
  fprintf(out, "\n\n");
  if(n) ncdirect_fg_rgb8(n, 0xff, 0xff, 0xff);
  const char* optexpo[] = {
    "-h", "this message",
    "-V", "print program name and version",
    "-i", "ignore failures, keep going",
    "-k", "keep screen; do not switch to alternate",
    "-d", "delay multiplier (non-negative float)",
    "-J", "emit JSON summary to file",
    "-f", "render to file (in addition to stdout)",
    "-c", "constant PRNG seed, useful for benchmarking",
    "-m", "margin, or 4 comma-separated margins",
    NULL
  };
  for(const char** op = optexpo ; *op ; op += 2){
    const char* expo = op[1];
    usage_expo(out, n, *op, expo);
  }
  if(n) ncdirect_fg_rgb8(n, 0xff, 0xff, 0x80);
  fprintf(out, " -l:");
  if(n) ncdirect_fg_rgb8(n, 0xff, 0xff, 0xff);
  fprintf(out, " logging level (%d: silent..%d: manic)\n", NCLOGLEVEL_SILENT, NCLOGLEVEL_TRACE);
  if(n) ncdirect_fg_rgb8(n, 0xff, 0xff, 0x80);
  fprintf(out, " -p:");
  if(n) ncdirect_fg_rgb8(n, 0xff, 0xff, 0xff);
  fprintf(out, " data file path (default: %s)\n", NOTCURSES_SHARE);
  fprintf(out, "\nspecify demos via their first letter. repetitions are allowed.\n");
  if(n) ncdirect_fg_rgb8(n, 0x80, 0xff, 0x80);
  fprintf(out, " default spec: %s\n\n", DEFAULT_DEMO);
  if(n) ncdirect_fg_rgb8(n, 0xff, 0xff, 0xff);
  int printed = 0;
  for(size_t i = 0 ; i < sizeof(demos) / sizeof(*demos) ; ++i){
    if(demos[i].name){
      if(printed % 5 == 0){
        fprintf(out, " ");
      }
      // U+24D0: CIRCLED LATIN SMALL LETTER A
      if(n) ncdirect_fg_rgb8(n, 0xff, 0xff, 0x80);
      fprintf(out, "%lc ", *demos[i].name - 'a' + 0x24d0);
      if(n) ncdirect_fg_rgb8(n, 0xff, 0xff, 0xff);
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
ext_demos(struct notcurses* nc, const char* spec, bool ignore_failures){
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
    cell c = CELL_TRIVIAL_INITIALIZER;
    cell_set_fg_rgb8(&c, 0, 0, 0);
    cell_set_bg_rgb8(&c, 0, 0, 0);
    ncplane_set_base_cell(n, &c);
    cell_release(n, &c);

    hud_schedule(demos[idx].name);
    ret = demos[idx].fxn(nc);
    notcurses_stats_reset(nc, &results[i].stats);
    clock_gettime(CLOCK_MONOTONIC, &now);
    uint64_t nowns = timespec_to_ns(&now);
    results[i].timens = nowns - prevns;
    prevns = nowns;
    results[i].result = ret;
    hud_completion_notify(&results[i]);
    if(ret && !ignore_failures){
      break;
    }
  }
  return 0;
}

// returns the demos to be run as a string. on error, returns NULL. on no
// specification, also returns NULL, heh. determine this by argv[optind];
// if it's NULL, there were valid options, but no spec.
static const char*
handle_opts(int argc, char** argv, notcurses_options* opts,
            bool* ignore_failures, FILE** json_output){
  bool constant_seed = false;
  *ignore_failures = false;
  char *renderfile = NULL;
  *json_output = NULL;
  int c;
  memset(opts, 0, sizeof(*opts));
  const struct option longopts[] = {
    { .name = "help", .has_arg = 0, .flag = NULL, .val = 'h', },
    { .name = NULL, .has_arg = 0, .flag = NULL, .val = 0, },
  };
  int lidx;
  while((c = getopt_long(argc, argv, "VhickJ:l:r:d:f:p:m:", longopts, &lidx)) != EOF){
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
      case 'i':
        *ignore_failures = true;
        break;
      case 'k':
        opts->flags |= NCOPTION_NO_ALTERNATE_SCREEN;
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
        datadir = optarg;
        break;
      case 'r':
        renderfile = optarg;
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
  if(renderfile){
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
  ncdirect_fg_rgb(nc, color);
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
summary_json(FILE* f, const char* spec, int rows, int cols){
  int ret = 0;
  ret |= (fprintf(f, "{\"notcurses-demo\":{\"spec\":\"%s\",\"TERM\":\"%s\",\"rows\":\"%d\",\"cols\":\"%d\",\"runs\":{",
                  spec, getenv("TERM"), rows, cols) < 0);
  for(size_t i = 0 ; i < strlen(spec) ; ++i){
    if(results[i].result || !results[i].stats.renders){
      continue;
    }
    ret |= (fprintf(f, "\"%s\":{\"bytes\":\"%ju\",\"frames\":\"%ju\",\"ns\":\"%ju\"}%s",
                    demos[results[i].selector - 'a'].name, results[i].stats.render_bytes,
                    results[i].stats.renders, results[i].timens, i < strlen(spec) - 1 ? "," : "") < 0);
  }
  ret |= (fprintf(f, "}}}\n") < 0);
  return ret;
}

static int
summary_table(struct ncdirect* nc, const char* spec, bool canimage, bool canvideo){
  bool failed = false;
  uint64_t totalbytes = 0;
  long unsigned totalframes = 0;
  uint64_t totalrenderns = 0;
  uint64_t totalwriteoutns = 0;
  printf("\n");
  table_segment(nc, "             runtime", "│");
  table_segment(nc, " frames", "│");
  table_segment(nc, "output(B)", "│");
  table_segment(nc, "rendering", "│");
  table_segment(nc, "%r", "│");
  table_segment(nc, "%w", "│");
  table_segment(nc, "    FPS", "│");
  table_segment(nc, "TheoFPS", "║\n══╤════════╤════════╪═══════╪═════════╪═════════╪══╪══╪═══════╪═══════╣\n");
  char timebuf[PREFIXSTRLEN + 1];
  char tfpsbuf[PREFIXSTRLEN + 1];
  char totalbuf[BPREFIXSTRLEN + 1];
  char rtimebuf[PREFIXSTRLEN + 1];
  uint64_t nsdelta = 0;
  for(size_t i = 0 ; i < strlen(spec) ; ++i){
    nsdelta += results[i].timens;
    qprefix(results[i].timens, GIG, timebuf, 0);
    qprefix(results[i].stats.render_ns, GIG, rtimebuf, 0);
    bprefix(results[i].stats.render_bytes, 1, totalbuf, 0);
    if(results[i].stats.renders){
      qprefix((uintmax_t)results[i].stats.renders * GIG * 1000 / results[i].stats.render_ns,
              1000, tfpsbuf, 0);
    }else{
      qprefix(0, GIG, tfpsbuf, 0);
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
    ncdirect_fg_rgb(nc, rescolor);
    printf("%2zu", i);
    ncdirect_fg_rgb8(nc, 178, 102, 255);
    printf("│");
    ncdirect_fg_rgb(nc, rescolor);
    printf("%8s", demos[results[i].selector - 'a'].name);
    ncdirect_fg_rgb8(nc, 178, 102, 255);
    printf("│%*ss│%7ju│%*s│ %*ss│%2jd│%2jd│%7.1f│%*s║",
           PREFIXFMT(timebuf), (uintmax_t)(results[i].stats.renders),
           BPREFIXFMT(totalbuf), PREFIXFMT(rtimebuf),
           (uintmax_t)(results[i].timens ?
            results[i].stats.render_ns * 100 / results[i].timens : 0),
           (uintmax_t)(results[i].timens ?
            results[i].stats.writeout_ns * 100 / results[i].timens : 0),
           results[i].timens ?
            results[i].stats.renders / ((double)results[i].timens / GIG) : 0.0,
           PREFIXFMT(tfpsbuf));
    ncdirect_fg_rgb(nc, rescolor);
    printf("%s\n", results[i].result < 0 ? "FAILED" :
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
  qprefix(nsdelta, GIG, timebuf, 0);
  bprefix(totalbytes, 1, totalbuf, 0);
  qprefix(totalrenderns, GIG, rtimebuf, 0);
  table_segment(nc, "", "══╧════════╧════════╪═══════╪═════════╪═════════╪══╪══╪═══════╪═══════╝\n");
  printf("            ");
  table_printf(nc, "│", "%*ss", PREFIXFMT(timebuf));
  table_printf(nc, "│", "%7lu", totalframes);
  table_printf(nc, "│", "%*s", BPREFIXFMT(totalbuf));
  table_printf(nc, "│", " %*ss", PREFIXFMT(rtimebuf));
  table_printf(nc, "│", "%2ld", nsdelta ? totalrenderns * 100 / nsdelta : 0);
  table_printf(nc, "│", "%2ld", nsdelta ? totalwriteoutns * 100 / nsdelta : 0);
  table_printf(nc, "│", "%7.1f", nsdelta ? totalframes / ((double)nsdelta / GIG) : 0);
  printf("\n");
  ncdirect_fg_rgb8(nc, 0xff, 0xb0, 0xb0);
  fflush(stdout); // in case we print to stderr below, we want color from above
  if(failed){
    fprintf(stderr, "\nError running demo.\nIs \"%s\" the correct data path? Supply it with -p.\n", datadir);
  }
  ncdirect_fg_rgb8(nc, 0xfe, 0x20, 0x76); // PANTONE Strong Red C + 3x0x20
  fflush(stdout); // in case we print to stderr below, we want color from above
#ifdef DFSG_BUILD
  fprintf(stderr, "\nDFSG version. Some demos are unavailable.\n");
#endif
  if(!canimage){
    fprintf(stderr, "\nNo multimedia support. Some demos are unavailable.\n");
  }else if(!canvideo){
    fprintf(stderr, "\nNo video support. Some demos are unavailable.\n");
  }
  return failed;
}

static int
scrub_stdplane(struct notcurses* nc){
  struct ncplane* n = notcurses_stdplane(nc);
  uint64_t channels = 0;
  channels_set_fg_rgb(&channels, 0); // explicit black + opaque
  channels_set_bg_rgb(&channels, 0);
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
  pthread_sigmask(SIG_SETMASK, &sigmask, NULL);
  const char* spec;
  FILE* json = NULL; // emit JSON summary to this file? (-J)
  bool ignore_failures; // continue after a failure? (-k)
  notcurses_options nopts;
  if((spec = handle_opts(argc, argv, &nopts, &ignore_failures, &json)) == NULL){
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
  clock_gettime(CLOCK_MONOTONIC, &starttime);
  struct notcurses* nc;
  if((nc = notcurses_init(&nopts, NULL)) == NULL){
    return EXIT_FAILURE;
  }
  const bool canimage = notcurses_canopen_images(nc);
  const bool canvideo = notcurses_canopen_videos(nc);
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
    if(ext_demos(nc, spec, ignore_failures)){
      goto err;
    }
    if(hud_destroy()){ // destroy here since notcurses_drop_planes will kill it
      goto err;
    }
    if(fpsgraph_stop(nc)){
      goto err;
    }
    about_destroy(nc);
  }while(restart_demos);
  ncmenu_destroy(menu);
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
  struct ncdirect* ncd = ncdirect_init(NULL, stdout, 0);
  if(!ncd){
    return EXIT_FAILURE;
  }
  if(json && summary_json(json, spec, ncdirect_dim_y(ncd), ncdirect_dim_x(ncd))){
    return EXIT_FAILURE;
  }
  // reinitialize without alternate screen to do some coloring
  if(summary_table(ncd, spec, canimage, canvideo)){
    ncdirect_stop(ncd);
    return EXIT_FAILURE;
  }
  free(results);
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

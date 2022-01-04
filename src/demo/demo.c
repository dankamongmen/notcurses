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
#include "demo.h"

// (non-)ansi terminal definition-4-life
static const unsigned MIN_SUPPORTED_ROWS = 24;
static const unsigned MIN_SUPPORTED_COLS = 76; // allow a bit of margin, sigh

static char *datadir;
static int democount;
static demoresult* results;

static const char DEFAULT_DEMO[] = "ixetunchdmbkywjgarvlsfqzo";

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
  return notcurses_data_path(datadir, datum);
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
int jungle_demo(struct notcurses* nc, uint64_t startns){
  (void)nc;
  (void)startns;
  return -1;
}
#endif

// Demos can be disabled either due to a DFSG build (any non-free material must
// be disabled) or a non-multimedia build (all images/videos must be disabled).
static struct {
  const char* name;
  int (*fxn)(struct notcurses*, uint64_t startns);
  bool dfsg_disabled;             // disabled for DFSG builds
} demos[26] = {
  { "animate", animate_demo, false, },
  { "box", box_demo, false, },
  { "chunli", chunli_demo, true, },
  { "dragon", dragon_demo, false, },
  { "eagle", eagle_demo, true, },
  { "fission", fission_demo, false, },
  { "grid", grid_demo, false, },
  { "highcon", highcon_demo, false, },
  { "intro", intro_demo, false, },
  { "jungle", jungle_demo, true, },
  { "keller", keller_demo, true, },
  { "luigi", luigi_demo, true, },
  { "mojibake", mojibake_demo, false, },
  { "normal", normal_demo, false, },
  { "outro", outro_demo, false, },
  { NULL, NULL, false, }, // it's a secret to everyone
  { "qrcode", qrcode_demo, false, }, // is blank without USE_QRCODEGEN
  { "reel", reel_demo, false, },
  { "sliders", sliders_demo, false, },
  { "trans", trans_demo, false, },
  { "uniblock", uniblock_demo, false, },
  { "view", view_demo, true, },
  { "whiteout", whiteout_demo, false, },
  { "xray", xray_demo, false, },
  { "yield", yield_demo, false, },
  { "zoo", zoo_demo, false, },
};

static void
usage_option(struct ncplane* n, const char* op){
  ncplane_set_fg_rgb8(n, 0x80, 0x80, 0x80);
  ncplane_printf(n, " [ ");
  ncplane_set_fg_rgb8(n, 0xff, 0xff, 0x80);
  ncplane_printf(n, "%s", op);
  ncplane_set_fg_rgb8(n, 0x80, 0x80, 0x80);
  ncplane_printf(n, " ] ");
  ncplane_set_fg_rgb8(n, 0xff, 0xff, 0xff);
}

static void
usage_expo(struct ncplane* n, const char* op, const char* expo){
  ncplane_set_fg_rgb8(n, 0xff, 0xff, 0x80);
  ncplane_printf(n, " %s: ", op);
  ncplane_set_fg_rgb8(n, 0xff, 0xff, 0xff);
  ncplane_printf(n, "%s\n", expo);
}

static void
usage(const char* exe, int status){
  FILE* out = status == EXIT_SUCCESS ? stdout : stderr;
  struct notcurses_options opts = {0};
  opts.flags = NCOPTION_CLI_MODE
               | NCOPTION_DRAIN_INPUT
               | NCOPTION_SUPPRESS_BANNERS;
  struct notcurses* nc = notcurses_init(&opts, out);
  if(!nc){
    exit(status);
  }
  struct ncplane* n = notcurses_stdplane(nc);
  ncplane_set_fg_rgb8(n, 0x00, 0xc0, 0xc0);
  ncplane_putstr(n, "usage: ");
  ncplane_set_fg_rgb8(n, 0x80, 0xff, 0x80);
  ncplane_printf(n, "%s ", exe);
  const char* options[] = { "-hVkc", "-m margins", "-p path", "-l loglevel",
                            "-d mult", "-J jsonfile", "demospec",
                            NULL };
  for(const char** op = options ; *op ; ++op){
    usage_option(n, *op);
  }
  ncplane_putstr(n, "\n\n");
  ncplane_set_fg_rgb8(n, 0xff, 0xff, 0xff);
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
    usage_expo(n, *op, expo);
  }
  ncplane_set_fg_rgb8(n, 0xff, 0xff, 0x80);
  ncplane_printf(n, " -l:");
  ncplane_set_fg_rgb8(n, 0xff, 0xff, 0xff);
  ncplane_printf(n, " logging level (%d: silent..%d: manic)\n", NCLOGLEVEL_SILENT, NCLOGLEVEL_TRACE);
  ncplane_set_fg_rgb8(n, 0xff, 0xff, 0x80);
  ncplane_printf(n, " -p:");
  ncplane_set_fg_rgb8(n, 0xff, 0xff, 0xff);
  ncplane_printf(n, " data file path (default: %s)\n", notcurses_data_dir());
  ncplane_printf(n, "\nspecify demos via their first letter. repetitions are allowed.\n");
  ncplane_set_fg_rgb8(n, 0x80, 0xff, 0x80);
  ncplane_printf(n, " default spec: %s\n\n", DEFAULT_DEMO);
  ncplane_set_fg_rgb8(n, 0xff, 0xff, 0xff);
  int printed = 0;
  for(size_t i = 0 ; i < sizeof(demos) / sizeof(*demos) ; ++i){
    if(demos[i].name){
      if(printed % 5 == 0){
        ncplane_printf(n, " ");
      }
      // U+24D0: CIRCLED LATIN SMALL LETTER A
      ncplane_set_fg_rgb8(n, 0xff, 0xff, 0x80);
      ncplane_printf(n, "%lc ", (wint_t)(*demos[i].name - 'a' + 0x24d0));
      ncplane_set_fg_rgb8(n, 0xff, 0xff, 0xff);
      ncplane_printf(n, "%-*.*s", 8, 8, demos[i].name + 1);
      if(++printed % 5 == 0){
        ncplane_printf(n, "\n");
      }
    }
  }
  if(printed % 5){
    ncplane_printf(n, "\n");
  }
  notcurses_render(nc);
  notcurses_stop(nc);
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
  uint64_t prevns = clock_getns(CLOCK_MONOTONIC);
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

    hud_schedule(demos[idx].name, prevns);
    ret = demos[idx].fxn(nc, prevns);
    notcurses_stats_reset(nc, &results[i].stats);
    uint64_t nowns = clock_getns(CLOCK_MONOTONIC);
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
        datadir = strdup(optarg);
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
  if(datadir == NULL){
    datadir = notcurses_data_dir();
  }
  const char* spec = argv[optind];
  return spec;
}

static int
table_segment_color(struct ncplane* n, const char* str, const char* delim,
                    const char* ascdelim, unsigned color){
  ncplane_set_fg_rgb(n, color);
  if(ncplane_putstr(n, str) < 0){
    return -1;
  }
  ncplane_set_fg_rgb8(n, 178, 102, 255);
  if(notcurses_canutf8(ncplane_notcurses(n))){
    if(ncplane_putstr(n, delim) < 0){
      return -1;
    }
  }else{
    if(ncplane_putstr(n, ascdelim) < 0){
      return -1;
    }
  }
  return 0;
}

static int
table_segment(struct ncplane* n, const char* str, const char* delim,
              const char* ascdelim){
  return table_segment_color(n, str, delim, ascdelim, 0xffffff);
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
                    demos[results[i].selector - 'a'].name, results[i].stats.raster_bytes,
                    results[i].stats.renders, results[i].timens, i < strlen(spec) - 1 ? "," : "") < 0);
  }
  ret |= (fprintf(f, "}}}\n") < 0);
  return ret;
}

static int
summary_table(struct notcurses* nc, const char* spec, bool canimage, bool canvideo){
  notcurses_leave_alternate_screen(nc);
  struct ncplane* n = notcurses_stdplane(nc);
  ncplane_set_bg_default(n);
  ncplane_set_scrolling(n, true);
  bool failed = false;
  uint64_t totalbytes = 0;
  long unsigned totalframes = 0;
  ncplane_putchar(n, '\n');
  // FIXME this shouldn't be necessary, but without it, late in 2.4.x we
  // stopped printing the table header. see #2389.
  notcurses_render(nc);
  const char* sep;
  if(notcurses_canutf8(ncplane_notcurses(n))){
    sep = u8"│";
  }else{
    sep = "|";
  }
  table_segment(n, "             runtime", "│", "|");
  table_segment(n, " frames", "│", "|");
  table_segment(n, "output(B)", "│", "|");
  table_segment(n, "    FPS", "│", "|");
  table_segment(n, "%r", "│", "|");
  table_segment(n, "%a", "│", "|");
  table_segment(n, "%w", "│", "|");
  table_segment(n, "TheoFPS", "║\n══╤════════╤════════╪═══════╪═════════╪═══════╪══╪══╪══╪═══════╣\n",
                "|\n--+--------+--------+-------+---------+-------+--+--+--+-------|\n");
  char timebuf[NCPREFIXSTRLEN + 1];
  char tfpsbuf[NCPREFIXSTRLEN + 1];
  char totalbuf[NCBPREFIXSTRLEN + 1];
  uint64_t nsdelta = 0;
  for(size_t i = 0 ; i < strlen(spec) ; ++i){
    nsdelta += results[i].timens;
    ncqprefix(results[i].timens, NANOSECS_IN_SEC, timebuf, 0);
    ncbprefix(results[i].stats.raster_bytes, 1, totalbuf, 0);
    uint64_t divisor = results[i].stats.render_ns + results[i].stats.writeout_ns + results[i].stats.raster_ns;
    if(divisor){
      ncqprefix((uintmax_t)results[i].stats.writeouts * NANOSECS_IN_SEC * 1000 / divisor,
              1000, tfpsbuf, 0);
    }else{
      ncqprefix(0, NANOSECS_IN_SEC, tfpsbuf, 0);
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
    ncplane_putegc(n, sep, NULL);
    ncplane_set_fg_rgb(n, rescolor);
    ncplane_printf(n, "%8s", demos[results[i].selector - 'a'].name);
    ncplane_set_fg_rgb8(n, 178, 102, 255);
    ncplane_printf(n, "%s%*ss%s%7" PRIu64 "%s%*s%s%7.1f%s%2" PRId64 "%s%2" PRId64 "%s%2" PRId64 "%s%*s%s",
           sep, NCPREFIXFMT(timebuf), sep,
           results[i].stats.renders, sep,
           NCBPREFIXFMT(totalbuf), sep,
           results[i].timens ?
            results[i].stats.renders / ((double)results[i].timens / NANOSECS_IN_SEC) : 0.0, sep,
           (results[i].timens ?
            results[i].stats.render_ns * 100 / results[i].timens : 0), sep,
           (results[i].timens ?
            results[i].stats.raster_ns * 100 / results[i].timens : 0), sep,
           (results[i].timens ?
            results[i].stats.writeout_ns * 100 / results[i].timens : 0), sep,
           NCPREFIXFMT(tfpsbuf),
           notcurses_canutf8(ncplane_notcurses(n)) ? "║" : "|");
    ncplane_set_fg_rgb(n, rescolor);
    ncplane_printf(n, "%s\n", results[i].result < 0 ? "FAILED" :
                   results[i].result > 0 ? "ABORTED" :
                   !results[i].stats.renders ? "SKIPPED"  : "");
    if(results[i].result < 0){
      failed = true;
    }
    totalframes += results[i].stats.renders;
    totalbytes += results[i].stats.raster_bytes;
  }
  ncqprefix(nsdelta, NANOSECS_IN_SEC, timebuf, 0);
  ncbprefix(totalbytes, 1, totalbuf, 0);
  table_segment(n, "", "══╧════════╧════════╪═══════╪═════════╪═══════╧══╧══╧══╧═══════╝\n",
                "--+--------+--------+-------+---------+-------+--+--+--+-------+\n");
  ncplane_putstr(n, "            ");
  table_printf(n, sep, "%*ss", NCPREFIXFMT(timebuf));
  table_printf(n, sep, "%7lu", totalframes);
  table_printf(n, sep, "%*s", NCBPREFIXFMT(totalbuf));
  //table_printf(nc, sep, "%7.1f", nsdelta ? totalframes / ((double)nsdelta / NANOSECS_IN_SEC) : 0);
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
#ifndef __MINGW32__
  sigset_t sigmask;
  // ensure SIGWINCH is delivered only to a thread doing input
  sigemptyset(&sigmask);
  sigaddset(&sigmask, SIGWINCH);
  pthread_sigmask(SIG_BLOCK, &sigmask, NULL);
#endif
  const char* spec;
  FILE* json = NULL; // emit JSON summary to this file? (-J)
  notcurses_options nopts = {0};
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
  notcurses_mice_enable(nc, NCMICE_BUTTON_EVENT | NCMICE_DRAG_EVENT);
  const bool canimage = notcurses_canopen_images(nc);
  const bool canvideo = notcurses_canopen_videos(nc);
  unsigned dimx, dimy;
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
  int r = summary_table(nc, spec, canimage, canvideo);
  notcurses_render(nc); // render our summary table
  free(results);
  if(notcurses_stop(nc)){
    return EXIT_FAILURE;
  }
  if(json && summary_json(json, spec, dimy, dimx)){
    return EXIT_FAILURE;
  }
  free(datadir);
  return r ? EXIT_FAILURE : EXIT_SUCCESS;

err:
  stop_input();
  notcurses_stop(nc);
  if(dimy < MIN_SUPPORTED_ROWS || dimx < MIN_SUPPORTED_COLS){
    fprintf(stderr, "At least a %dx%d terminal is required (current: %dx%d)\n",
            MIN_SUPPORTED_ROWS, MIN_SUPPORTED_COLS, dimy, dimx);
  }
  return EXIT_FAILURE;
}

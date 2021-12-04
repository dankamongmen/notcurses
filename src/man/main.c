#include <stdlib.h>
#include <getopt.h>
#include <notcurses/notcurses.h>

static void
usage(const char* argv0, FILE* o){
  fprintf(o, "usage: %s [ -hV ] files\n", argv0);
  fprintf(o, " -h: print help and return success\n");
  fprintf(o, " -v: print version and return success\n");
}

static int
parse_args(int argc, char** argv){
  const char* argv0 = *argv;
  int longindex;
  int c;
  struct option longopts[] = {
    { .name = "help", .has_arg = 0, .flag = NULL, .val = 'h', },
    { .name = NULL, .has_arg = 0, .flag = NULL, .val = 0, }
  };
  while((c = getopt_long(argc, argv, "hV", longopts, &longindex)) != -1){
    switch(c){
      case 'h': usage(argv0, stdout);
                exit(EXIT_SUCCESS);
                break;
      case 'V': fprintf(stderr, "ncman version %s\n", notcurses_version());
                exit(EXIT_SUCCESS);
                break;
      default: usage(argv0, stderr);
               return -1;
               break;
    }
  }
  if(argv[optind] == NULL){
    usage(argv0, stderr);
    return -1;
  }
  return optind;
}

static int
manloop(struct notcurses* nc, const char* arg){
  unsigned dimy, dimx;
  struct ncplane* stdn = notcurses_stddim_yx(nc, &dimy, &dimx);
  ncplane_putstr(stdn, arg);
  if(notcurses_render(nc)){
    return -1;
  }
  uint32_t key;
  ncinput ni;
  while((key = notcurses_get(nc, NULL, &ni)) != (uint32_t)-1){
    switch(key){
      case 'q': return 0;
    }
  }
  return -1;
}

static int
ncman(struct notcurses* nc, const char* arg){
  unsigned dimy, dimx;
  struct ncplane* stdn = notcurses_stddim_yx(nc, &dimy, &dimx);
  // FIXME usage bar at bottom
  return manloop(nc, arg);
}

int main(int argc, char** argv){
  int nonopt = parse_args(argc, argv);
  if(nonopt <= 0){
    return EXIT_FAILURE;
  }
  struct notcurses_options nopts = {
    .flags = NCOPTION_NO_ALTERNATE_SCREEN,
  };
  struct notcurses* nc = notcurses_core_init(&nopts, NULL);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  bool success;
  for(int i = 0 ; i < argc - nonopt ; ++i){
    success = false;
    if(ncman(nc, argv[nonopt + i])){
      break;
    }
    success = true;
  }
  return notcurses_stop(nc) || !success ? EXIT_FAILURE : EXIT_SUCCESS;
}

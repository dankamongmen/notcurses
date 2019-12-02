#include <array>
#include <cstdlib>
#include <clocale>
#include <libgen.h>
#include <unistd.h>
#include <iostream>
#include <pthread.h>
#include <sys/eventfd.h>
#include "notcurses.h"

static void usage(std::ostream& os, const char* name, int exitcode)
  __attribute__ ((noreturn));

void usage(std::ostream& o, const char* name, int exitcode){
  o << "usage: " << name << " files" << '\n';
  exit(exitcode);
}

static struct marshal {
  struct notcurses* nc;
  struct ncvisual* ncv;
  int averr;
} m;

int ncview(struct notcurses* nc, struct ncvisual* ncv, int* averr){
  if(ncvisual_stream(nc, ncv, averr)){
    return -1;
  }
  return 0;
}

void* ncviewthread(void* v){
  ncview(m.nc, m.ncv, &m.averr);
  return NULL;
}

int main(int argc, char** argv){
  setlocale(LC_ALL, "");
  if(argc == 1){
    usage(std::cerr, argv[0], EXIT_FAILURE);
  }
  notcurses_options opts{};
  opts.outfp = stdout;
  auto nc = notcurses_init(&opts);
  if(nc == nullptr){
    return EXIT_FAILURE;
  }
  int dimy, dimx;
  notcurses_term_dim_yx(nc, &dimy, &dimx);
  auto ncp = notcurses_newplane(nc, dimy - 1, dimx, 1, 0, nullptr);
  if(ncp == nullptr){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  for(int i = 1 ; i < argc ; ++i){
    std::array<char, 128> errbuf;
    int averr;
    auto ncv = ncplane_visual_open(ncp, argv[i], &averr);
    if(ncv == nullptr){
      av_make_error_string(errbuf.data(), errbuf.size(), averr);
      notcurses_stop(nc);
      std::cerr << "Error opening " << argv[i] << ": " << errbuf.data() << std::endl;
      return EXIT_FAILURE;
    }
    pthread_t tid;
    m.nc = nc;
    m.ncv = ncv;
    m.averr = 0;
    int efd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if(efd < 0){
      fprintf(stderr, "Error creating eventfd (%s)\n", strerror(errno));
      return -1;
    }
    panelreel_options popts{};
    struct panelreel* pr = panelreel_create(ncp, &popts, efd);
    if(pr == nullptr){
      return EXIT_FAILURE;
    }
    notcurses_render(nc);
    pthread_create(&tid, NULL, ncviewthread, NULL);
    ncspecial_key special;
    cell c = CELL_TRIVIAL_INITIALIZER;
    notcurses_getc_blocking(nc, &c, &special);
    cell_release(ncp, &c);
    // ncvisual_destroy(ncv);
  }
  if(notcurses_stop(nc)){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

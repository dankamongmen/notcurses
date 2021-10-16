#include <stdlib.h>
#include <unistd.h>
#include <locale.h>
#include <notcurses/notcurses.h>
#include "compat/compat.c"

int main(int argc, char** argv){
  if(setlocale(LC_ALL, "") == NULL){
    fprintf(stderr, "Couldn't set locale based off LANG\n");
    return EXIT_FAILURE;
  }
  if(argc < 2){
    fprintf(stderr, "usage: blitters file [files...]\n");
    return EXIT_FAILURE;
  }
  struct notcurses_options nopts = {
    .flags = NCOPTION_INHIBIT_SETLOCALE | NCOPTION_NO_ALTERNATE_SCREEN
             | NCOPTION_DRAIN_INPUT,
  };
  struct notcurses* nc = notcurses_init(&nopts, NULL);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  struct ncplane* std = notcurses_stdplane(nc);
  const int blitters[] = {
    NCBLIT_DEFAULT, // let the ncvisual pick
    NCBLIT_1x1,     // full block
    NCBLIT_2x1,     // full/(upper|left) blocks
    NCBLIT_2x2,     // quadrants
    NCBLIT_3x2,     // sextants
    NCBLIT_BRAILLE, // 4 rows, 2 cols (braille)
    NCBLIT_PIXEL,   // pixel graphics
    -1,
  };
  for(const int* blitter = blitters ; *blitter >= 0 ; ++blitter){
    for(int scaling = NCSCALE_NONE ; scaling <= NCSCALE_STRETCH ; ++scaling){
      for(int i = 1 ; i < argc ; ++i){
        ncplane_erase(std);
        const char* fname = argv[i];
        struct ncvisual* ncv = ncvisual_from_file(fname);
        if(ncv == NULL){
          goto err;
        }
        notcurses_render(nc);
        struct ncvisual_options vopts = {
          .scaling = scaling,
          .blitter = *blitter,
          .n = notcurses_stdplane(nc),
          .flags = NCVISUAL_OPTION_CHILDPLANE,
        };
        struct ncplane* cn;
        if((cn = ncvisual_blit(nc, ncv, &vopts)) == NULL){
          ncvisual_destroy(ncv);
          goto err;
        }
        notcurses_render(nc);
        struct timespec ts = {
          .tv_sec = 0,
          .tv_nsec = 500000000,
        };
        clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);
        ncvisual_destroy(ncv);
        ncplane_destroy(cn);
      }
    }
  }
  notcurses_stop(nc);
  return EXIT_SUCCESS;

err:
  notcurses_stop(nc);
  return EXIT_FAILURE;
}

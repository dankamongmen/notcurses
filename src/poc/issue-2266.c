#include <notcurses/notcurses.h>

int main(void){
  // init notcurses
  struct notcurses_options nopts = {
	// .loglevel = NCLOGLEVEL_TRACE,
  };
  struct notcurses* nc = notcurses_init(&nopts, NULL);
  if(nc == NULL){
    return EXIT_FAILURE;
  }

  // fill buffer with random pixels
  int x = 2; int y = 2;
  size_t bufferlen = y * x * 3;
  unsigned char* buffer = malloc(bufferlen);
  if(buffer == NULL){
    return -1;
  }
  for(size_t r = 0 ; r < bufferlen ; ++r){
    buffer[r] = rand() % 256;
    buffer[r] = rand() % 256;
  }

  // prepare visual
  struct ncvisual* ncv = ncvisual_from_rgb_packed(buffer, y, x * 3, x, 0xff);
  if(ncv == NULL){
    free(buffer);
    return -1;
  }
  struct ncvisual_options vopts = {
    .y = 1,
    .blitter = NCBLIT_PIXEL,
  };

  // render visual
  struct ncplane* ncvp = ncvisual_render(nc, ncv, &vopts);
  if(ncvp == NULL){
    free(buffer);
    return -1;
  }
  notcurses_render(nc);

  // wait for input to clean up & exit
  ncinput ni;
  notcurses_getc_blocking(nc, &ni);
  ncplane_destroy(ncvp);
  ncvisual_destroy(ncv);
  notcurses_stop(nc);
  return EXIT_SUCCESS;
}

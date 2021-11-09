#include <unistd.h>
#include <notcurses/direct.h>

static int
rendered_cursor(void){
  struct notcurses_options opts = {
    .loglevel = NCLOGLEVEL_TRACE,
  };
  struct notcurses* nc = notcurses_init(&opts, NULL);
  if(nc == NULL){
    return -1;
  }
  if(notcurses_cursor_enable(nc, 10, 10)){
    notcurses_stop(nc);
    fprintf(stderr, "couldn't enable cursor\n");
    return -1;
  }
  sleep(1);
  if(notcurses_render(nc)){
    notcurses_stop(nc);
    fprintf(stderr, "couldn't render\n");
    return -1;
  }
  sleep(1);
  return notcurses_stop(nc);
}

int main(void){
  if(rendered_cursor()){
    return EXIT_FAILURE;
  }
  uint64_t flags = NCDIRECT_OPTION_VERY_VERBOSE
                   | NCDIRECT_OPTION_DRAIN_INPUT;
  struct ncdirect* n = ncdirect_core_init(NULL, stdout, flags);
  if(n == NULL){
    return EXIT_FAILURE;
  }
  unsigned y, x;
  if(ncdirect_cursor_yx(n, &y, &x)){
    goto err;
  }
  unsigned dimx = ncdirect_dim_x(n);
  unsigned dimy = ncdirect_dim_y(n);
  printf("Cursor: column %u/%u row %u/%u\n", x, dimx, y, dimy);
  ncdirect_stop(n);
  return EXIT_SUCCESS;

err:
  fprintf(stderr, "direct mode cursor lookup failed\n");
  ncdirect_stop(n);
  return EXIT_FAILURE;
}

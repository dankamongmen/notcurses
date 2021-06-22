#include <notcurses/direct.h>


static int
rendered_cursor(void){
  struct notcurses* nc = notcurses_init(NULL, NULL);
  notcurses_cursor_enable(nc, 10, 10);
  sleep(1);
  notcurses_render(nc);
  sleep(1);
  notcurses_stop(nc);
  return 0;
}

int main(void){
  struct ncdirect* n = ncdirect_core_init(NULL, stdout, 0);
  if(n == NULL){
    return EXIT_FAILURE;
  }
  int y, x;
  if(ncdirect_cursor_yx(n, &y, &x)){
    goto err;
  }
  int dimx = ncdirect_dim_x(n);
  int dimy = ncdirect_dim_y(n);
  printf("Cursor: column %d/%d row %d/%d\n", x, dimx, y, dimy);
  ncdirect_stop(n);
  if(rendered_cursor()){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;

err:
  ncdirect_stop(n);
  return EXIT_FAILURE;
}

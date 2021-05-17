#include <notcurses/direct.h>

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
  return EXIT_SUCCESS;

err:
  ncdirect_stop(n);
  return EXIT_FAILURE;
}

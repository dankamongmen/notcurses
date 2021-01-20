#include <notcurses/direct.h>

int main(void){
  struct ncdirect* n = ncdirect_core_init(NULL, NULL, 0);
  if(!n){
    return EXIT_FAILURE;
  }
  ncdirect_set_fg_rgb8(n, 100, 100, 100);
  ncdirect_set_bg_rgb8(n, 0xff, 0xff, 0xff);
  printf("a");
  ncdirect_set_bg_rgb8(n, 0, 0, 0);
  printf("b");
  printf(" ");
  printf(" ");
  ncdirect_set_bg_rgb8(n, 0, 0, 1);
  printf("c");
  printf(" ");
  printf(" ");
  ncdirect_set_bg_rgb8(n, 0xff, 0xff, 0xff);
  printf("d");
  printf("\n");
  ncdirect_stop(n);
  return EXIT_SUCCESS;
}

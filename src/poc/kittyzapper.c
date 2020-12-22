#include <notcurses/direct.h>

int main(void){
  struct ncdirect* n = ncdirect_init(NULL, NULL, 0);
  if(!n){
    return EXIT_FAILURE;
  }
  ncdirect_fg_rgb8(n, 100, 100, 100);
  ncdirect_bg_rgb8(n, 0xff, 0xff, 0xff);
  printf("a");
  ncdirect_bg_rgb8(n, 0, 0, 0);
  printf("b");
  printf(" ");
  printf(" ");
  ncdirect_bg_rgb8(n, 0, 0, 1);
  printf("c");
  printf(" ");
  printf(" ");
  ncdirect_bg_rgb8(n, 0xff, 0xff, 0xff);
  printf("d");
  printf("\n");
  ncdirect_stop(n);
  return EXIT_SUCCESS;
}

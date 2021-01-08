#include <stdlib.h>
#include <notcurses/direct.h>

int main(void){
  struct ncdirect* n = ncdirect_init(NULL, NULL, 0);
  if(n == NULL){
    return EXIT_FAILURE;
  }
  if(ncdirect_set_fg_rgb(n, 0x00aaaa) || printf("oh yeaaaaaaaaah\n") < 0){
    ncdirect_stop(n);
    return EXIT_FAILURE;
  }
  return ncdirect_stop(n) ? EXIT_FAILURE : EXIT_SUCCESS;
}

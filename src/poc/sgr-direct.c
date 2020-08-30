#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <assert.h>
#include <notcurses/direct.h>

int main(void){
  if(!setlocale(LC_ALL, "")){
    return EXIT_FAILURE;
  }
  struct ncdirect* nc = ncdirect_init(NULL, stdout, 0);
  if(!nc){
    return EXIT_FAILURE;
  }
  int e = 0;
  for(unsigned i = 0 ; i < (NCSTYLE_ITALIC << 1u) ; ++i){
    if(ncdirect_styles_set(nc, i)){
      ncdirect_stop(nc);
      return EXIT_FAILURE;
    }
    printf("%08x ", i);
    if(++e % 6 == 0){
      printf("\n");
    }
  }
  printf("\n");
  ncdirect_stop(nc);
  return EXIT_SUCCESS;
}

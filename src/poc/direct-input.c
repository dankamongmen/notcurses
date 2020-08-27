#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <notcurses/direct.h>

int main(void){
  struct ncdirect* n = ncdirect_init(NULL, NULL);
  if(n == NULL){
    return EXIT_FAILURE;
  }
  ncinput ni;
  char32_t i;
  while((i = ncdirect_getc_blocking(n, &ni)) != (char32_t)-1){
    printf("Read input: [%c%c%c] %lc\n", ni.ctrl ? 'C' : 'c',
           ni.alt ? 'A' : 'a', ni.shift ? 'S' : 's', i);
    if(ni.ctrl && i == 'D'){
      break;
    }
  }
  if(ncdirect_stop(n) || i == (char32_t)-1){
    fprintf(stderr, "Failure reading input (%s)\n", strerror(errno));
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

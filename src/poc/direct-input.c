#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <notcurses/direct.h>

int main(void){
  struct ncdirect* n = ncdirect_core_init(NULL, NULL, 0);
  if(n == NULL){
    return EXIT_FAILURE;
  }
  ncinput ni;
  uint32_t i;
  while((i = ncdirect_get_blocking(n, &ni)) != (uint32_t)-1){
    unsigned char utf8[5] = {0};
    notcurses_ucs32_to_utf8(&i, 1, utf8, sizeof(utf8));
    printf("Read input: [%c%c%c] %s\n",
           ncinput_ctrl_p(&ni) ? 'C' : 'c',
           ncinput_alt_p(&ni) ? 'A' : 'a',
           ncinput_shift_p(&ni) ? 'S' : 's',
           utf8);
    if(ncinput_ctrl_p(&ni) && i == 'D'){
      break;
    }
  }
  if(ncdirect_stop(n) || i == (uint32_t)-1){
    fprintf(stderr, "Failure reading input (%s)\n", strerror(errno));
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

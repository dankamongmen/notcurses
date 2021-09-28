#include <notcurses/direct.h>

// play with libreadline as wrapped by notcurses-direct

static int
rl(struct ncdirect* n){
  char* l;
  while( (l = ncdirect_readline(n, "ncdirect")) ){
    fprintf(stderr, "input: [%s]\n", l);
    free(l);
  }
  return 0;
}

int main(void){
  uint64_t flags = NCDIRECT_OPTION_INHIBIT_CBREAK;
  struct ncdirect* n = ncdirect_core_init(NULL, NULL, flags);
  if(n == NULL){
    return EXIT_FAILURE;
  }
  rl(n);
  if(ncdirect_stop(n)){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

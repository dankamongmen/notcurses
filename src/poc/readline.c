#include <notcurses/direct.h>

// play with libreadline as wrapped by notcurses-direct

static int
rl(struct ncdirect* n){
  char* l;
  while( (l = ncdirect_readline(n, "[ncdirect] ")) ){
    fprintf(stderr, "input: [%s]\n", l);
    free(l);
  }
  return 0;
}

static void
usage(void){
  fprintf(stderr, "usage: readline [ -v ]\n");
  exit(EXIT_FAILURE);
}

int main(int argc, char** argv){
  uint64_t flags = 0;
  if(argc != 1){
    if(argc != 2){
      usage();
    }
    if(strcmp(argv[1], "-v")){
      usage();
    }
    flags = NCDIRECT_OPTION_VERY_VERBOSE;
  }
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

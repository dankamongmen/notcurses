#include <stdio.h>
#include <stdlib.h>
#include <notcurses.h>

int main(void){
  struct notcurses* nc;
  int x, y;
  if((nc = notcurses_init()) == NULL){
    return EXIT_FAILURE;
  }
  if(notcurses_term_dimensions(nc, &y, &x)){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  printf("Dimensions: %d rows x %d cols\n", y, x);
  if(notcurses_stop(nc)){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

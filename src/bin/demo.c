#include <stdlib.h>
#include <notcurses.h>

int main(void){
  if(notcurses_init()){
    return EXIT_FAILURE;
  }
  if(notcurses_stop()){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

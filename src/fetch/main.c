#include <notcurses/notcurses.h>

int main(int argc, const char** argv){
  struct notcurses* nc = notcurses_init(NULL, NULL);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

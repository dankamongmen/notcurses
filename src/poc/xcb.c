#include <stdlib.h>
#include <notcurses/notcurses.h>

int main(void){
  struct notcurses* nc = notcurses_init(NULL, NULL);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  char* title = notcurses_title(nc);
  if(title == NULL){
    notcurses_stop(nc);
    fprintf(stderr, "Couldn't get window title\n");
    return EXIT_FAILURE;
  }
  notcurses_stop(nc);
  printf("Window title: %s\n", title);
  free(title);
  return EXIT_SUCCESS;
}

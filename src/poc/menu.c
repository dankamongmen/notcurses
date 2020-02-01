#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <stdlib.h>
#include <notcurses.h>

int main(void){
  if(!setlocale(LC_ALL, "")){
    return EXIT_FAILURE;
  }
  notcurses_options opts;
  memset(&opts, 0, sizeof(opts));
  struct notcurses* nc = notcurses_init(&opts, stdout);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  menu_options mopts = {
  };
  struct ncmenu* top = ncmenu_create(nc, &mopts);
  mopts.bottom = true;
  struct ncmenu* bottom = ncmenu_create(nc, &mopts);

  notcurses_render(nc);
  char32_t keypress;
  ncinput ni;
  while((keypress = notcurses_getc_blocking(nc, &ni)) != (char32_t)-1){
    switch(keypress){
      // FIXME
    }
    if(keypress == 'q'){
      break;
    }
    notcurses_render(nc);
  }
  ncmenu_destroy(top);
  ncmenu_destroy(bottom);
  if(notcurses_stop(nc)){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

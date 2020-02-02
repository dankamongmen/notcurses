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
  struct menu_item items[] = {
    { .desc = "Restart", },
  };
  struct menu_section sections[] = {
    { .name = "Demo", .items = items, },
  };
  sections[0].itemcount = sizeof(items) / sizeof(*items);
  menu_options mopts;
  memset(&mopts, 0, sizeof(mopts));
  mopts.sections = sections;
  mopts.sectioncount = sizeof(sections) / sizeof(*sections);
  struct ncmenu* top = ncmenu_create(nc, &mopts);
  mopts.bottom = true;
  struct ncmenu* bottom = ncmenu_create(nc, &mopts);

  notcurses_render(nc);
  char32_t keypress;
  ncinput ni;
  int dimy, dimx;
  struct ncplane* n = notcurses_stdplane(nc);
  ncplane_dim_yx(n, &dimy, &dimx);
  ncplane_styles_on(n, NCSTYLE_REVERSE);
  if(ncplane_putstr_aligned(n, dimy - 1, NCALIGN_RIGHT, "menu poc. press q to exit") < 0){
	  return EXIT_FAILURE;
  }
  notcurses_render(nc);
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

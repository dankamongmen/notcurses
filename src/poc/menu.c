#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <stdlib.h>
#include <notcurses.h>

static int
run_menu(struct notcurses* nc, struct ncmenu* ncm){
  char32_t keypress;
  ncinput ni;
  notcurses_render(nc);
  while((keypress = notcurses_getc_blocking(nc, &ni)) != (char32_t)-1){
    if(ni.alt){
      switch(keypress){
        case 'd': case 'D':
          ncmenu_unroll(ncm, 0);
          break;
        case 'f': case 'F':
          ncmenu_unroll(ncm, 1);
          break;
      }
    }
    if(keypress == 'q'){
      ncmenu_destroy(ncm);
      return 0;
    }
    notcurses_render(nc);
  }
  ncmenu_destroy(ncm);
  return -1;
}

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
  struct ncmenu_item demo_items[] = {
    { .desc = "Restart", },
  };
  struct ncmenu_item file_items[] = {
    { .desc = "New", },
    { .desc = "Open", },
    { .desc = "Close", },
    { .desc = NULL, },
    { .desc = "Quit", },
  };
  struct ncmenu_section sections[] = {
    { .name = "Demo", .items = demo_items, },
    { .name = "File", .items = file_items, },
  };
  sections[0].itemcount = sizeof(demo_items) / sizeof(*demo_items);
  sections[1].itemcount = sizeof(file_items) / sizeof(*file_items);
  ncmenu_options mopts;
  memset(&mopts, 0, sizeof(mopts));
  mopts.sections = sections;
  mopts.sectioncount = sizeof(sections) / sizeof(*sections);
  channels_set_fg(&mopts.headerchannels, 0x00ff00);
  channels_set_bg(&mopts.headerchannels, 0x440000);
  struct ncmenu* top = ncmenu_create(nc, &mopts);

  notcurses_render(nc);
  int dimy, dimx;
  struct ncplane* n = notcurses_stdplane(nc);
  ncplane_dim_yx(n, &dimy, &dimx);
  ncplane_set_fg(n, 0x00dddd);
  if(ncplane_putstr_aligned(n, dimy - 1, NCALIGN_RIGHT, " -=+ menu poc. press q to exit +=- ") < 0){
	  return EXIT_FAILURE;
  }
  run_menu(nc, top);

  ncplane_erase(n);
  mopts.bottom = true;
  struct ncmenu* bottom = ncmenu_create(nc, &mopts);
  if(ncplane_putstr_aligned(n, 0, NCALIGN_RIGHT, " -=+ menu poc. press q to exit +=- ") < 0){
	  return EXIT_FAILURE;
  }
  run_menu(nc, top);

  if(notcurses_stop(nc)){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <stdlib.h>
#include <notcurses/notcurses.h>

static int
run_menu(struct notcurses* nc, struct ncmenu* ncm){
  ncplane_options nopts = {
    .y = 10,
    .x = NCALIGN_CENTER,
    .rows = 3,
    .cols = 40,
    .flags = NCPLANE_OPTION_HORALIGNED,
  };
  struct ncplane* selplane = ncplane_create(notcurses_stdplane(nc), &nopts);
  if(selplane == NULL){
    return -1;
  }
  ncplane_set_fg_rgb(selplane, 0x0);
  ncplane_set_bg_rgb(selplane, 0xdddddd);
  uint64_t channels = 0;
  ncchannels_set_fg_rgb(&channels, 0x000088);
  ncchannels_set_bg_rgb(&channels, 0x88aa00);
  if(ncplane_set_base(selplane, " ", 0, channels) < 0){
    goto err;
  }
  uint32_t keypress;
  ncinput ni;
  notcurses_render(nc);
  while((keypress = notcurses_get_blocking(nc, &ni)) != (uint32_t)-1){
    if(!ncmenu_offer_input(ncm, &ni)){
      const char* sel;
      if( (sel = ncmenu_mouse_selected(ncm, &ni, NULL)) ){
        if(sel && !strcmp(sel, "Quit")){
          ncmenu_destroy(ncm);
          ncplane_destroy(selplane);
          return 0;
        }
      }else if(ni.evtype == NCTYPE_RELEASE){
        continue;
      }else if(keypress == 'q'){
        ncmenu_destroy(ncm);
        ncplane_destroy(selplane);
        return 0;
      }else if(keypress == NCKEY_ENTER){ // selected a menu item
        if( (sel = ncmenu_selected(ncm, &ni)) ){
          if(strcmp(sel, "Quit") == 0){
            ncmenu_destroy(ncm);
            ncplane_destroy(selplane);
            return 0;
          }
        }
      }
    }
    ncplane_erase(selplane);
    ncinput selni;
    const char* selitem = ncmenu_selected(ncm, &selni);
    ncplane_putstr_aligned(selplane, 1, NCALIGN_CENTER, selitem ? selitem : "");
    notcurses_render(nc);
  }
  ncmenu_destroy(ncm);
  return 0;

err:
  ncplane_destroy(selplane);
  return -1;
}

int main(void){
  notcurses_options opts = { };
  struct notcurses* nc = notcurses_core_init(&opts, NULL);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  notcurses_mice_enable(nc, NCMICE_BUTTON_EVENT);
  struct ncmenu_item demo_items[] = {
    { .desc = "Restart", .shortcut = { .id = 'r', .modifiers = NCKEY_MOD_CTRL, }, },
    { .desc = "Derp", .shortcut = { .id = 'd', }, },
  };
  struct ncmenu_item file_items[] = {
    { .desc = "New", .shortcut = { .id = 'n', .modifiers = NCKEY_MOD_CTRL, }, },
    { .desc = "Open", .shortcut = { .id = 'o', .modifiers = NCKEY_MOD_CTRL, }, },
    { .desc = "Close", .shortcut = { .id = 'c', .modifiers = NCKEY_MOD_CTRL, }, },
    { .desc = NULL, },
    { .desc = "Quit", .shortcut = { .id = 'q', }, },
  };
  struct ncmenu_item help_items[] = {
    { .desc = "About", .shortcut = { .id = 'a', .modifiers = NCKEY_MOD_CTRL, }, },
  };
  struct ncmenu_section sections[] = {
    { .name = "Schwarzgerät", .items = demo_items,
      .itemcount = sizeof(demo_items) / sizeof(*demo_items),
      .shortcut = { .id = 0x00e4, .modifiers = NCKEY_MOD_ALT, }, },
    { .name = "File", .items = file_items,
      .itemcount = sizeof(file_items) / sizeof(*file_items),
      .shortcut = { .id = 'f', .modifiers = NCKEY_MOD_ALT, }, },
    { .name = NULL, .items = NULL, .itemcount = 0, },
    { .name = "Help", .items = help_items,
      .itemcount = sizeof(help_items) / sizeof(*help_items),
      .shortcut = { .id = 'h', .modifiers = NCKEY_MOD_ALT, }, },
  };
  ncmenu_options mopts;
  memset(&mopts, 0, sizeof(mopts));
  mopts.sections = sections;
  mopts.sectioncount = sizeof(sections) / sizeof(*sections);
  ncchannels_set_fg_rgb(&mopts.headerchannels, 0x00ff00);
  ncchannels_set_bg_rgb(&mopts.headerchannels, 0x440000);
  ncchannels_set_fg_rgb(&mopts.sectionchannels, 0xb0d700);
  ncchannels_set_bg_rgb(&mopts.sectionchannels, 0x002000);
  unsigned dimy, dimx;
  struct ncplane* n = notcurses_stddim_yx(nc, &dimy, &dimx);
  struct ncmenu* top = ncmenu_create(n, &mopts);
  if(top == NULL){
    goto err;
  }
  if(ncmenu_item_set_status(top, "Schwarzgerät", "Restart", false)){
    goto err;
  }
  if(ncmenu_item_set_status(top, "File", "Open", false)){
    goto err;
  }
  uint64_t channels = 0;
  ncchannels_set_fg_rgb(&channels, 0x88aa00);
  ncchannels_set_bg_rgb(&channels, 0x000088);
  if(ncplane_set_base(n, "x", 0, channels) < 0){
    return EXIT_FAILURE;
  }

  notcurses_render(nc);
  ncplane_set_fg_rgb(n, 0x00dddd);
  if(ncplane_putstr_aligned(n, dimy - 1, NCALIGN_RIGHT, " -=+ menu poc. press q to exit +=- ") < 0){
	  return EXIT_FAILURE;
  }
  run_menu(nc, top);

  ncplane_erase(n);

  mopts.flags |= NCMENU_OPTION_BOTTOM;
  struct ncmenu* bottom = ncmenu_create(n, &mopts);
  if(bottom == NULL){
    goto err;
  }
  if(ncmenu_item_set_status(bottom, "Schwarzgerät", "Restart", false)){
    goto err;
  }
  if(ncmenu_item_set_status(bottom, "Schwarzgerät", "Derp", false)){
    goto err;
  }
  if(ncplane_putstr_aligned(n, 0, NCALIGN_RIGHT, " -=+ menu poc. press q to exit +=- ") < 0){
	  return EXIT_FAILURE;
  }
  run_menu(nc, bottom);

  if(notcurses_stop(nc)){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;

err:
  notcurses_stop(nc);
  return EXIT_FAILURE;
}

#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <stdlib.h>
#include <notcurses.h>

static int
run_menu(struct notcurses* nc, struct ncmenu* ncm){
  struct ncplane* selplane = ncplane_aligned(notcurses_stdplane(nc), 3, 40, 10, NCALIGN_CENTER, NULL);
  if(selplane == NULL){
    return -1;
  }
  ncplane_set_fg(selplane, 0x0);
  ncplane_set_bg(selplane, 0xdddddd);
  uint64_t channels = 0;
  channels_set_fg(&channels, 0x000088);
  channels_set_bg(&channels, 0x88aa00);
  if(ncplane_set_base(selplane, channels, 0, " ") < 0){
    goto err;
  }
  char32_t keypress;
  ncinput ni;
  notcurses_render(nc);
  while((keypress = notcurses_getc_blocking(nc, &ni)) != (char32_t)-1){
    if(keypress == NCKEY_LEFT){
      if(ncmenu_prevsection(ncm)){
        goto err;
      }
    }else if(keypress == NCKEY_RIGHT){
      if(ncmenu_nextsection(ncm)){
        goto err;
      }
    }else if(keypress == NCKEY_UP){
      if(ncmenu_previtem(ncm)){
        goto err;
      }
    }else if(keypress == NCKEY_DOWN){
      if(ncmenu_nextitem(ncm)){
        goto err;
      }
    }else if(keypress == '\x1b'){
      if(ncmenu_rollup(ncm)){
        goto err;
      }
    }else if(ni.alt){
      switch(keypress){
        case 'a': case 'A': case 0x00e4:
          if(ncmenu_unroll(ncm, 0)){
            goto err;
          }
          break;
        case 'f': case 'F':
          if(ncmenu_unroll(ncm, 1)){
            goto err;
          }
          break;
      }
    }else if(keypress == 'q'){
      ncmenu_destroy(nc, ncm);
      ncplane_destroy(selplane);
      return 0;
    }
    ncplane_erase(selplane);
    const char* selitem = ncmenu_selected(ncm);
    ncplane_putstr_aligned(selplane, 1, NCALIGN_CENTER, selitem ? selitem : "");
    notcurses_render(nc);
  }
  ncmenu_destroy(nc, ncm);

err:
  ncplane_destroy(selplane);
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
    { .desc = "Restart", .shortcut = { .id = 'r', .ctrl = true, }, },
  };
  struct ncmenu_item file_items[] = {
    { .desc = "New", .shortcut = { .id = 'n', .ctrl = true, }, },
    { .desc = "Open", .shortcut = { .id = 'o', .ctrl = true, }, },
    { .desc = "Close", .shortcut = { .id = 'c', .ctrl = true, }, },
    { .desc = NULL, },
    { .desc = "Quit", .shortcut = { .id = 'q', .ctrl = true, }, },
  };
  struct ncmenu_section sections[] = {
    { .name = "Schwarzgerät", .items = demo_items,
      .itemcount = sizeof(demo_items) / sizeof(*demo_items),
      .shortcut = { .id = 0x00e4, .alt = true, }, },
    { .name = "File", .items = file_items,
      .itemcount = sizeof(file_items) / sizeof(*file_items),
      .shortcut = { .id = 'f', .alt = true, }, },
  };
  ncmenu_options mopts;
  memset(&mopts, 0, sizeof(mopts));
  mopts.sections = sections;
  mopts.sectioncount = sizeof(sections) / sizeof(*sections);
  channels_set_fg(&mopts.headerchannels, 0x00ff00);
  channels_set_bg(&mopts.headerchannels, 0x440000);
  struct ncmenu* top = ncmenu_create(nc, &mopts);
  if(top == NULL){
    goto err;
  }

  struct ncplane* n = notcurses_stdplane(nc);
  int dimy, dimx;
  ncplane_dim_yx(n, &dimy, &dimx);
  uint64_t channels = 0;
  channels_set_fg(&channels, 0x88aa00);
  channels_set_bg(&channels, 0x000088);
  if(ncplane_set_base(n, channels, 0, "x") < 0){
    return EXIT_FAILURE;
  }

  notcurses_render(nc);
  ncplane_set_fg(n, 0x00dddd);
  if(ncplane_putstr_aligned(n, dimy - 1, NCALIGN_RIGHT, " -=+ menu poc. press q to exit +=- ") < 0){
	  return EXIT_FAILURE;
  }
  run_menu(nc, top);

  ncplane_erase(n);
  mopts.bottom = true;
  struct ncmenu* bottom = ncmenu_create(nc, &mopts);
  if(bottom == NULL){
    goto err;
  }
  if(ncplane_putstr_aligned(n, 0, NCALIGN_RIGHT, " -=+ menu poc. press q to exit +=- ") < 0){
	  return EXIT_FAILURE;
  }
  run_menu(nc, top);

  if(notcurses_stop(nc)){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;

err:
  notcurses_stop(nc);
  return EXIT_FAILURE;
}

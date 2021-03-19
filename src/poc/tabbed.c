#include <notcurses/notcurses.h>

#define REDRAW()                       \
  do{                                  \
    nctabbed_redraw(nct);              \
    if(notcurses_render(nc) < 0){      \
      goto ded;                        \
    }                                  \
    notcurses_getc_blocking(nc, NULL); \
  }while(0)

#define TAB1 1
#define TAB2 2
#define TAB3 3
#define TABANOTHER 4
#define ADDEDTAB 5

void tabcbfn(struct nctab* t, struct ncplane* ncp, void* curry){
  ncplane_erase(ncp);
  ncplane_putstr(ncp, "This is the tab content. Nothing to see here...");
  ncplane_cursor_move_yx(ncp, 3, 5);
  switch(*(int*) curry){
    case TAB1:
      ncplane_putstr(ncp, "use left/right arrow keys for navigation, [/] for rotating the tabs, a to add a tab, r to remove a tab, q to quit");
      break;
    case TAB2:
      ncplane_putstr(ncp, "the second tab");
      break;
    case TAB3:
      ncplane_putstr(ncp, "third tab contents");
      break;
    case TABANOTHER:
      ncplane_putstr(ncp, "And the another tab contents i guess");
      break;
    case ADDEDTAB:
      ncplane_putstr(ncp, "this is a tab YOU added!");
      break;
  }
}

int main(void){
  int tab1 = TAB1;
  int tab2 = TAB2;
  int tab3 = TAB3;
  int tabanother = TABANOTHER;
  int addedtab = ADDEDTAB;
  struct notcurses* nc = notcurses_core_init(NULL, NULL);
  if(!nc){
    return EXIT_FAILURE;
  }
  int rows, cols;
  struct ncplane* stdp = notcurses_stddim_yx(nc, &rows, &cols);
  struct ncplane_options popts = {
    .y = 5,
    .x = 5,
    .rows = rows - 10,
    .cols = cols - 10
  };
  struct ncplane* ncp = ncplane_create(stdp, &popts);
  struct nctabbed_options topts = {
    .hdrchan = CHANNELS_RGB_INITIALIZER(255, 0, 0, 0, 0, 0),
    .selchan = CHANNELS_RGB_INITIALIZER(0, 255, 0, 0, 0, 0),
    .flags = 0
  };
  struct nctabbed* nct = nctabbed_create(ncp, &topts);
  ncplane_set_base(nctabbed_content_plane(nct), " ", 0, CHANNELS_RGB_INITIALIZER(255, 255, 255, 15, 15, 15));
  REDRAW();
  nctabbed_add(nct, NULL, NULL, tabcbfn, "Tab #1", &tab1);
  REDRAW();
  nctabbed_add(nct, NULL, NULL, tabcbfn, "Tab #2", &tab3);
  REDRAW();
  nctabbed_add(nct, NULL, NULL, tabcbfn, "Tab #3", &tab3);
  REDRAW();
  nctabbed_add(nct, NULL, NULL, tabcbfn, "Another tab right here", &tabanother);
  REDRAW();
  char32_t c;
  while((c = notcurses_getc_blocking(nc, NULL)) != 'q'){
    switch(c){
      case NCKEY_RIGHT:
        nctabbed_next(nct);
        break;
      case NCKEY_LEFT:
        nctabbed_prev(nct);
        break;
      case '[':
        nctabbed_rotate(nct, -1);
        break;
      case ']':
        nctabbed_rotate(nct, 1);
        break;
      case 'a':
        nctabbed_add(nct, NULL, NULL, tabcbfn, "added tab", &addedtab);
        break;
      case 'r':
        nctabbed_del(nct, nctabbed_selected(nct));
        break;
      default:;
    }
    nctabbed_redraw(nct);
    if(notcurses_render(nc)){
      goto ded;
    }
  }
ded:
  if(notcurses_stop(nc) < 0){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

#include <notcurses/notcurses.h>

#define REDRAW()                       \
  do{                                  \
    nctabbed_redraw(nct);              \
    if(notcurses_render(nc) < 0){      \
      goto ded;                        \
    }                                  \
    notcurses_getc_blocking(nc, NULL); \
  }while(0)

void tabcbfn(struct nctab* t, struct ncplane* ncp, void* curry){
  (void) t;
  (void) curry;
  ncplane_erase(ncp);
  ncplane_puttext(ncp, -1, NCALIGN_LEFT,
                  "Use left/right arrow keys for navigation, "
                  "'[' and ']' to rotate tabs, "
                  "'a' to add a tab, 'r' to remove a tab, "
                  "',' and '.' to move the selected tab, "
                  "and 'q' to quit",
                  NULL);
}

void print_usage(char** argv){
  printf("Usage: %s [ -bht | --bottom | --help | --top ]...\n", argv[0]);
}

int main(int argc, char** argv){
  struct notcurses* nc;
  bool bottom = false;
  for(int i = 1 ; i < argc ; ++i){
    if(strcmp(argv[i], "--help") == 0){
      print_usage(argv);
      return EXIT_SUCCESS;
    }else if(strcmp(argv[i], "--bottom") == 0){
      bottom = true;
    }else if(strcmp(argv[i], "--top") == 0){
      bottom = false;
    }else if(argv[i][0] == '-'){
      for(char* c = &argv[i][1] ; *c ; ++c){
        switch(*c){
          case 'h':
            print_usage(argv);
            return EXIT_SUCCESS;
          case 'b':
            bottom = true;
            break;
          case 't':
            bottom = false;
            break;
          default:
            print_usage(argv);
            return EXIT_FAILURE;
        }
      }
    }else{
      print_usage(argv);
      return EXIT_FAILURE;
    }
  }
  nc = notcurses_core_init(NULL, NULL);
  if(!nc){
    return EXIT_FAILURE;
  }
  int rows, cols;
  struct ncplane* stdp = notcurses_stddim_yx(nc, &rows, &cols);
  struct ncplane_options popts = {
    .y = 3,
    .x = 5,
    .rows = rows - 10,
    .cols = cols - 10
  };
  struct ncplane* ncp = ncplane_create(stdp, &popts);
  struct nctabbed_options topts = {
    .hdrchan = CHANNELS_RGB_INITIALIZER(255, 0, 0, 60, 60, 60),
    .selchan = CHANNELS_RGB_INITIALIZER(0, 255, 0, 0, 0, 0),
    .sepchan = CHANNELS_RGB_INITIALIZER(255, 255, 255, 100, 100, 100),
    .separator = " || ",
    .flags = bottom ? NCTABBED_OPTION_BOTTOM : 0
  };
  struct nctabbed* nct = nctabbed_create(ncp, &topts);
  ncplane_set_base(nctabbed_content_plane(nct), " ", 0, CHANNELS_RGB_INITIALIZER(255, 255, 255, 15, 60, 15));
  REDRAW();
  (void) nctabbed_add(nct, NULL, NULL, tabcbfn, "Tab #1", NULL);
  REDRAW();
  (void) nctabbed_add(nct, NULL, NULL, tabcbfn, "Tab #2", NULL);
  REDRAW();
  (void) nctabbed_add(nct, NULL, NULL, tabcbfn, "Tab #3", NULL);
  REDRAW();
  (void) nctabbed_add(nct, NULL, NULL, tabcbfn, "alpha", NULL);
  REDRAW();
  (void) nctabbed_add(nct, NULL, NULL, tabcbfn, "beta", NULL);
  REDRAW();
  (void) nctabbed_add(nct, NULL, NULL, tabcbfn, "gamma", NULL);
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
      case ',':
        nctab_move_left(nct, nctabbed_selected(nct));
        break;
      case '.':
        nctab_move_right(nct, nctabbed_selected(nct));
        break;
      case 'a':
        (void) nctabbed_add(nct, NULL, NULL, tabcbfn, "added tab", NULL);
        break;
      case 'r':
        nctabbed_del(nct, nctabbed_selected(nct));
        break;
      default:;
    }
    nctabbed_ensure_selected_header_visible(nct);
    nctabbed_redraw(nct);
    if(notcurses_render(nc)){
      goto ded;
    }
  }
  goto fin;
ded:
  notcurses_stop(nc);
  return EXIT_FAILURE;
fin:
  if(notcurses_stop(nc) < 0){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

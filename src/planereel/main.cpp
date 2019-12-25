#include <cstdlib>
#include <clocale>
#include <sstream>
#include <getopt.h>
#include <iostream>
#include <notcurses.h>

// FIXME ought be able to get pr from tablet, methinks?
static struct panelreel* PR;

class TabletCtx {
  public:
    TabletCtx() : lines(rand() % 5 + 3) {}
    int getLines() const {
      return lines;
    }
  private:
    int lines;
};

int tabletfxn(struct tablet* t, int begx, int begy, int maxx, int maxy,
              bool cliptop){
  struct ncplane* p = tablet_ncplane(t);
  TabletCtx *tctx = (TabletCtx*)tablet_userptr(t);
  ncplane_erase(p);
  cell c = CELL_SIMPLE_INITIALIZER(' ');
  cell_set_bg(&c, (((uintptr_t)t) % 0x1000000) + cliptop + begx + maxx);
  ncplane_set_default(p, &c);
  cell_release(p, &c);
  return tctx->getLines() > maxy - begy ? maxy - begy : tctx->getLines();
}

void usage(const char* argv0, std::ostream& c, int status){
  c << "usage: " << argv0 << " [ -h ] | [ -b bordermask ] [ -t tabletmask ]\n";
  c << " -b bordermask: hex panelreel border mask\n";
  c << " -t tabletmask: hex tablet border mask" << std::endl;
  exit(status);
}

void parse_args(int argc, char** argv, struct notcurses_options* opts,
                struct panelreel_options* popts){
  int c;
  while((c = getopt(argc, argv, "b:t:h")) != -1){
    switch(c){
      case 'b':{
        std::stringstream ss;
        ss << std::hex << optarg;
        ss >> popts->bordermask;
        break;
      }case 't':
        // FIXME
        break;
      case 'h':
        usage(argv[0], std::cout, EXIT_SUCCESS);
        break;
      default:
        std::cerr << "Unknown option\n";
        usage(argv[0], std::cerr, EXIT_FAILURE);
        break;
    }
  }
  opts->suppress_bannner = true;
  opts->clear_screen_start = true;
}

int main(int argc, char** argv){
  if(setlocale(LC_ALL, "") == nullptr){
    return EXIT_FAILURE;
  }
  struct notcurses_options opts{};
  struct panelreel_options popts{};
  parse_args(argc, argv, &opts, &popts);
  struct notcurses* nc = notcurses_init(&opts, stdout);
  if(!nc){
    return EXIT_FAILURE;
  }
  struct ncplane* nstd = notcurses_stdplane(nc);
  int dimy, dimx;
  ncplane_dim_yx(nstd, &dimy, &dimx);
  struct ncplane* n = notcurses_newplane(nc, dimy - 1, dimx, 1, 0, nullptr);
  if(!n){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  if(ncplane_set_fg(nstd, 0xb11bb1)){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  if(ncplane_putstr_aligned(nstd, 0, NCALIGN_CENTER, "(a)dd (d)el (q)uit") <= 0){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  channels_set_fg(&popts.focusedchan, 0xffffff);
  channels_set_bg(&popts.focusedchan, 0x00c080);
  popts.bordermask = NCBOXMASK_BOTTOM | NCBOXMASK_TOP |
                      NCBOXMASK_RIGHT | NCBOXMASK_LEFT;
  struct panelreel* pr = panelreel_create(n, &popts, -1);
  if(!pr || notcurses_render(nc)){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  PR = pr; // FIXME eliminate
  char32_t key;
  while((key = notcurses_getc_blocking(nc, nullptr)) != (char32_t)-1){
    switch(key){
      case 'q':
        return notcurses_stop(nc) ? EXIT_FAILURE : EXIT_SUCCESS;
      case 'a':{
        TabletCtx* tctx = new TabletCtx();
        panelreel_add(pr, nullptr, nullptr, tabletfxn, tctx);
        break;
      }
      case 'd':
        panelreel_del_focused(pr);
        break;
      case NCKEY_UP:
        panelreel_prev(pr);
        break;
      case NCKEY_DOWN:
        panelreel_next(pr);
        break;
      default:
        break;
    }
    if(notcurses_render(nc)){
      break;
    }
  }
  notcurses_stop(nc);
  return EXIT_FAILURE;
}

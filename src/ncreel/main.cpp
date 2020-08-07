#include <cstdlib>
#include <clocale>
#include <sstream>
#include <getopt.h>
#include <iostream>
#include <memory>
#include <ncpp/NotCurses.hh>
#include <ncpp/Reel.hh>
#include <ncpp/NCKey.hh>

using namespace ncpp;

class TabletCtx {
  public:
    TabletCtx() :
      lines(rand() % 5 + 3),
      rgb(rand() % 0x1000000),
      idx(++class_idx) {}
    int getLines() const {
      return lines;
    }
    void addLine() {
      ++lines;
    }
    void subLine() {
      if(lines){
        --lines;
      }
    }
    int getIdx() const {
      return idx;
    }
    unsigned getRGB() const {
      return rgb;
    }
  private:
    int lines;
    unsigned rgb;
    int idx;
    inline static int class_idx = 0;
};

int tabletfxn(struct nctablet* _t, bool cliptop){
  (void)cliptop; // FIXME
  NcTablet *t = NcTablet::map_tablet (_t);
  Plane* p = t->get_plane();
  auto tctx = t->get_userptr<TabletCtx>();
  p->erase();
  Cell c(' ');
  c.set_bg(tctx->getRGB());
  p->set_base_cell(c);
  p->release(c);
  p->set_bg(0xffffff);
  p->set_fg(0x000000);
  p->printf(1, 1, "%d %p lines: %d", tctx->getIdx(), _t, tctx->getLines());
  return tctx->getLines();
}

void usage(const char* argv0, std::ostream& c, int status){
  c << "usage: " << argv0 << " [ -h ] | options\n"
    << " -b bordermask: hex ncreel border mask (0x0..0xf)\n"
    << " -m margin(s): margin(s) around the reel\n"
    << " -t tabletmask: hex tablet border mask (0x0..0xf)" << std::endl;
  exit(status);
}

void parse_args(int argc, char** argv, struct notcurses_options* opts,
                struct ncreel_options* ropts){
  const struct option longopts[] = {
    { .name = nullptr, .has_arg = 0, .flag = nullptr, .val = 0, },
  };
  int c;
  while((c = getopt_long(argc, argv, "b:t:m:h", longopts, nullptr)) != -1){
    switch(c){
      case 'b':{
        std::stringstream ss;
        ss << std::hex << optarg;
        ss >> ropts->bordermask;
        break;
      }case 't':{
        std::stringstream ss;
        ss << std::hex << optarg;
        ss >> ropts->tabletmask;
        break;
      }case 'm':{
        if(notcurses_lex_margins(optarg, opts)){
          usage(argv[0], std::cout, EXIT_FAILURE);
        }
        break;
      }case 'h':
        usage(argv[0], std::cout, EXIT_SUCCESS);
        break;
      default:
        std::cerr << "Unknown option\n";
        usage(argv[0], std::cerr, EXIT_FAILURE);
        break;
    }
  }
  opts->flags |= NCOPTION_SUPPRESS_BANNERS;
}

int runreels(struct notcurses* nc, struct ncplane* n, ncreel_options* nopts){
  if(ncplane_set_fg_rgb(n, 0xb1, 0x1b, 0xb1)){
    return -1;
  }
  channels_set_fg(&nopts->focusedchan, 0xffffff);
  channels_set_bg(&nopts->focusedchan, 0x00c080);
  channels_set_fg(&nopts->borderchan, 0x00c080);
  auto nr = ncreel_create(n, nopts);
  if(!nr || notcurses_render(nc)){
    return -1;
  }
  int y, x;
  char32_t key;
  ncinput ni;
  while((key = notcurses_getc_blocking(nc, &ni)) != (char32_t)-1){
    switch(key){
      case 'q':
        return 0;
      case 'a':{
        auto tctx = new TabletCtx();
        ncreel_add(nr, nullptr, nullptr, tabletfxn, tctx);
        break;
      }
      case 'd':
        ncreel_del(nr, ncreel_focused(nr));
        break;
      case '+':{
        auto t = ncreel_focused(nr);
        if(t){
          auto tctx = static_cast<TabletCtx*>(nctablet_userptr(t));
          tctx->addLine();
        }
        break;
      }case '-':{
        auto t = ncreel_focused(nr);
        if(t){
          auto tctx = static_cast<TabletCtx*>(nctablet_userptr(t));
          tctx->subLine();
        }
        break;
      }case '*':
        notcurses_debug(nc, stderr);
        break;
      case NCKEY_LEFT:
        ncplane_yx(ncreel_plane(nr), &y, &x);
        ncplane_move_yx(ncreel_plane(nr), y, x - 1);
        break;
      case NCKEY_RIGHT:
        ncplane_yx(ncreel_plane(nr), &y, &x);
        ncplane_move_yx(ncreel_plane(nr), y, x + 1);
        break;
      case NCKEY_UP:
        ncreel_prev(nr);
        break;
      case NCKEY_DOWN:
        ncreel_next(nr);
        break;
      default:
        break;
    }
    if(ncreel_redraw(nr)){
      break;
    }
    if(notcurses_render(nc)){
      break;
    }
  }
  return -1;
}

int main(int argc, char** argv){
  if(setlocale(LC_ALL, "") == nullptr){
    return EXIT_FAILURE;
  }
  notcurses_options ncopts{};
  ncreel_options nopts{};
  parse_args(argc, argv, &ncopts, &nopts);
  int margin_t = ncopts.margin_t;
  int margin_l = ncopts.margin_l;
  int margin_r = ncopts.margin_r;
  int margin_b = ncopts.margin_b;
  ncopts.margin_t = ncopts.margin_l = ncopts.margin_r = ncopts.margin_b = 0;
  auto nc = notcurses_init(&ncopts, NULL);
  if(nc == nullptr){
    return EXIT_FAILURE;
  }
  int dimy, dimx;
  auto nstd = notcurses_stddim_yx(nc, &dimy, &dimx);
  if(ncplane_putstr_aligned(nstd, 0, NCALIGN_CENTER, "(a)dd (d)el (+/-) change lines (q)uit") <= 0){
    return -1;
  }
  auto n = ncplane_new(nc, dimy - 1 - (margin_t + margin_b),
                       dimx - (margin_r + margin_l),
                       1 + margin_t, margin_l, nullptr);
  if(!n){
    return -1;
  }
  int r = runreels(nc, n, &nopts);
  if(notcurses_stop(nc)){
    return EXIT_FAILURE;
  }
  return r ? EXIT_FAILURE : EXIT_SUCCESS;
}

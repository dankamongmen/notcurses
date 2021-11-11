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
    unsigned getLines() const {
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

int tabletfxn(struct nctablet* _t, bool cliptop __attribute__ ((unused))){
  NcTablet *t = NcTablet::map_tablet(_t);
  Plane* p = t->get_plane();
  auto tctx = t->get_userptr<TabletCtx>();
  p->erase();
  Cell c(' ');
  c.set_bg_rgb(tctx->getRGB());
  p->set_base_cell(c);
  p->release(c);
  p->set_bg_rgb(0xffffff);
  p->set_fg_rgb(0x000000);
  unsigned ret = tctx->getLines();
  if(ret > p->get_dim_y()){
    ret = p->get_dim_y();
  }
  p->printf(1, 1, "%d %p lines: %d (showing: %d)", tctx->getIdx(), _t, tctx->getLines(), ret);
  return ret;
}

void usage(const char* argv0, std::ostream& c, int status){
  c << "usage: " << argv0 << " [ -h ] | options\n"
    << " -b bordermask: hex ncreel border mask (0x0..0xf)\n"
    << " -m margin(s): margin(s) around the reel\n"
    << " -ln: logging level, higher n for more output (0 ≤ n ≤ 8)\n"
    << " -t tabletmask: hex tablet border mask (0x0..0xf)" << std::endl;
  exit(status);
}

void parse_args(int argc, char** argv, struct notcurses_options* opts,
                struct ncreel_options* ropts){
  const struct option longopts[] = {
    { .name = nullptr, .has_arg = 0, .flag = nullptr, .val = 0, },
  };
  int c;
  while((c = getopt_long(argc, argv, "l:b:t:m:h", longopts, nullptr)) != -1){
    switch(c){
      case 'l':{
        int loglevel;
        std::stringstream s(optarg);
        s >> loglevel;
        opts->loglevel = static_cast<ncloglevel_e>(loglevel);
        if(opts->loglevel < NCLOGLEVEL_SILENT || opts->loglevel > NCLOGLEVEL_TRACE){
          fprintf(stderr, "Invalid log level: %d\n", opts->loglevel);
          usage(argv[0], std::cerr, EXIT_FAILURE);
        }
        break;
      }case 'b':{
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

int runreels(struct notcurses* nc, struct ncreel* nr){
  if(notcurses_render(nc)){
    return -1;
  }
  int y, x;
  char32_t key;
  ncinput ni;
  while((key = notcurses_get_blocking(nc, &ni)) != (char32_t)-1){
    if(ni.evtype == EvType::Release){
      continue;
    }
    switch(key){
      case 'q':
        return 0;
      case 'a':{
        auto tctx = new TabletCtx();
        if(!ncreel_add(nr, nullptr, nullptr, tabletfxn, tctx)){
          return -1;
        }
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
        if(ncreel_redraw(nr)){
          fprintf(stderr, "Error redrawing reel\n");
          return -1;
        }
        break;
      }case '-':{
        auto t = ncreel_focused(nr);
        if(t){
          auto tctx = static_cast<TabletCtx*>(nctablet_userptr(t));
          tctx->subLine();
        }
        if(ncreel_redraw(nr)){
          fprintf(stderr, "Error redrawing reel\n");
          return -1;
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
    if(notcurses_render(nc)){
      break;
    }
  }
  return -1;
}

static int
resize_reel(struct ncplane* n){
  const struct ncplane* p = ncplane_parent_const(n);
  unsigned py, px;
  ncplane_dim_yx(p, &py, &px);
  if(ncplane_resize(n, 0, 0, 0, 0, 0, 0, py - 1, px) < 0){
    return -1;
  }
  auto nr = static_cast<struct ncreel*>(ncplane_userptr(n));
  ncreel_redraw(nr);
  return 0;
}

int main(int argc, char** argv){
  if(setlocale(LC_ALL, "") == nullptr){
    return EXIT_FAILURE;
  }
  notcurses_options ncopts{};
  ncreel_options ropts{};
  parse_args(argc, argv, &ncopts, &ropts);
  auto nc = notcurses_init(&ncopts, NULL);
  if(nc == nullptr){
    return EXIT_FAILURE;
  }
  unsigned dimy, dimx;
  auto nstd = notcurses_stddim_yx(nc, &dimy, &dimx);
  struct ncplane* n;
  if(ncplane_putstr_aligned(nstd, 0, NCALIGN_CENTER, "(a)dd (d)el (+/-) change lines (q)uit") <= 0){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  struct ncplane_options nopts = {
    .y = 1,
    .x = NCALIGN_CENTER,
    .rows = dimy - 1,
    .cols = dimx,
    .userptr = nullptr,
    .name = "reel",
    .resizecb = resize_reel,
    .flags = NCPLANE_OPTION_HORALIGNED,
    .margin_b = 0, .margin_r = 0,
  };
  n = ncplane_create(nstd, &nopts);
  if(!n){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  if(ncplane_set_fg_rgb8(n, 0xb1, 0x1b, 0xb1)){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  ncchannels_set_fg_rgb(&ropts.focusedchan, 0xffffff);
  ncchannels_set_bg_rgb(&ropts.focusedchan, 0x00c080);
  ncchannels_set_fg_rgb(&ropts.borderchan, 0x00c080);
  auto nr = ncreel_create(n, &ropts);
  ncplane_set_userptr(n, nr);
  if(!nr){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  int r = runreels(nc, nr);
  ncreel_destroy(nr);
  if(notcurses_stop(nc)){
    return EXIT_FAILURE;
  }
  return r ? EXIT_FAILURE : EXIT_SUCCESS;
}

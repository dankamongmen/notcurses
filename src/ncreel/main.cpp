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
      rgb(rand() % 0x1000000) {}
    int getLines() const {
      return lines;
    }
    unsigned getRGB() const {
      return rgb;
    }
  private:
    int lines;
    unsigned rgb;
};

int tabletfxn(struct nctablet* _t, int begx, int begy, int maxx, int maxy,
              bool cliptop){
  (void)begx;
  (void)begy;
  (void)maxx;
  (void)cliptop;
  NcTablet *t = NcTablet::map_tablet (_t);
  Plane* p = t->get_plane();
  auto tctx = t->get_userptr<TabletCtx>();
  p->erase();
  Cell c(' ');
  c.set_bg(tctx->getRGB());
  p->set_base_cell(c);
  p->release(c);
  return tctx->getLines() > maxy - begy ? maxy - begy : tctx->getLines();
}

void usage(const char* argv0, std::ostream& c, int status){
  c << "usage: " << argv0 << " [ -h ] | options\n";
  c << " -b bordermask: hex ncreel border mask (0x0..0xf)\n";
  c << " -t tabletmask: hex tablet border mask (0x0..0xf)" << std::endl;
  exit(status);
}

void parse_args(int argc, char** argv, struct notcurses_options* opts,
                struct ncreel_options* ropts){
  const struct option longopts[] = {
    { .name = nullptr, .has_arg = 0, .flag = nullptr, .val = 0, },
  };
  int c;
  while((c = getopt_long(argc, argv, "b:t:h", longopts, nullptr)) != -1){
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

int runreels(NotCurses& nc, ncreel_options& nopts){
  std::unique_ptr<Plane> nstd(nc.get_stdplane());
  int dimy, dimx;
  nstd->get_dim(&dimy, &dimx);
  auto n = std::make_shared<Plane>(dimy - 1, dimx, 1, 0);
  if(!n){
    return -1;
  }
  if(!n->set_fg_rgb(0xb1, 0x1b, 0xb1)){
    return -1;
  }
  if(n->putstr(0, NCAlign::Center, "(a)dd (d)el (q)uit") <= 0){
    return -1;
  }
  channels_set_fg(&nopts.focusedchan, 0xffffff);
  channels_set_bg(&nopts.focusedchan, 0x00c080);
  channels_set_fg(&nopts.borderchan, 0x00c080);
  std::shared_ptr<NcReel> nr(n->ncreel_create(&nopts));
  if(!nr || !nc.render()){
    return -1;
  }
  int y, x;
  char32_t key;
  while((key = nc.getc(true)) != (char32_t)-1){
    switch(key){
      case 'q':
        return 0;
      case 'a':{
        auto tctx = new TabletCtx();
        nr->add(nullptr, nullptr, tabletfxn, tctx);
        break;
      }
      case 'd':
        nr->del_focused();
        break;
      case '*':
        notcurses_debug(nc, stderr);
        break;
      case NCKEY_LEFT:
        nr->get_plane()->get_yx(&y, &x);
        nr->move(y, x - 1);
        break;
      case NCKEY_RIGHT:
        nr->get_plane()->get_yx(&y, &x);
        nr->move(y, x + 1);
        break;
      case NCKEY_UP:
        nr->prev();
        break;
      case NCKEY_DOWN:
        nr->next();
        break;
      default:
        break;
    }
    if(!nc.render()){
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
  NotCurses nc(ncopts);
  int r = runreels(nc, nopts);
  nc.stop();
  return r ? EXIT_FAILURE : EXIT_SUCCESS;
}

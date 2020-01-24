#include <cstdlib>
#include <clocale>
#include <sstream>
#include <getopt.h>
#include <iostream>
#include <memory>
#include <ncpp/NotCurses.hh>
#include <ncpp/PanelReel.hh>
#include <ncpp/NCKey.hh>

using namespace ncpp;

class TabletCtx {
  public:
    TabletCtx() : lines(rand() % 5 + 3) {}
    int getLines() const {
      return lines;
    }
  private:
    int lines;
};

int tabletfxn(struct tablet* _t, int begx, int begy, int maxx, int maxy,
              bool cliptop){
  Tablet *t = Tablet::map_tablet (_t);
  Plane* p = t->get_plane();
  auto tctx = t->get_userptr<TabletCtx>();
  p->erase();
  Cell c(' ');
  c.set_bg((((uintptr_t)t) % 0x1000000) + cliptop + begx + maxx);
  p->set_base(c);
  p->release(c);
  return tctx->getLines() > maxy - begy ? maxy - begy : tctx->getLines();
}

void usage(const char* argv0, std::ostream& c, int status){
  c << "usage: " << argv0 << " [ -h ] | options\n";
  c << " --ot: offset from top\n";
  c << " --ob: offset from bottom\n";
  c << " --ol: offset from left\n";
  c << " --or: offset from right\n";
  c << " -b bordermask: hex panelreel border mask (0x0..0xf)\n";
  c << " -t tabletmask: hex tablet border mask (0x0..0xf)" << std::endl;
  exit(status);
}

constexpr int OPT_TOPOFF = 100;
constexpr int OPT_BOTTOMOFF = 101;
constexpr int OPT_LEFTOFF = 102;
constexpr int OPT_RIGHTOFF = 103;

void parse_args(int argc, char** argv, struct notcurses_options* opts,
                struct panelreel_options* popts){
  const struct option longopts[] = {
    { .name = "ot", .has_arg = 1, .flag = nullptr, OPT_TOPOFF, },
    { .name = "ob", .has_arg = 1, .flag = nullptr, OPT_BOTTOMOFF, },
    { .name = "ol", .has_arg = 1, .flag = nullptr, OPT_LEFTOFF, },
    { .name = "or", .has_arg = 1, .flag = nullptr, OPT_RIGHTOFF, },
    { .name = nullptr, .has_arg = 0, .flag = nullptr, 0, },
  };
  int c;
  while((c = getopt_long(argc, argv, "b:t:h", longopts, nullptr)) != -1){
    switch(c){
      case OPT_BOTTOMOFF:{
        std::stringstream ss;
        ss << optarg;
        ss >> popts->boff;
        break;
      }case OPT_TOPOFF:{
        std::stringstream ss;
        ss << optarg;
        ss >> popts->toff;
        break;
      }case OPT_LEFTOFF:{
        std::stringstream ss;
        ss << optarg;
        ss >> popts->loff;
        break;
      }case OPT_RIGHTOFF:{
        std::stringstream ss;
        ss << optarg;
        ss >> popts->roff;
        break;
      }case 'b':{
        std::stringstream ss;
        ss << std::hex << optarg;
        ss >> popts->bordermask;
        break;
      }case 't':{
        std::stringstream ss;
        ss << std::hex << optarg;
        ss >> popts->tabletmask;
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
  opts->suppress_banner = true;
}

int main(int argc, char** argv){
  if(setlocale(LC_ALL, "") == nullptr){
    return EXIT_FAILURE;
  }
  parse_args(argc, argv, &NotCurses::default_notcurses_options, &PanelReel::default_options);
  NotCurses nc;
  std::unique_ptr<Plane> nstd(nc.get_stdplane());
  int dimy, dimx;
  nstd->get_dim(&dimy, &dimx);
  auto n = std::make_shared<Plane>(dimy - 1, dimx, 1, 0);
  if(!n){
    return EXIT_FAILURE;
  }
  if(!nstd->set_fg(0xb11bb1)){
    return EXIT_FAILURE;
  }
  if(nstd->putstr(0, NCAlign::Center, "(a)dd (d)el (q)uit") <= 0){
    return EXIT_FAILURE;
  }
  channels_set_fg(&PanelReel::default_options.focusedchan, 0xffffff);
  channels_set_bg(&PanelReel::default_options.focusedchan, 0x00c080);
  channels_set_fg(&PanelReel::default_options.borderchan, 0x00c080);
  std::shared_ptr<PanelReel> pr(n->panelreel_create());
  if(!pr || !nc.render()){
    return EXIT_FAILURE;
  }
  char32_t key;
  while((key = nc.getc(true)) != (char32_t)-1){
    switch(key){
      case 'q':
        return !nc.stop() ? EXIT_FAILURE : EXIT_SUCCESS;
      case 'a':{
        TabletCtx* tctx = new TabletCtx();
        pr->add(nullptr, nullptr, tabletfxn, tctx);
        break;
      }
      case 'd':
        pr->del_focused();
        break;
      case NCKEY_UP:
        pr->prev();
        break;
      case NCKEY_DOWN:
        pr->next();
        break;
      default:
        break;
    }
    if(!nc.render()){
      break;
    }
  }
  return EXIT_FAILURE;
}

#include <memory>
#include <unistd.h>
#include <cstdlib>
#include <clocale>
#include <iostream>
#include <ncpp/Reader.hh>
#include <ncpp/NotCurses.hh>

using namespace ncpp;

auto usage(const char* argv0, int ret) -> void {
  std::cerr << "usage: " << argv0 << " [ -hs ]" << std::endl;
  exit(ret);
}

auto main(int argc, const char** argv) -> int {
  if(!setlocale(LC_ALL, "")){
    std::cout << "Error setting locale\n";
    return EXIT_FAILURE;
  }
  bool horscroll = false;
  if(argc == 2){
    if(strcmp(argv[1], "-hs") == 0){
      horscroll = true;
    }else{
      usage(argv[0], EXIT_FAILURE);
    }
  }else if(argc > 2){
    usage(argv[0], EXIT_FAILURE);
  }
  notcurses_options ncopts{};
  ncopts.flags = NCOPTION_INHIBIT_SETLOCALE;
  NotCurses nc(ncopts);
  unsigned dimy, dimx;
  auto n = std::make_unique<Plane *>(nc.get_stdplane(&dimy, &dimx));
  nc.get_term_dim(&dimy, &dimx);
  ncreader_options opts{};
  opts.flags = NCREADER_OPTION_CURSOR | (horscroll ? NCREADER_OPTION_HORSCROLL : 0);
  // can't use Plane until we have move constructor for Reader
  struct ncplane_options nopts = {
    .y = 2,
    .x = 2,
    .rows = dimy / 8,
    .cols = dimx / 2,
    .userptr = nullptr,
    .name = "read",
    .resizecb = nullptr,
    .flags = 0,
    .margin_b = 0, .margin_r = 0,
  };
  struct ncplane* rp = ncplane_create(**n, &nopts);
  ncplane_set_base(rp, "░", 0, 0);
  auto nr = ncreader_create(rp, &opts);
  if(nr == nullptr){
    return EXIT_FAILURE;
  }
  ncinput ni;
  unsigned tgeomy, tgeomx, vgeomy, vgeomx;
  struct ncplane* ncp = ncreader_plane(nr);
  struct ncplane* tplane = ncplane_above(ncp);
  ncplane_dim_yx(tplane, &tgeomy, &tgeomx);
  ncplane_dim_yx(ncp, &vgeomy, &vgeomx);
  (*n)->printf(0, 0, "Scroll: %c Cursor: 000/000 Viewgeom: %03u/%03u Textgeom: %03u/%03u",
               horscroll ? '+' : '-', vgeomy, vgeomx, tgeomy, tgeomx);
  nc.render();
  while(nc.get(true, &ni) != (char32_t)-1){
    if(ni.evtype == EvType::Release){
      continue;
    }
    if(ni.ctrl && ni.id == 'L'){
      notcurses_refresh(nc, NULL, NULL);
    }else if((ni.ctrl && ni.id == 'D') || ni.id == NCKEY_ENTER){
      break;
    }else if(ncreader_offer_input(nr, &ni)){
      unsigned ncpy, ncpx;
      ncplane_cursor_yx(ncp, &ncpy, &ncpx);
      ncplane_dim_yx(tplane, &tgeomy, &tgeomx);
      ncplane_dim_yx(ncp, &vgeomy, &vgeomx);
      (*n)->printf(0, 0, "Scroll: %c Cursor: %03u/%03u Viewgeom: %03u/%03u Textgeom: %03u/%03u",
                   horscroll ? '+' : '-', ncpy, ncpx, vgeomy, vgeomx, tgeomy, tgeomx);
      nc.render();
    }
  }
  nc.render();
  char* contents;
  ncreader_destroy(nr, &contents);
  nc.stop();
  if(contents){
    printf("\n input: %s\n", contents);
  }
  return EXIT_SUCCESS;
}

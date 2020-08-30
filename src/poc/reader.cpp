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
  notcurses_options nopts{};
  nopts.flags = NCOPTION_INHIBIT_SETLOCALE;
  NotCurses nc(nopts);
  int dimy, dimx;
  std::unique_ptr<Plane *> n = std::make_unique<Plane *>(nc.get_stdplane(&dimy, &dimx));
  nc.get_term_dim(&dimy, &dimx);
  ncreader_options opts{};
  opts.physrows = dimy / 8;
  opts.physcols = dimx / 2;
  opts.egc = "░";
  opts.flags = horscroll ? NCREADER_OPTION_HORSCROLL : 0;
  // FIXME c++ is crashing
  //Reader nr(nc, 0, 0, &opts);
  auto nr = ncreader_create(**n, 2, 2, &opts);
  if(nr == nullptr){
    return EXIT_FAILURE;
  }
  if(!nc.cursor_enable(2, 2)){
    return EXIT_FAILURE;
  }
  ncinput ni;
  nc.render();
  while(nc.getc(true, &ni) != (char32_t)-1){
fprintf(stderr, "READ ME %lc\n", ni.id);
    if(ni.ctrl && ni.id == 'L'){
      notcurses_refresh(nc, NULL, NULL);
    }else if((ni.ctrl && ni.id == 'D') || ni.id == NCKEY_ENTER){
      break;
    }else if(ncreader_offer_input(nr, &ni)){
      int y, x;
      struct ncplane* ncp = ncreader_plane(nr);
      ncplane_cursor_yx(ncp, &y, &x);
      nc.cursor_enable(y + 2, 2 + (x >= ncplane_dim_x(ncp) ? ncplane_dim_x(ncp) - 1 : x));
      int ncpy, ncpx;
      ncplane_cursor_yx(ncp, &ncpy, &ncpx);
      struct ncplane* tplane = ncplane_above(ncp);
      int tgeomy, tgeomx, vgeomy, vgeomx;
      ncplane_dim_yx(tplane, &tgeomy, &tgeomx);
      ncplane_dim_yx(ncp, &vgeomy, &vgeomx);
      (*n)->printf(0, 0, "Scroll: %lc Cursor: %03d/%03d Viewgeom: %03d/%03d Textgeom: %03d/%03d",
                  horscroll ? L'✔' : L'🗴', ncpy, ncpx, vgeomy, vgeomx, tgeomy, tgeomx);
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

#include <cstdlib>
#include <ncpp/NotCurses.hh>

int main(void){
  if(setlocale(LC_ALL, "") == nullptr){
    return EXIT_FAILURE;
  }
  ncpp::NotCurses nc;
  if(!nc.mouse_enable()){
    return EXIT_FAILURE;
  }
  std::unique_ptr<ncpp::Plane> n(nc.get_stdplane ());
  ncpp::Cell tl, tr, bl, br, hl, vl;
  if(!n->load_double_box(0, 0, tl, tr, bl, br, hl, vl)){
    return EXIT_FAILURE;
  }
  if(!n->perimeter(tl, tr, bl, br, hl, vl, 0)){
    return EXIT_FAILURE;
  }
  ncpp::Plane plotplane{6, 70, 1, 1, nullptr};
  struct ncplot_options popts{};
  popts.rangex = 60;
  popts.detectrange = true;
  struct ncplot* plot = ncplot_create(plotplane, &popts);
  char32_t r;
  ncinput ni;
  // FIXME launch ticker thread
  if(!nc.render()){
    return EXIT_FAILURE;
  }
  while(errno = 0, (r = nc.getc(true, &ni)) != (char32_t)-1){
    if(r == 0){ // interrupted by signal
      continue;
    }
  }
  ncplot_destroy(plot);
  return EXIT_SUCCESS;
}

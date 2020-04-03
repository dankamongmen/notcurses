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
  struct ncplot_options popts{};
  struct ncplot* plot = ncplot_create(*n, &popts);
  char32_t r;
  ncinput ni;
  while(errno = 0, (r = nc.getc(true, &ni)) != (char32_t)-1){
    if(r == 0){ // interrupted by signal
      continue;
    }
  }
  ncplot_destroy(plot);
  return EXIT_SUCCESS;
}

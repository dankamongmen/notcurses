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
  // FIXME
  ncplot_destroy(plot);
  return EXIT_SUCCESS;
}

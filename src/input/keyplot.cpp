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
  // FIXME
  return EXIT_SUCCESS;
}

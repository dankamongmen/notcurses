#include <locale.h>
#include <unistd.h>
#include <notcurses/notcurses.h>

// can we leave what was already on the screen there? (narrator: it seems not)
int main(void){
  if(!setlocale(LC_ALL, "")){
    fprintf(stderr, "Couldn't set locale\n");
    return EXIT_FAILURE;
  }
  struct ncdirect* n; // see bug #391
  if((n = ncdirect_init(NULL, stdout)) == NULL){
    return EXIT_FAILURE;
  }
  if(ncdirect_render_image(n, "../data/normal.png", NCBLIT_DEFAULT, NCSCALE_STRETCH) != NCERR_SUCCESS){
    return EXIT_FAILURE;
  }
  sleep(1);
  if(ncdirect_render_image(n, "../data/changes.jpg", NCBLIT_DEFAULT, NCSCALE_STRETCH) != NCERR_SUCCESS){
    return EXIT_FAILURE;
  }
  if(ncdirect_stop(n)){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

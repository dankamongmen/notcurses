#include <locale.h>
#include <unistd.h>
#include <notcurses/direct.h>

// can we leave what was already on the screen there? (narrator: it seems not)
int main(void){
  if(!setlocale(LC_ALL, "")){
    fprintf(stderr, "Couldn't set locale\n");
    return EXIT_FAILURE;
  }
  struct ncdirect* n; // see bug #391
  if((n = ncdirect_init(NULL, stdout, 0)) == NULL){
    return EXIT_FAILURE;
  }
  if(!ncdirect_canopen_images(n)){
    fprintf(stderr, "This notcurses was not build with multimedia support.\n");
    return EXIT_FAILURE;
  }
  ncdirect_fg_rgb8(n, 0xff, 0, 0xff);
  ncdirect_bg_rgb8(n, 0, 0xff, 0);
  ncdirect_printf_aligned(n, -1, NCALIGN_CENTER, "let's play!");
  if(ncdirect_render_image(n, "../data/normal.png", NCALIGN_LEFT,
                           NCBLIT_DEFAULT, NCSCALE_STRETCH)){
    return EXIT_FAILURE;
  }
  if(ncdirect_render_image(n, "../data/normal.png", NCALIGN_CENTER,
                           NCBLIT_DEFAULT, NCSCALE_STRETCH)){
    return EXIT_FAILURE;
  }
  if(ncdirect_render_image(n, "../data/normal.png", NCALIGN_RIGHT,
                           NCBLIT_DEFAULT, NCSCALE_STRETCH)){
    return EXIT_FAILURE;
  }
  sleep(1);
  if(ncdirect_clear(n)){
    return EXIT_FAILURE;
  }
  if(ncdirect_render_image(n, "../data/changes.jpg", NCALIGN_LEFT,
                           NCBLIT_DEFAULT, NCSCALE_SCALE)){
    return EXIT_FAILURE;
  }
  if(ncdirect_render_image(n, "../data/changes.jpg", NCALIGN_CENTER,
                           NCBLIT_DEFAULT, NCSCALE_SCALE)){
    return EXIT_FAILURE;
  }
  if(ncdirect_render_image(n, "../data/changes.jpg", NCALIGN_RIGHT,
                           NCBLIT_DEFAULT, NCSCALE_SCALE)){
    return EXIT_FAILURE;
  }
  sleep(1);
  if(ncdirect_clear(n)){
    return EXIT_FAILURE;
  }
  if(ncdirect_render_image(n, "../data/warmech.bmp", NCALIGN_RIGHT,
                           NCBLIT_DEFAULT, NCSCALE_NONE)){
    return EXIT_FAILURE;
  }
  if(ncdirect_render_image(n, "../data/warmech.bmp", NCALIGN_LEFT,
                           NCBLIT_DEFAULT, NCSCALE_NONE)){
    return EXIT_FAILURE;
  }
  if(ncdirect_stop(n)){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

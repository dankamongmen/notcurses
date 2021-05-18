#include <locale.h>
#include <unistd.h>
#include <notcurses/direct.h>

// print progressive partial subsets of the image
static int
partial_image(struct ncdirect* n, const char* file){
  ncdirectf* nf = ncdirectf_from_file(n, file);
  if(nf == NULL){
    return -1;
  }
  // get the number of pixels
  ncvgeom geom;
  ncblitter_e blit = NCBLIT_1x1;
  ncdirectf_geom(n, nf, &blit, NCSCALE_NONE, 0, 0, &geom);
  if(geom.cdimy <= 0){
    fprintf(stderr, "no cell dim information\n");
    ncdirectf_free(nf);
    return -1;
  }
  for(int y = geom.pixy ; y > 0 ; y -= 5){
    int rows = y;
    for(int x = geom.pixx ; x > 0 ; x -= 5){
      int cols = x;
      ncdirectv* v;
      printf("Size: %dx%d\n", cols, rows);
      if((v = ncdirectf_render(n, nf, blit, NCSCALE_NONE, rows, cols)) == NULL){
        ncdirectf_free(nf);
        return -1;
      }
      if(ncdirect_raster_frame(n, v, NCALIGN_CENTER)){
        ncdirectf_free(nf);
        return -1;
      }
    }
  }
  ncdirectf_free(nf);
  return 0;
}

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
    goto err;
  }
  ncdirect_set_fg_rgb8(n, 0xff, 0, 0xff);
  ncdirect_set_bg_rgb8(n, 0, 0xff, 0);
  ncdirect_printf_aligned(n, -1, NCALIGN_CENTER, "let's play!");
  if(ncdirect_render_image(n, "../data/normal.png", NCALIGN_LEFT,
                           NCBLIT_DEFAULT, NCSCALE_STRETCH)){
    goto err;
  }
  if(ncdirect_render_image(n, "../data/normal.png", NCALIGN_CENTER,
                           NCBLIT_DEFAULT, NCSCALE_STRETCH)){
    goto err;
  }
  if(ncdirect_render_image(n, "../data/normal.png", NCALIGN_RIGHT,
                           NCBLIT_DEFAULT, NCSCALE_STRETCH)){
    goto err;
  }
  sleep(1);
  if(ncdirect_clear(n)){
    goto err;
  }
  if(ncdirect_render_image(n, "../data/changes.jpg", NCALIGN_LEFT,
                           NCBLIT_DEFAULT, NCSCALE_SCALE)){
    goto err;
  }
  if(ncdirect_render_image(n, "../data/changes.jpg", NCALIGN_CENTER,
                           NCBLIT_DEFAULT, NCSCALE_SCALE)){
    goto err;
  }
  if(ncdirect_render_image(n, "../data/changes.jpg", NCALIGN_RIGHT,
                           NCBLIT_DEFAULT, NCSCALE_SCALE)){
    goto err;
  }
  sleep(1);
  if(ncdirect_clear(n)){
    goto err;
  }
  if(ncdirect_render_image(n, "../data/warmech.bmp", NCALIGN_RIGHT,
                           NCBLIT_DEFAULT, NCSCALE_NONE)){
    goto err;
  }
  if(ncdirect_render_image(n, "../data/warmech.bmp", NCALIGN_LEFT,
                           NCBLIT_DEFAULT, NCSCALE_NONE)){
    goto err;
  }
  if(ncdirect_stop(n)){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;

err:
  ncdirect_stop(n);
  return EXIT_FAILURE;
}

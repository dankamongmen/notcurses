#include <pthread.h>
#include "demo.h"

static void*
fadethread(void* vncp){
  struct timespec fade = { .tv_sec = 5, .tv_nsec = 0, };
  ncplane_fadeout(vncp, &fade);
  return NULL;
}

static struct ncplane*
outro_message(struct notcurses* nc, int* rows, int* cols){
  const char str0[] = " ATL, baby! ATL! ";
  const char str1[] = " much, much more is coming ";
  const char str2[] = " hack on! —dank❤ ";
  int xstart = (*cols - (strlen(str1) + 4)) / 2;
  int ystart = *rows - 6;
  struct ncplane* on = notcurses_newplane(nc, 5, strlen(str1) + 4, ystart, xstart, NULL);
  if(on == NULL){
    return NULL;
  }
  cell bgcell = CELL_SIMPLE_INITIALIZER(' ');
  channels_set_bg_rgb(&bgcell.channels, 0x58, 0x36, 0x58);
  if(ncplane_set_default(on, &bgcell) < 0){
    return NULL;
  }
  ncplane_dim_yx(on, rows, cols);
  int ybase = 0;
  // bevel the upper corners
  if(ncplane_set_bg_alpha(on, CELL_ALPHA_TRANS)){
    return NULL;
  }
  if(ncplane_cursor_move_yx(on, ybase, 0)){
    return NULL;
  }
  if(ncplane_putsimple(on, ' ') < 0 || ncplane_putsimple(on, ' ') < 0){
    return NULL;
  }
  if(ncplane_cursor_move_yx(on, ybase, *cols - 2)){
    return NULL;
  }
  if(ncplane_putsimple(on, ' ') < 0 || ncplane_putsimple(on, ' ') < 0){
    return NULL;
  }
  // ...and now the lower corners
  if(ncplane_cursor_move_yx(on, *rows - 1, 0)){
    return NULL;
  }
  if(ncplane_putsimple(on, ' ') < 0 || ncplane_putsimple(on, ' ') < 0){
    return NULL;
  }
  if(ncplane_cursor_move_yx(on, *rows - 1, *cols - 2)){
    return NULL;
  }
  if(ncplane_putsimple(on, ' ') < 0 || ncplane_putsimple(on, ' ') < 0){
    return NULL;
  }
  if(ncplane_set_fg_rgb(on, 0, 0, 0)){
    return NULL;
  }
  if(ncplane_set_bg_rgb(on, 0, 180, 180)){
    return NULL;
  }
  if(ncplane_set_bg_alpha(on, CELL_ALPHA_OPAQUE)){ // FIXME use intermediate
    return NULL;
  }
  if(ncplane_putstr_aligned(on, ++ybase, str0, NCALIGN_CENTER) < 0){
    return NULL;
  }
  if(ncplane_putstr_aligned(on, ++ybase, str1, NCALIGN_CENTER) < 0){
    return NULL;
  }
  if(ncplane_putstr_aligned(on, ++ybase, str2, NCALIGN_CENTER) < 0){
    return NULL;
  }
  if(notcurses_render(nc)){
    return NULL;
  }
  cell_release(on, &bgcell);
  *rows = ystart;
  *cols = xstart;
  return on;
}

int outro(struct notcurses* nc){
  struct ncplane* ncp;
  if((ncp = notcurses_stdplane(nc)) == NULL){
    return -1;
  }
  int rows, cols;
  ncplane_erase(ncp);
  ncplane_dim_yx(ncp, &rows, &cols);
  int averr = 0;
  struct ncvisual* ncv = ncplane_visual_open(ncp, "../tests/changes.jpg", &averr);
  if(ncv == NULL){
    return -1;
  }
  if(ncvisual_decode(ncv, &averr) == NULL){
    ncvisual_destroy(ncv);
    return -1;
  }
  if(ncvisual_render(ncv)){
    ncvisual_destroy(ncv);
    return -1;
  }
  int xstart = cols;
  int ystart = rows;
  struct ncplane* on = outro_message(nc, &ystart, &xstart);
  if(on){
    pthread_t tid;
    // will fade across 5s
    pthread_create(&tid, NULL, fadethread, ncp);
    struct timespec iterts;
    int targy = rows / 2 - (rows - ystart);
    ns_to_timespec(5000000000 / (ystart - targy), &iterts);
    int y;
    for(y = ystart - 1 ; y >= targy ; --y){
      nanosleep(&iterts, NULL);
      ncplane_move_yx(on, y, xstart);
      notcurses_render(nc);
    }
    ncplane_fadeout(on, &demodelay);
    pthread_join(tid, NULL);
    ncplane_destroy(on);
  }
  ncvisual_destroy(ncv);
  return on ? 0 : -1;
}

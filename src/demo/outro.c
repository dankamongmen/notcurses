#include <pthread.h>
#include "demo.h"

static int y;
static int targy;
static int xstart;
static struct ncplane* on;
static struct ncvisual* chncv;

static int
perframe(struct notcurses* nc, struct ncvisual* ncv __attribute__ ((unused))){
  static int three = 3; // move up one every three callbacks
  demo_render(nc);
  if(y < targy){
    return 0;
  }
  ncplane_move_yx(on, y, xstart);
  if(--three == 0){
    --y;
    three = 3;
  }
  return 0;
}

static void*
fadethread(void* vnc){
  struct notcurses* nc = vnc;
  struct ncplane* ncp = notcurses_stdplane(nc);
  struct timespec fade = { .tv_sec = 2, .tv_nsec = 0, };
  ncplane_fadeout(ncp, &fade, demo_fader);
  ncvisual_destroy(chncv);
  int averr;
  char* path = find_data("samoa.avi");
  struct ncvisual* ncv = ncplane_visual_open(ncp, path, &averr);
  free(path);
  if(ncv == NULL){
    return NULL;
  }
  if(ncvisual_decode(ncv, &averr) == NULL){
    ncvisual_destroy(ncv);
    return NULL;
  }
  int rows, cols;
  notcurses_term_dim_yx(nc, &rows, &cols);
  struct ncplane* apiap = notcurses_newplane(nc, 1, cols, rows - 1, 0, NULL);
  ncplane_set_fg_rgb(apiap, 0xc0, 0x40, 0x80);
  ncplane_set_bg_rgb(apiap, 0, 0, 0);
  ncplane_putstr_aligned(apiap, 0, NCALIGN_CENTER,
      "Apia 🡺 Atlanta. Samoa, tula'i ma sisi ia lau fu'a, lou pale lea!");
  ncvisual_stream(nc, ncv, &averr, perframe);
  ncvisual_destroy(ncv);
  ncplane_erase(ncp);
  fade.tv_sec = 2;
  fade.tv_nsec = 0;
  nanosleep(&fade, NULL);
  ncplane_destroy(apiap);
  return vnc;
}

static struct ncplane*
outro_message(struct notcurses* nc, int* rows, int* cols){
  const char str0[] = " ATL, baby! ATL! ";
  const char str1[] = " throw your hands in the air ";
  const char str2[] = " hack on! —dank❤ ";
  int xs = (*cols - (strlen(str1) + 4)) / 2;
  int ystart = *rows - 6;
  *cols = xs;
  struct ncplane* non = notcurses_newplane(nc, 5, strlen(str1) + 4, ystart, *cols, NULL);
  if(non == NULL){
    return NULL;
  }
  cell bgcell = CELL_SIMPLE_INITIALIZER(' ');
  channels_set_bg_rgb(&bgcell.channels, 0x58, 0x36, 0x58);
  if(ncplane_set_default(non, &bgcell) < 0){
    return NULL;
  }
  ncplane_dim_yx(non, rows, cols);
  int ybase = 0;
  // bevel the upper corners
  if(ncplane_set_bg_alpha(non, CELL_ALPHA_TRANSPARENT)){
    return NULL;
  }
  if(ncplane_cursor_move_yx(non, ybase, 0)){
    return NULL;
  }
  if(ncplane_putsimple(non, ' ') < 0 || ncplane_putsimple(non, ' ') < 0){
    return NULL;
  }
  if(ncplane_cursor_move_yx(non, ybase, *cols - 2)){
    return NULL;
  }
  if(ncplane_putsimple(non, ' ') < 0 || ncplane_putsimple(non, ' ') < 0){
    return NULL;
  }
  // ...and now the lower corners
  if(ncplane_cursor_move_yx(non, *rows - 1, 0)){
    return NULL;
  }
  if(ncplane_putsimple(non, ' ') < 0 || ncplane_putsimple(non, ' ') < 0){
    return NULL;
  }
  if(ncplane_cursor_move_yx(non, *rows - 1, *cols - 2)){
    return NULL;
  }
  if(ncplane_putsimple(non, ' ') < 0 || ncplane_putsimple(non, ' ') < 0){
    return NULL;
  }
  if(ncplane_set_fg_rgb(non, 0, 0, 0)){
    return NULL;
  }
  if(ncplane_set_bg_rgb(non, 0, 180, 180)){
    return NULL;
  }
  if(ncplane_set_bg_alpha(non, CELL_ALPHA_OPAQUE)){ // FIXME use intermediate
    return NULL;
  }
  ncplane_styles_on(non, CELL_STYLE_BOLD);
  if(ncplane_putstr_aligned(non, ++ybase, NCALIGN_CENTER, str0) < 0){
    return NULL;
  }
  ncplane_styles_off(non, CELL_STYLE_BOLD);
  if(ncplane_putstr_aligned(non, ++ybase, NCALIGN_CENTER, str1) < 0){
    return NULL;
  }
  ncplane_styles_on(non, CELL_STYLE_ITALIC);
  if(ncplane_putstr_aligned(non, ++ybase, NCALIGN_CENTER, str2) < 0){
    return NULL;
  }
  ncplane_styles_off(non, CELL_STYLE_ITALIC);
  if(demo_render(nc)){
    return NULL;
  }
  cell_release(non, &bgcell);
  *rows = ystart;
  *cols = xs;
  return non;
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
  char* path = find_data("changes.jpg");
  chncv = ncplane_visual_open(ncp, path, &averr);
  free(path);
  if(chncv == NULL){
    return -1;
  }
  if(ncvisual_decode(chncv, &averr) == NULL){
    ncvisual_destroy(chncv);
    return -1;
  }
  if(ncvisual_render(chncv, 0, 0, 0, 0)){
    ncvisual_destroy(chncv);
    return -1;
  }
  xstart = cols;
  int ystart = rows;
  on = outro_message(nc, &ystart, &xstart);
  y = ystart - 1;
  void* ret = NULL; // thread result
  if(on){
    ncplane_move_top(on);
    pthread_t tid;
    // will fade across 2s
    targy = 3;
    pthread_create(&tid, NULL, fadethread, nc);
    pthread_join(tid, &ret);
    ncplane_fadeout(on, &demodelay, demo_fader);
    ncplane_destroy(on);
  }
  if(ret == NULL){
    return -1;
  }
  return on ? 0 : -1;
}

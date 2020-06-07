#include <pthread.h>
#include "demo.h"

static int y;
static int targy;
static int xstart;
static struct ncplane* on;
struct ncvisual_options vopts;

// called in the context of the ncvisual streamer of samoa.avi
static int
perframe(struct ncvisual* ncv __attribute__ ((unused)),
         struct ncvisual_options* ovopts,
         const struct timespec* abstime, void* vthree){
  int* three = vthree; // move up one every three callbacks
  DEMO_RENDER(ncplane_notcurses(ovopts->n));
  if(y < targy){
    return 0;
  }
  ncplane_move_yx(on, y, xstart);
  if(--*three == 0){
    --y;
    *three = 3;
  }
  clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, abstime, NULL);
  return 0;
}

static void*
videothread(void* vnc){
  struct notcurses* nc = vnc;
  nc_err_e err;
  char* path = find_data("samoa.avi");
  struct ncvisual* ncv = ncvisual_from_file(path, &err);
  free(path);
  if(ncv == NULL){
    return NULL;
  }
  int rows, cols;
  struct ncplane* ncp = notcurses_stddim_yx(nc, &rows, &cols);
  struct ncvisual_options ovopts = {
    .scaling = NCSCALE_STRETCH,
    .n = ncp,
  };
  int three = 3;
  if(ncvisual_render(nc, ncv, &ovopts) == NULL){
    return NULL;
  }
  struct timespec fade;
  timespec_mul(&demodelay, 2, &fade);
  demo_render(nc);
  // vopts carries through 'changes.jpg', sitting above the standard plane
  ncplane_fadeout(vopts.n, &fade, demo_fader, NULL);
  ncplane_destroy(vopts.n);
  struct ncplane* apiap = ncplane_new(nc, 1, cols, rows - 1, 0, NULL);
  ncplane_set_fg_rgb(apiap, 0xc0, 0x40, 0x80);
  ncplane_set_bg_rgb(apiap, 0, 0, 0);
  ncplane_putstr_aligned(apiap, 0, NCALIGN_CENTER,
      "Apia 🡺 Atlanta. Samoa, tula'i ma sisi ia lau fu'a, lou pale lea!");
  int canceled = ncvisual_stream(nc, ncv, &err, delaymultiplier, perframe, &ovopts, &three);
  ncvisual_destroy(ncv);
  ncplane_destroy(apiap);
  if(canceled == 1){
    return PTHREAD_CANCELED;
  }
  return vnc;
}

static struct ncplane*
outro_message(struct notcurses* nc, int* rows, int* cols){
  const char str0[] = " ATL, baby! ATL! ";
  const char str1[] = " throw your hands in the air ";
  const char str2[] = " hack on! —dank❤ ";
  int ystart = *rows - 6;
  struct ncplane* non = ncplane_aligned(notcurses_stdplane(nc), 5,
                                        strlen(str1) + 4, ystart,
                                        NCALIGN_CENTER, NULL);
  if(non == NULL){
    return NULL;
  }
  int xs;
  ncplane_yx(non, NULL, &xs);
  uint64_t channels = 0;
  channels_set_bg_rgb(&channels, 0x58, 0x36, 0x58);
  if(ncplane_set_base(non, " ", 0, channels) < 0){
    return NULL;
  }
  ncplane_dim_yx(non, rows, cols);
  int ybase = 0;
  // bevel the upper corners
  if(ncplane_set_bg_alpha(non, CELL_ALPHA_TRANSPARENT)){
    return NULL;
  }
  if(ncplane_putsimple_yx(non, ybase, 0, ' ') < 0 || ncplane_putsimple(non, ' ') < 0){
    return NULL;
  }
  if(ncplane_putsimple_yx(non, ybase, *cols - 2, ' ') < 0 || ncplane_putsimple(non, ' ') < 0){
    return NULL;
  }
  // ...and now the lower corners
  if(ncplane_putsimple_yx(non, *rows - 1, 0, ' ') < 0 || ncplane_putsimple(non, ' ') < 0){
    return NULL;
  }
  if(ncplane_putsimple_yx(non, *rows - 1, *cols - 2, ' ') < 0 || ncplane_putsimple(non, ' ') < 0){
    return NULL;
  }
  if(ncplane_set_fg_rgb(non, 0, 0, 0)){
    return NULL;
  }
  if(ncplane_set_bg_rgb(non, 0, 180, 180)){
    return NULL;
  }
  if(ncplane_set_bg_alpha(non, CELL_ALPHA_BLEND)){
    return NULL;
  }
  ncplane_styles_on(non, NCSTYLE_BOLD);
  if(ncplane_putstr_aligned(non, ++ybase, NCALIGN_CENTER, str0) < 0){
    return NULL;
  }
  ncplane_styles_off(non, NCSTYLE_BOLD);
  if(ncplane_putstr_aligned(non, ++ybase, NCALIGN_CENTER, str1) < 0){
    return NULL;
  }
  ncplane_styles_on(non, NCSTYLE_ITALIC);
  if(ncplane_putstr_aligned(non, ++ybase, NCALIGN_CENTER, str2) < 0){
    return NULL;
  }
  ncplane_styles_off(non, NCSTYLE_ITALIC);
  *rows = ystart;
  *cols = xs;
  return non;
}

int outro(struct notcurses* nc){
  if(!notcurses_canutf8(nc)){ // there's UTF8 in the outro message
    return 0;
  }
  int rows, cols;
  struct ncplane* ncp = notcurses_stddim_yx(nc, &rows, &cols);
  ncplane_erase(ncp);
  struct ncvisual* chncv = NULL;
  if(notcurses_canopen_images(nc)){
    nc_err_e err = 0;
    char* path = find_data("changes.jpg");
    chncv = ncvisual_from_file(path, &err);
    free(path);
    if(chncv == NULL){
      return -1;
    }
    vopts.scaling = NCSCALE_STRETCH;
    vopts.flags = NCVISUAL_OPTION_BLEND;
    if((vopts.n = ncvisual_render(nc, chncv, &vopts)) == NULL){
      ncvisual_destroy(chncv);
      return -1;
    }
  }
  xstart = cols;
  int ystart = rows;
  on = outro_message(nc, &ystart, &xstart);
  if(on == NULL){
    return -1;
  }
  y = ystart - 1;
  DEMO_RENDER(nc);
  ncplane_move_top(on);
  if(notcurses_canopen_videos(nc)){
    pthread_t tid;
    // will fade across 2 * demodelay
    targy = 3;
    pthread_create(&tid, NULL, videothread, nc);
    void* ret;
    pthread_join(tid, &ret);
    if(ret == PTHREAD_CANCELED){
      return 1;
    }
  }
  // fade out the closing message, which has reached the top (if we ran the
  // video) or is sitting at the bottom (if we didn't).
  ncplane_fadeout(on, &demodelay, demo_fader, NULL);
  ncplane_destroy(on);
  ncvisual_destroy(chncv);
  return on ? 0 : -1;
}

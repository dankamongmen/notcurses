#include <pthread.h>
#include "demo.h"

static int y;
static int targy;
static unsigned xstart;
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

// vsamoa is samoa fadectx
static int
changes_fadeout(struct notcurses* nc, struct ncplane* ncp,
                const struct timespec* abstime, void* vsamoa){
  static int iter = 0;
  struct ncfadectx* samoactx = vsamoa;
  struct ncplane* samplane = notcurses_stdplane(nc);
  if(++iter > ncfadectx_iterations(samoactx)){
    iter = ncfadectx_iterations(samoactx);
  }
  int r = ncplane_fadein_iteration(samplane, samoactx, iter, demo_fader, NULL);
  if(r){
    return r;
  }
  return demo_fader(nc, ncp, abstime, NULL);
}

static void*
videothread(void* vnc){
  struct notcurses* nc = vnc;
  char* path = find_data("samoa.avi");
  struct ncvisual* ncv = ncvisual_from_file(path);
  free(path);
  if(ncv == NULL){
    return NULL;
  }
  unsigned rows, cols;
  struct ncplane* ncp = notcurses_stddim_yx(nc, &rows, &cols);
  struct ncvisual_options ovopts = {
    .scaling = NCSCALE_STRETCH,
    .n = ncp,
    .y = 1,
  };
  int three = 3;
  if(ncvisual_blit(nc, ncv, &ovopts) == NULL){
    return NULL;
  }
  struct timespec fade;
  timespec_mul(&demodelay, 2, &fade);
  demo_render(nc);

  // vopts carries through 'changes.jpg', sitting above the standard plane. it
  // is faded out as we fade the samoa video (on standard plane ncp) in.
  struct ncfadectx* samoactx = ncfadectx_setup(ncp);
  if(samoactx == NULL){
    return NULL;
  }
  if(ncplane_fadeout(vopts.n, &fade, changes_fadeout, samoactx) < 0){
    ncfadectx_free(samoactx);
    ncplane_destroy(vopts.n);
    ncvisual_destroy(ncv);
    return PTHREAD_CANCELED;
  }
  ncfadectx_free(samoactx);
  ncplane_destroy(vopts.n);
  struct ncplane_options nopts = {
    .y = 1,
    .rows = 1,
    .cols = cols,
    .name = "apia",
  };
  struct ncplane* apiap = ncplane_create(ncp, &nopts);
  if(apiap == NULL){
    ncfadectx_free(samoactx);
    ncplane_destroy(vopts.n);
    ncvisual_destroy(ncv);
    return PTHREAD_CANCELED;
  }
  uint64_t trans_channel = 0;
  ncchannels_set_bg_alpha(&trans_channel, NCALPHA_TRANSPARENT);
  ncchannels_set_fg_alpha(&trans_channel, NCALPHA_TRANSPARENT);
  ncplane_set_base(apiap, "", 0, trans_channel);
  ncplane_set_fg_rgb8(apiap, 0xc0, 0x40, 0x80);
  ncplane_set_bg_rgb8(apiap, 0, 0, 0);
  ncplane_putstr_aligned(apiap, 0, NCALIGN_CENTER,
      notcurses_canutf8(nc) ?
       "Apia 🡺 Atlanta. Samoa, tula'i ma sisi ia lau fu'a, lou pale lea!" :
       "Apia -> Atlanta. Samoa, tula'i ma sisi ia lau fu'a, lou pale lea!");
  int canceled = ncvisual_stream(nc, ncv, delaymultiplier, perframe, &ovopts, &three);
  ncvisual_destroy(ncv);
  ncplane_destroy(apiap);
  if(canceled == 1){
    return PTHREAD_CANCELED;
  }
  return vnc;
}

static struct ncplane*
outro_message(struct notcurses* nc, unsigned* rows, unsigned* cols){
  const char str0[] = " ATL, baby! ATL! ";
  const char str1[] = " throw your hands in the air ";
  const char* str2 = notcurses_canutf8(nc) ? " hack on! —dank❤ " : " hack on! --dank ";
  int ystart = *rows - 6;
  ncplane_options nopts = {
    .rows = 5,
    .cols = strlen(str1) + 4,
    .y = ystart,
    .x = NCALIGN_CENTER,
    .flags = NCPLANE_OPTION_HORALIGNED,
    .name = "atl",
  };
  struct ncplane* non = ncplane_create(notcurses_stdplane(nc), &nopts);
  if(non == NULL){
    return NULL;
  }
  int xs;
  ncplane_yx(non, NULL, &xs);
  uint64_t channels = 0;
  ncchannels_set_bg_rgb8(&channels, 0x58, 0x36, 0x58);
  if(ncplane_set_base(non, " ", 0, channels) < 0){
    return NULL;
  }
  ncplane_dim_yx(non, rows, cols);
  int ybase = 0;
  // bevel the upper corners
  if(ncplane_set_bg_alpha(non, NCALPHA_TRANSPARENT)){
    return NULL;
  }
  if(ncplane_putchar_yx(non, ybase, 0, ' ') < 0 || ncplane_putchar(non, ' ') < 0){
    return NULL;
  }
  if(ncplane_putchar_yx(non, ybase, *cols - 2, ' ') < 0 || ncplane_putchar(non, ' ') < 0){
    return NULL;
  }
  // ...and now the lower corners
  if(ncplane_putchar_yx(non, *rows - 1, 0, ' ') < 0 || ncplane_putchar(non, ' ') < 0){
    return NULL;
  }
  if(ncplane_putchar_yx(non, *rows - 1, *cols - 2, ' ') < 0 || ncplane_putchar(non, ' ') < 0){
    return NULL;
  }
  if(ncplane_set_fg_rgb8(non, 0, 0, 0)){
    return NULL;
  }
  if(ncplane_set_bg_rgb8(non, 0, 180, 180)){
    return NULL;
  }
  if(ncplane_set_bg_alpha(non, NCALPHA_BLEND)){
    return NULL;
  }
  ncplane_on_styles(non, NCSTYLE_BOLD);
  if(ncplane_putstr_aligned(non, ++ybase, NCALIGN_CENTER, str0) < 0){
    return NULL;
  }
  ncplane_off_styles(non, NCSTYLE_BOLD);
  if(ncplane_putstr_aligned(non, ++ybase, NCALIGN_CENTER, str1) < 0){
    return NULL;
  }
  ncplane_on_styles(non, NCSTYLE_ITALIC);
  if(ncplane_putstr_aligned(non, ++ybase, NCALIGN_CENTER, str2) < 0){
    return NULL;
  }
  ncplane_off_styles(non, NCSTYLE_ITALIC);
  *rows = ystart;
  *cols = xs;
  return non;
}

int outro_demo(struct notcurses* nc, uint64_t startns){
  (void)startns;
  unsigned rows, cols;
  struct ncplane* ncp = notcurses_stddim_yx(nc, &rows, &cols);
  ncplane_erase(ncp);
  struct ncvisual* chncv = NULL;
  memset(&vopts, 0, sizeof(vopts));
  if(notcurses_canopen_images(nc)){
    char* path = find_data("changes.jpg");
    chncv = ncvisual_from_file(path);
    free(path);
    if(chncv == NULL){
      return -1;
    }
    vopts.scaling = NCSCALE_STRETCH;
    vopts.flags = NCVISUAL_OPTION_BLEND | NCVISUAL_OPTION_CHILDPLANE;
    vopts.n = notcurses_stdplane(nc);
    if((vopts.n = ncvisual_blit(nc, chncv, &vopts)) == NULL){
      ncvisual_destroy(chncv);
      return -1;
    }
    ncplane_set_name(vopts.n, "bnnr");
  }
  xstart = cols;
  unsigned ystart = rows;
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

#include "demo.h"

static struct marshal {
  struct ncvisual_options pipopts;
} marsh;

#define PIPCOLUMNS 18

static int
pip_resize_cb(struct ncplane* n){
  int absx = ncplane_abs_x(n);
  int width = ncplane_dim_x(n);
  int pwidth = ncplane_dim_x(ncplane_parent(n));
  int pabsx = ncplane_abs_x(ncplane_parent(n));
  if(absx + width == pabsx + pwidth){
    return 0;
  }
  return ncplane_move_yx(n, 1, pabsx + pwidth - width);
}

// pip is non-NULL iff we can do pixel rendering
static inline int
streamer(struct ncvisual* ncv, struct ncvisual_options* vopts,
         const struct timespec* tspec, void* vpip){
  if(vpip){
    if(!marsh.pipopts.n){
      struct ncplane_options nopts = {
        .y = 1,
        .x = NCALIGN_RIGHT,
        .rows = 12,
        .cols = PIPCOLUMNS,
        .flags = NCPLANE_OPTION_HORALIGNED,
        .name = "pip",
        .resizecb = pip_resize_cb,
      };
      marsh.pipopts.n = ncplane_create(vopts->n, &nopts);
      if(marsh.pipopts.n == NULL){
        return -1;
      }
      marsh.pipopts.blitter = NCBLIT_PIXEL;
      marsh.pipopts.scaling = NCSCALE_STRETCH;
    }
    if(marsh.pipopts.n){
      ncvisual_blit(ncplane_notcurses(marsh.pipopts.n), ncv, &marsh.pipopts);
      ncplane_move_above(marsh.pipopts.n, vopts->n);
    }
  }
  return demo_simple_streamer(ncv, vopts, tspec, NULL);
}

static int
view_video_demo(struct notcurses* nc){
  struct ncplane* ncp = notcurses_stdplane(nc);
  struct ncvisual* ncv;
  char* fm6 = find_data("fm6.mov");
  ncv = ncvisual_from_file(fm6);
  if(!ncv){
    free(fm6);
    return -1;
  }
  free(fm6);
  struct ncvisual_options vopts = {
    .scaling = NCSCALE_STRETCH,
    .n = ncp,
    .y = 1,
  };
  void* pip = NULL;
  if(notcurses_check_pixel_support(nc) > 0){
    pip = nc;
  }
  int ret = ncvisual_stream(nc, ncv, 0.5 * delaymultiplier,
                            streamer, &vopts, pip);
  ncvisual_destroy(ncv);
  ret |= ncplane_destroy(marsh.pipopts.n);
  return ret;
}

static struct ncplane*
legend(struct ncplane* stdn, int dimy, int dimx){
  struct ncplane_options nopts = {
    .y = dimy / 8 - 1,
    .x = (dimx - 25) / 2,
    .rows = 4,
    .cols = 25,
    .name = "lgd",
  };
  struct ncplane* n = ncplane_create(stdn, &nopts);
  ncplane_set_bg_alpha(n, NCALPHA_TRANSPARENT);
  uint64_t channels = 0;
  ncchannels_set_bg_alpha(&channels, NCALPHA_TRANSPARENT);
  ncplane_set_base(n, " ", 0, channels);
  ncplane_set_styles(n, NCSTYLE_BOLD);
  ncplane_set_fg_rgb8(n, 0xff, 0xff, 0xff);
  ncplane_set_fg_alpha(n, NCALPHA_HIGHCONTRAST);
  if(ncplane_putstr_aligned(n, 0, NCALIGN_CENTER, "target launch") <= 0){
    ncplane_destroy(n);
    return NULL;
  }
  ncplane_set_fg_rgb8(n, 0, 0, 0);
  if(ncplane_putstr_aligned(n, 1, NCALIGN_CENTER, "2003-12-11 FM-6") <= 0){
    ncplane_destroy(n);
    return NULL;
  }
  if(ncplane_putstr_aligned(n, 2, NCALIGN_CENTER, "RIM-161 SM-3 v. Aries TTV") <= 0){
    ncplane_destroy(n);
    return NULL;
  }
  ncplane_set_fg_rgb8(n, 0x80, 0xc0, 0x80);
  if(ncplane_putstr_aligned(n, 3, NCALIGN_CENTER, "exo-atmospheric intercept") <= 0){
    ncplane_destroy(n);
    return NULL;
  }
  ncplane_off_styles(n, NCSTYLE_BOLD);
  return n;
}

// first, images scaled to the rendering area's size. we first put up the
// DSSCAW logo (opaquely), then the purple drinking menace (opaquely), then
// the DSSAW logo atop it (transparently). the purple fellow has no
// transparency, so he's always opaque.
static int
view_images(struct notcurses* nc, struct ncplane* nstd, int dimy, int dimx){
  ncplane_erase(nstd);
  // standard plane gets PurpleDrank (which will cover the plane), but first
  // serves as a blocker behind dsplane, which gets the DSSCAW logo.
  struct ncplane_options nopts = {
    .rows = dimy,
    .cols = dimx,
    .name = "cblt",
  };
  struct ncplane* dsplane = ncplane_create(nstd, &nopts);
  if(dsplane == NULL){
    return -1;
  }
  char* pic = find_data("dsscaw-purp.png");
  struct ncvisual* ncv2 = ncvisual_from_file(pic);
  if(ncv2 == NULL){
    free(pic);
    ncplane_destroy(dsplane);
    return -1;
  }
  free(pic);
  struct ncvisual_options vopts = {
    .n = dsplane,
    .scaling = NCSCALE_STRETCH,
    .y = 1,
  };
  if(ncvisual_blit(nc, ncv2, &vopts) == NULL){
    ncvisual_destroy(ncv2);
    ncplane_destroy(dsplane);
    return -1;
  }
  uint64_t channels = 0;
  ncchannels_set_fg_alpha(&channels, NCALPHA_TRANSPARENT);
  ncchannels_set_bg_alpha(&channels, NCALPHA_TRANSPARENT);
  ncplane_set_base(dsplane, "", 0, channels);
  ncvisual_destroy(ncv2);
  demo_render(nc);
  demo_nanosleep(nc, &demodelay);
  // now we open PurpleDrank on the standard plane, and hide DSSAW
  ncplane_move_bottom(dsplane);
  pic = find_data("PurpleDrank.jpg");
  struct ncvisual* ncv = ncvisual_from_file(pic);
  if(ncv == NULL){
    ncplane_destroy(dsplane);
    free(pic);
    return -1;
  }
  free(pic);
  vopts.n = notcurses_stdplane(nc);
  if(ncvisual_blit(nc, ncv, &vopts) == NULL){
    ncvisual_destroy(ncv);
    ncplane_destroy(dsplane);
    return -1;
  }
  ncvisual_destroy(ncv);
  demo_render(nc);
  demo_nanosleep(nc, &demodelay);
  // bring DSSCAW back up to the top
  ncplane_move_top(dsplane);
  demo_render(nc);
  demo_nanosleep(nc, &demodelay);
  ncplane_destroy(dsplane);
  return 0;
}

int view_demo(struct notcurses* nc, uint64_t startns){
  (void)startns;
  if(!notcurses_canopen_images(nc)){
    return 0;
  }
  memset(&marsh, 0, sizeof(marsh));
  unsigned dimy, dimx;
  struct ncplane* nstd = notcurses_stddim_yx(nc, &dimy, &dimx);
  int ret = view_images(nc, nstd, dimy, dimx);
  if(ret){
    return ret;
  }
  struct ncplane* ncpl = legend(nstd, dimy, dimx);
  if(ncpl == NULL){
    return -1;
  }
  if(notcurses_canopen_videos(nc)){
    ret |= view_video_demo(nc);
  }
  ncplane_destroy(ncpl);
  return ret;
}

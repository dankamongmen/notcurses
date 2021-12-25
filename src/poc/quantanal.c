#include <inttypes.h>
#include <notcurses/notcurses.h>
#include <compat/compat.h>

static char*
colorbuf(void){
  return calloc(1u << 24u, 1);
}

static uint32_t
count_colors(uint32_t pixel, char* buf, uint32_t colors){
  if(buf[pixel & 0xffffffu] == 0){
    buf[pixel & 0xffffffu] = 1;
    return colors + 1;
  }
  return colors;
}

static void
compare(const struct ncvisual* n1, const struct ncvisual* n2,
        const ncvgeom* geom, struct notcurses* nc){
  uint32_t co0 = 0, co1 = 0;
  char* cbuf0 = colorbuf();
  char* cbuf1 = colorbuf();
  struct ncplane* stdn = notcurses_stdplane(nc);
  uint64_t rdelta = 0;
  uint64_t gdelta = 0;
  uint64_t bdelta = 0;
  unsigned ly = geom->rpixy;
  unsigned lx = geom->rpixx;
  ncplane_set_fg_rgb(stdn, 0x03c04a);
  for(unsigned y = 0 ; y < ly ; ++y){
    for(unsigned x = 0 ; x < lx ; ++x){
      uint32_t p0, p1;
      if(ncvisual_at_yx(n1, y, x, &p0) ||
         ncvisual_at_yx(n2, y, x, &p1)){
        fprintf(stderr, "error getting pixel at %u/%u\n", y, x);
        return;
      }
      co0 = count_colors(p0, cbuf0, co0);
      co1 = count_colors(p1, cbuf1, co1);
      // three component differences for current pixel
      int rd, gd, bd;
      rd = ncpixel_r(p0) - ncpixel_r(p1);
      gd = ncpixel_g(p0) - ncpixel_g(p1);
      bd = ncpixel_b(p0) - ncpixel_b(p1);
      if(rd < 0){
        rd = -rd;
      }
      if(gd < 0){
        gd = -gd;
      }
      if(bd < 0){
        bd = -bd;
      }
      rdelta += rd;
      gdelta += gd;
      bdelta += bd;
//fprintf(stderr, "0x%08x 0x%08x\n", p0, p1);
    }
    ncplane_printf_yx(stdn, -1, 1, "%08u pixels analyzed", (y + 1) * lx);
    ncplane_printf(stdn, " Δr %"PRIu64" Δg %"PRIu64" Δb %"PRIu64,
                  rdelta, gdelta, bdelta);
    notcurses_render(nc);
  }
  ncplane_putchar(stdn, '\n');
  ncplane_printf(stdn, " Colors: %"PRIu32" vs %"PRIu32"\n", co0, co1);
  double p = lx * ly;
  ncplane_printf(stdn, " %.0fpx Δr %"PRIu64" (%.03g) Δg %"PRIu64" (%.03g) Δb %"PRIu64 " (%.03g)\n",
                 p, rdelta, rdelta / p, gdelta, gdelta / p, bdelta, bdelta / p);
  ncplane_printf(stdn, " avg diff per pixel: %.03g\n", (rdelta + gdelta + bdelta) / p);
  notcurses_render(nc);
  free(cbuf0);
  free(cbuf1);
}

int main(int argc, char** argv){
  if(argc < 2){
    fprintf(stderr, "usage: %s images..." NL, *argv);
    return EXIT_FAILURE;
  }
  struct notcurses_options opts = {0};
  opts.flags = NCOPTION_CLI_MODE
               | NCOPTION_DRAIN_INPUT
               | NCOPTION_SUPPRESS_BANNERS;
  struct notcurses* nc = notcurses_init(&opts, NULL);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  ncpixelimpl_e blit = notcurses_check_pixel_support(nc);
  if(blit != NCPIXEL_SIXEL){
    notcurses_stop(nc);
    fprintf(stderr, "needed pixel blit type %d (sixel), got %d\n",
                    NCPIXEL_SIXEL, blit);
    return EXIT_FAILURE;
  }
  struct ncplane* stdn = notcurses_stdplane(nc);
  while(*++argv){
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    ncplane_set_fg_rgb(stdn, 0xf9d71c);
    ncplane_printf(stdn, "analyzing %s...\n", *argv);
    notcurses_render(nc);
    struct ncvisual* ncv = ncvisual_from_file(*argv);
    if(ncv == NULL){
      notcurses_stop(nc);
      fprintf(stderr, "error opening %s" NL, *argv);
      return EXIT_FAILURE;
    }
    struct ncplane* ncp = ncplane_dup(stdn, NULL);
    if(ncp == NULL){
      notcurses_stop(nc);
      return EXIT_FAILURE;
    }
    struct ncvisual_options vopts = {0};
    vopts.n = ncp;
    vopts.blitter = NCBLIT_PIXEL;
    vopts.flags = NCVISUAL_OPTION_NODEGRADE | NCVISUAL_OPTION_NOINTERPOLATE;
    struct ncvgeom geom;
    if(ncvisual_geom(nc, ncv, &vopts, &geom)){
      ncplane_set_fg_rgb(stdn, 0xd16002);
      ncplane_printf(stdn, " Image too large, scaling to display\n");
      notcurses_render(nc);
      vopts.scaling = NCSCALE_STRETCH;
      if(ncvisual_geom(nc, ncv, &vopts, &geom)){
        notcurses_stop(nc);
        fprintf(stderr, "error geometrizing %s" NL, *argv);
        return EXIT_FAILURE;
      }
    }
    ncplane_set_fg_rgb(stdn, 0x5dbb63);
    unsigned rgbabytes = 4 * geom.rpixy * geom.rpixx;
    ncplane_printf(stdn, " source pixels: %ux%u rendered: %ux%u %uB\n",
                   geom.pixy, geom.pixx, geom.rpixy, geom.rpixx, rgbabytes);
    notcurses_render(nc);
    if(ncvisual_blit(nc, ncv, &vopts) == NULL){
      notcurses_stop(nc);
      fprintf(stderr, "error rendering %s" NL, *argv);
      return EXIT_FAILURE;
    }
    char* s;
    if((s = ncplane_at_yx(ncp, 0, 0, NULL, NULL)) == NULL){
      notcurses_stop(nc);
      fprintf(stderr, "error retrieving sixel for %s" NL, *argv);
      return EXIT_FAILURE;
    }
    ncplane_set_fg_rgb(stdn, 0x74b72e);
    size_t slen = strlen(s);
    ncplane_printf(stdn, " control sequence: %" PRIuPTR " byte%s (%.4g%%)\n",
                   slen, slen == 1 ? "" : "s", (double)slen * 100 / rgbabytes);
    ncplane_reparent(ncp, ncp);
    notcurses_render(nc);
    unsigned leny = geom.rpixy;
    unsigned lenx = geom.rpixx;
    struct ncvisual* quantncv = ncvisual_from_sixel(s, leny, lenx);
    if(quantncv == NULL){
      notcurses_stop(nc);
      fprintf(stderr, "error loading %" PRIuPTR "B sixel" NL, slen);
      free(s);
      return EXIT_FAILURE;
    }
    free(s);
    compare(ncv, quantncv, &geom, nc);
    ncvisual_destroy(quantncv);
    ncvisual_destroy(ncv);
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);
    uint64_t t0 = timespec_to_ns(&start);
    uint64_t t1 = timespec_to_ns(&end);
    ncplane_set_fg_rgb(stdn, 0x03ac13);
    ncplane_printf(stdn, "done with %s in %.3fms.\n", *argv, (t1 - t0) / 1000000.0);
    ncplane_destroy(ncp);
    notcurses_render(nc);
  }
  return notcurses_stop(nc) ? EXIT_FAILURE : EXIT_SUCCESS;
}

#include <inttypes.h>
#include <notcurses/notcurses.h>
#include <compat/compat.h>

int main(int argc, char** argv){
  if(argc < 2){
    fprintf(stderr, "usage: %s images..." NL, *argv);
    return EXIT_FAILURE;
  }
  struct notcurses_options opts = {0};
  opts.flags = NCOPTION_CLI_MODE | NCOPTION_DRAIN_INPUT;
  struct notcurses* nc = notcurses_init(&opts, NULL);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  // FIXME check pixel blitting type
  struct ncplane* stdn = notcurses_stdplane(nc);
  while(*++argv){
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
    vopts.flags = NCVISUAL_OPTION_NODEGRADE;
    struct ncvgeom geom;
    if(ncvisual_geom(nc, ncv, &vopts, &geom)){
      notcurses_stop(nc);
      fprintf(stderr, "error geometrizing %s" NL, *argv);
      return EXIT_FAILURE;
    }
    ncplane_set_fg_rgb(stdn, 0x5dbb63);
    ncplane_printf(stdn, " source pixels: %ux%u\n", geom.pixy, geom.pixx);
    ncplane_set_fg_rgb(stdn, 0x03c04a);
    unsigned rgbabytes = 4 * geom.rpixy * geom.rpixx;
    ncplane_printf(stdn, " rendered: %ux%u %uB\n", geom.rpixy, geom.rpixx, rgbabytes);
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
    // FIXME compare ncv and quantncv
    ncvisual_destroy(quantncv);
    ncvisual_destroy(ncv);
    ncplane_set_fg_rgb(stdn, 0x03ac13);
    ncplane_printf(stdn, "done with %s.\n", *argv);
    ncplane_destroy(ncp);
    notcurses_render(nc);
  }
  return notcurses_stop(nc) ? EXIT_FAILURE : EXIT_SUCCESS;
}

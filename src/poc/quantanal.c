#include <notcurses/notcurses.h>

int main(int argc, char** argv){
  if(argc < 2){
    fprintf(stderr, "usage: %s images...\n", *argv);
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
      fprintf(stderr, "error opening %s\n", *argv);
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
    if(ncvisual_blit(nc, ncv, &vopts) == NULL){
      notcurses_stop(nc);
      fprintf(stderr, "error rendering %s\n", *argv);
      return EXIT_FAILURE;
    }
    // FIXME acquire sixel as s
    char* s = strdup("");
    unsigned leny = 0, lenx = 0; // FIXME
    struct ncvisual* quantncv = ncvisual_from_sixel(s, leny, lenx);
    if(quantncv == NULL){
      notcurses_stop(nc);
      fprintf(stderr, "error loading %zuB sixel\n", strlen(s));
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

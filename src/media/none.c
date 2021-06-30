#include "builddef.h"
#ifndef USE_OIIO
#ifndef USE_FFMPEG
#include "internal.h"
#include "visual-details.h"

static void
printbanner(const notcurses* nc){
  term_fg_palindex(nc, stderr, nc->tcache.caps.colors <= 88 ? 1 : 0xcb);
  fprintf(stderr, " Warning! Notcurses was built without multimedia support.\n");
}

const ncvisual_implementation local_visual_implementation = {
  .visual_printbanner = printbanner,
};

#endif
#endif

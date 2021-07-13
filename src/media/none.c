#include "builddef.h"
#ifndef USE_OIIO
#ifndef USE_FFMPEG
#include "internal.h"

static void
printbanner(const notcurses* nc){
  fprintf(nc->ttyfp, "built without multimedia support\n");
}

const ncvisual_implementation local_visual_implementation = {
  .visual_printbanner = printbanner,
};

#endif
#endif

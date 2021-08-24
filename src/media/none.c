#include "builddef.h"
#ifndef USE_OIIO
#ifndef USE_FFMPEG
#include "lib/internal.h"

static void
printbanner(fbuf* f){
  fbuf_puts(f, "built without multimedia support\n");
}

const ncvisual_implementation local_visual_implementation = {
  .visual_printbanner = printbanner,
};

#endif
#endif

#include <errno.h>
#include <wchar.h>
#include <wctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "demo.h"

// show unicode blocks. a block is always a multiple of 16 codepoints.
#define ITERATIONS 30 // show this many pages
#define BLOCKSIZE 512 // show this many per page
#define CHUNKSIZE 32  // show this many per line

int unicodeblocks_demo(struct notcurses* nc){
  struct ncplane* n = notcurses_stdplane(nc);
  int maxx, maxy;
  notcurses_term_dimyx(nc, &maxy, &maxx);
  int i;
  // show 256 at a time. start with the known-working ascii/latin-1 blocks.
  uint32_t blockstart = 0;
  for(i = 0 ; i < ITERATIONS ; ++i){
    ncplane_erase(n);
    int chunk;
    if(ncplane_cursor_move_yx(n, 2, 2)){
      return -1;
    }
    ncplane_fg_rgb8(n, 0xad, 0xd8, 0xe6);
    ncplane_bg_rgb8(n, 0, 0, 0);
    if(ncplane_printf(n, "Unicode points %04x–%04x\n", blockstart, blockstart + BLOCKSIZE) <= 0){
      return -1;
    }
    for(chunk = 0 ; chunk < BLOCKSIZE / CHUNKSIZE ; ++chunk){
      if(ncplane_cursor_move_yx(n, 4 + chunk, 1)){
        return -1;
      }
      int z;
      cell c = CELL_TRIVIAL_INITIALIZER;
      // 16 to a line
      for(z = 0 ; z < CHUNKSIZE ; ++z){
        mbstate_t ps;
        memset(&ps, 0, sizeof(ps));
        wchar_t w = blockstart + chunk * CHUNKSIZE + z;
        char utf8arr[MB_CUR_MAX + 1];
        if(iswprint(w)){
          int bwc = wcrtomb(utf8arr, w, &ps);
          if(bwc < 0){
            fprintf(stderr, "Couldn't convert %u (%x) (%lc) (%s)\n",
                    blockstart + chunk * CHUNKSIZE + z,
                    blockstart + chunk * CHUNKSIZE + z, w, strerror(errno));
            return -1;
          }
          utf8arr[bwc] = '\0';
        }else{ // don't dump non-printing codepoints
          utf8arr[0] = ' ';
          utf8arr[1] = '\0';
        }
        if(cell_load(n, &c, utf8arr) < 0){ // FIXME check full len was eaten?
          return -1;;
        }
        /*ncplane_fg_rgb8(n, 0xad + z, 0xd8, 0xe6);
        ncplane_bg_rgb8(n, 0x20, 0x20, 0x20);*/
        cell_set_fg(&c, 0xad + z * 2, 0xd8, 0xe6 - z * 2);
        cell_set_bg(&c, 8 * chunk, 8 * chunk, 8 * chunk);
        if(ncplane_putc(n, &c) < 0){
          return -1;
        }
      }
      cell_release(n, &c);
      if(notcurses_render(nc)){
        return -1;
      }
    }
    usleep(10000);
    // for a 32-bit wchar_t, we would want up through 24 bits of block ID. but
    // really, the vast majority of space is unused. cap at 0x3000.
    blockstart += BLOCKSIZE;
  }
  return 0;
}

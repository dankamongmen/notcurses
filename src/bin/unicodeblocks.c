#include <errno.h>
#include <wchar.h>
#include <wctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "demo.h"

// show unicode blocks. a block is always a multiple of 16 codepoints.
#define BLOCKSIZE 512 // show this many per page
#define CHUNKSIZE 32  // show this many per line

int unicodeblocks_demo(struct notcurses* nc){
  struct ncplane* n = notcurses_stdplane(nc);
  int maxx, maxy;
  notcurses_term_dimyx(nc, &maxy, &maxx);
  // some blocks are good for the printing, some less so. some are only
  // marginally covered by mainstream fonts, some not at all. we explicitly
  // list the ones we want.
  const struct {
    const char* name;
    uint32_t start;
  } blocks[] = {
    { .name = "Basic Latin, Latin 1 Supplement, Latin Extended", .start = 0, },
    { .name = "IPA Extensions, Spacing Modifiers, Greek and Coptic", .start = 0x200, },
    { .name = "Cyrillic, Cyrillic Supplement, Armenian, Hebrew", .start = 0x400, },
    { .name = "Arabic, Syriac, Arabic Supplement", .start = 0x600, },
    { .name = "Samaritan, Mandaic, Devanagari, Bengali", .start = 0x800, },
    { .name = "Gurmukhi, Gujarati, Oriya, Tamil", .start = 0xa00, },
    { .name = "Telugu, Kannada, Malayalam, Sinhala", .start = 0xc00, },
    { .name = "Thai, Lao, Tibetan", .start = 0xe00, },
    { .name = "Myanmar, Georgian, Hangul Jamo", .start = 0x1000, },
    { .name = "Ethiopic, Ethiopic Supplement, Cherokee", .start = 0x1200, },
    { .name = "Canadian", .start = 0x1400, },
    { .name = "Runic, Tagalog, Hanunoo, Buhid, Tagbanwa, Khmer", .start = 0x1600, },
    { .name = "Mongolian, Canadian Extended, Limbu, Tai Le", .start = 0x1800, },
    { .name = "Buginese, Tai Tham, Balinese, Sundanese, Batak", .start = 0x1a00, },
    { .name = "Lepcha, Ol Chiki, Vedic Extensions, Phonetic Extensions", .start = 0x1c00, },
    { .name = "Latin Extended Additional, Greek Extended", .start = 0x1e00, },
    { .name = "General Punctuation, Letterlike Symbols, Arrows", .start = 0x2000, },
    { .name = "Mathematical Operators, Miscellaneous Technical", .start = 0x2200, },
    { .name = "Control Pictures, Box Drawing, Block Elements", .start = 0x2400, },
    { .name = "Miscellaneous Symbols, Dingbats", .start = 0x2600, },
    { .name = "Braille Patterns, Supplemental Arrows", .start = 0x2800, },
    { .name = "Supplemental Mathematical Operators", .start = 0x2a00, },
    { .name = "Glagolitic, Georgian Supplement, Tifinagh", .start = 0x2c00, },
    { .name = "Supplemental Punctuation, CJK Radicals", .start = 0x2e00, },
  };
  size_t sindex;
  ncplane_erase(n);
  for(sindex = 0 ; sindex < sizeof(blocks) / sizeof(*blocks) ; ++sindex){
    uint32_t blockstart = blocks[sindex].start;
    const char* description = blocks[sindex].name;
    int chunk;
    if(ncplane_cursor_move_yx(n, 2, 2)){
      return -1;
    }
    ncplane_fg_rgb8(n, 0xad, 0xd8, 0xe6);
    ncplane_bg_rgb8(n, 0, 0, 0);
    if(ncplane_printf(n, "Unicode points %04x–%04x\n", blockstart, blockstart + BLOCKSIZE) <= 0){
      return -1;
    }
    if(ncplane_cursor_move_yx(n, 3, 3)){
      return -1;
    }
    if(ncplane_printf(n, description) <= 0){
      return -1;
    }
    for(chunk = 0 ; chunk < BLOCKSIZE / CHUNKSIZE ; ++chunk){
      if(ncplane_cursor_move_yx(n, 4 + chunk, 2)){
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
        if(wcwidth(w) == 1 && iswprint(w)){
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

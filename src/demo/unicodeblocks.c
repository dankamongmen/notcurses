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

static int
fade_block(struct ncplane* nn, const struct timespec* subdelay){
  int ret = ncplane_fadein(nn, subdelay);
  ncplane_destroy(nn);
  return ret;
}

static int
draw_block(struct ncplane* nn, uint32_t blockstart){
  cell ul = CELL_TRIVIAL_INITIALIZER, ur = CELL_TRIVIAL_INITIALIZER;
  cell ll = CELL_TRIVIAL_INITIALIZER, lr = CELL_TRIVIAL_INITIALIZER;
  cell hl = CELL_TRIVIAL_INITIALIZER, vl = CELL_TRIVIAL_INITIALIZER;
  cells_rounded_box(nn, 0, 0, &ul, &ur, &ll, &lr, &hl, &vl);
  cell_set_bg_alpha(&ul, 3);
  cell_set_bg_alpha(&ur, 3);
  cell_set_bg_alpha(&ll, 3);
  cell_set_bg_alpha(&lr, 3);
  cell_set_bg_rgb(&hl, 0, 0, 0);
  cell_set_bg_rgb(&vl, 0, 0, 0);
  if(ncplane_box_sized(nn, &ul, &ur, &ll, &lr, &hl, &vl,
                  BLOCKSIZE / CHUNKSIZE + 2,
                  (CHUNKSIZE * 2) + 2, 0)){
    return -1;
  }
  cell_release(nn, &ul); cell_release(nn, &ur); cell_release(nn, &hl);
  cell_release(nn, &ll); cell_release(nn, &lr); cell_release(nn, &vl);
  int chunk;
  for(chunk = 0 ; chunk < BLOCKSIZE / CHUNKSIZE ; ++chunk){
    if(ncplane_cursor_move_yx(nn, chunk + 1, 1)){
      return -1;
    }
    int z;
    cell c = CELL_TRIVIAL_INITIALIZER;
    // 16 to a line
    for(z = 0 ; z < CHUNKSIZE ; ++z){
      wchar_t w[2] = { blockstart + chunk * CHUNKSIZE + z, L'\u200e' };
      char utf8arr[MB_CUR_MAX * 2 + 1];
      if(wcswidth(w, 2) >= 1 && iswprint(w[0])){
        mbstate_t ps;
        memset(&ps, 0, sizeof(ps));
        const wchar_t *wptr = w;
        int bwc = wcsrtombs(utf8arr, &wptr, sizeof(utf8arr), &ps);
        if(bwc < 0){
          fprintf(stderr, "Couldn't convert %u (%x) (%lc) (%s)\n",
                  blockstart + chunk * CHUNKSIZE + z,
                  blockstart + chunk * CHUNKSIZE + z, w[0], strerror(errno));
          return -1;
        }
        utf8arr[bwc] = '\0';
      }else{ // don't dump non-printing codepoints
        utf8arr[0] = ' ';
        utf8arr[1] = 0xe2;
        utf8arr[2] = 0x80;
        utf8arr[3] = 0x8e;
        utf8arr[4] = '\0';
      }
      if(cell_load(nn, &c, utf8arr) < 0){ // FIXME check full len was eaten?
        return -1;;
      }
      cell_set_fg_rgb(&c, 0xad + z * 2, 0xd8, 0xe6 - z * 2);
      cell_set_bg_rgb(&c, 8 * chunk, 8 * chunk + z, 8 * chunk);
      if(ncplane_putc(nn, &c) < 0){
        return -1;
      }
      if(wcwidth(w[0]) < 2 || !iswprint(w[0])){
        if(cell_load(nn, &c, " ") < 0){
          return -1;
        }
        if(ncplane_putc(nn, &c) < 0){
          return -1;
        }
      }
    }
    cell_release(nn, &c);
  }
  return 0;
}

int unicodeblocks_demo(struct notcurses* nc){
  struct ncplane* n = notcurses_stdplane(nc);
  int maxx, maxy;
  notcurses_term_dim_yx(nc, &maxy, &maxx);
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
    { .name = "CJK Symbols and Punctuation", .start = 0x3000, },
    { .name = "Enclosed CJK Letters and Months", .start = 0x3200, },
    { .name = "CJK Unified Ideographs Extension A", .start = 0x3400, },
    { .name = "CJK Unified Ideographs Extension A (cont.)", .start = 0x3600, },
    { .name = "CJK Unified Ideographs Extension A (cont.)", .start = 0x3800, },
    { .name = "CJK Unified Ideographs Extension A (cont.)", .start = 0x3a00, },
    { .name = "CJK Unified Ideographs Extension A (cont.)", .start = 0x3c00, },
    { .name = "CJK Unified Ideographs Extension A (cont.)", .start = 0x3e00, },
    { .name = "CJK Unified Ideographs Extension A (cont.)", .start = 0x4000, },
    { .name = "CJK Unified Ideographs Extension A (cont.)", .start = 0x4200, },
    { .name = "CJK Unified Ideographs Extension A (cont.)", .start = 0x4400, },
    { .name = "CJK Unified Ideographs Extension A (cont.)", .start = 0x4600, },
    { .name = "CJK Unified Ideographs Extension A (cont.)", .start = 0x4800, },
    { .name = "CJK Unified Ideographs Extension A (cont.)", .start = 0x4a00, },
    { .name = "CJK Unified Ideographs Extension A, Yijang Hexagram", .start = 0x4c00, },
    { .name = "CJK Unified Ideographs", .start = 0x4e00, },
    { .name = "Yi Syllables", .start = 0xa000, },
    { .name = "Yi Syllables", .start = 0xa200, },
    { .name = "Yi Syllables, Yi Radicals, Lisu, Vai", .start = 0xa400, },
    { .name = "Vai, Cyrillic Extended-B, Bamum, Tone Letters, Latin Extended-D", .start = 0xa600, },
    { .name = "Halfwidth and Fullwidth Forms", .start = 0xff00, },
    { .name = "Linear B Syllabary, Linear B Ideograms, Aegean Numbers, Phaistos Disc", .start = 0x10000, },
    { .name = "Lycian, Carian, Coptic Epact Numbers, Old Italic, Gothic, Old Permic", .start = 0x10200, },
    { .name = "Cuneiform", .start = 0x12000, },
    { .name = "Cuneiform (cont.)", .start = 0x12200, },
    { .name = "Byzantine Musical Symbols, Musical Symbols", .start = 0x1d000, },
    { .name = "Ancient Greek Musical Notation, Mayan Numerals, Tai Xuan Jing, Counting Rods", .start = 0x1d200, },
    { .name = "Mathematical Alphanumeric Symbols", .start = 0x1d400, },
    { .name = "Mathematical Alphanumeric Symbols (cont.)", .start = 0x1d600, },
    { .name = "Sutton SignWriting", .start = 0x1d800, },
    { .name = "Glagolitic Supplement, Nyiakeng Puachue Hmong", .start = 0x1e000, },
    { .name = "Ottoman Siyaq Numbers", .start = 0x1ed00, },
    { .name = "Arabic Mathematical Alphabetic Symbols", .start = 0x1ee00, },
    { .name = "Mahjong Tiles, Domino Tiles, Playing Cards", .start = 0x1f000, },
    { .name = "Enclosed Ideographic Supplement, Miscellaneous Symbols", .start = 0x1f200, },
    { .name = "Miscellaneous Symbols and Pictographs (cont.)", .start = 0x1f400, },
    { .name = "Emoticons, Ornamental Dingbats, Transport and Map Symbols", .start = 0x1f600, },
    { .name = "Supplemental Arrows-C, Supplemental Symbols", .start = 0x1f800, },
    { .name = "Chess Symbols, Symbols and Pictographs Extended-A", .start = 0x1fa00, },
  };
  size_t sindex;
  // we don't want a full delay period for each one, urk...or do we?
  struct timespec subdelay;
  uint64_t nstotal = timespec_to_ns(&demodelay);
  ns_to_timespec(nstotal / 5, &subdelay);
  for(sindex = 0 ; sindex < sizeof(blocks) / sizeof(*blocks) ; ++sindex){
    ncplane_set_bg_rgb(n, 0, 0, 0);
    //ncplane_erase(n);
    uint32_t blockstart = blocks[sindex].start;
    const char* description = blocks[sindex].name;
    ncplane_set_fg_rgb(n, 0xad, 0xd8, 0xe6);
    if(ncplane_cursor_move_yx(n, 1, (maxx - 26) / 2)){
      return -1;
    }
    if(ncplane_printf(n, "Unicode points %05x–%05x", blockstart, blockstart + BLOCKSIZE) <= 0){
      return -1;
    }
    int xstart = (maxx - ((CHUNKSIZE * 2) + 3)) / 2;
    struct ncplane* nn;
    if((nn = notcurses_newplane(nc, BLOCKSIZE / CHUNKSIZE + 2, (CHUNKSIZE * 2) + 2, 3, xstart, NULL)) == NULL){
      return -1;
    }
    if(draw_block(nn, blockstart)){
      return -1;
    }
    ncplane_set_fg_rgb(n, 0x40, 0xc0, 0x40);
    if(ncplane_cursor_move_yx(n, 6 + BLOCKSIZE / CHUNKSIZE, 0)){
      return -1;
    }
    if(ncplane_printf(n, "%*.*s", maxx, maxx, "") <= 0){
      return -1;
    }
    if(ncplane_cursor_move_yx(n, 6 + BLOCKSIZE / CHUNKSIZE, (maxx - strlen(description)) / 2)){
      return -1;
    }
    if(ncplane_printf(n, "%s", description) <= 0){
      return -1;
    }
    if(fade_block(nn, &subdelay)){ // destroys nn
      return -1;
    }
    // for a 32-bit wchar_t, we would want up through 24 bits of block ID. but
    // really, the vast majority of space is unused.
    blockstart += BLOCKSIZE;
  }
  return 0;
}

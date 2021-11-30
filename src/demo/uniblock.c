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

// idx/max: how far we are block-wise, and now many blocks we have total
static int
fade_block(struct notcurses* nc, struct ncplane* nn, const struct timespec* subdelay,
           struct ncprogbar* pbar, size_t idx, size_t max){
  //int ret = ncplane_fadein(nn, subdelay, demo_fader);
  ncprogbar_set_progress(pbar, (double)idx / max);
  int ret = demo_render(nc);
  demo_nanosleep(nc, subdelay);
  ncplane_destroy(nn);
  return ret;
}

// negative row will result in vertical pbar on the left side
static struct ncprogbar*
pbar_make(struct notcurses* nc, int row){
  unsigned dimx, dimy;
  struct ncplane* std = notcurses_stddim_yx(nc, &dimy, &dimx);
  struct ncplane_options nopts = {
    .y = row < 0 ? 4 : row,
    .x = row < 0 ? 1 : NCALIGN_CENTER,
    .rows = row < 0 ? dimy - 5 : 1,
    .cols = row < 0 ? 1 : 20,
    .name = "pbar",
    .flags = row < 0 ? 0 : NCPLANE_OPTION_HORALIGNED,
  };
  struct ncplane* pbar = ncplane_create(std, &nopts);
  if(pbar == NULL){
    return NULL;
  }
  uint64_t channels = 0;
  ncchannels_set_bg_rgb(&channels, 0);
  ncplane_set_base(pbar, "", 0, channels);
  int posy, posx;
  ncplane_yx(pbar, &posy, &posx);
  unsigned pdimy, pdimx;
  ncplane_dim_yx(pbar, &pdimy, &pdimx);
  ncplane_cursor_move_yx(std, posy - 1, posx - 1);
  channels = 0;
  ncchannels_set_fg_rgb8(&channels, 0, 0xde, 0xde);
  if(ncplane_rounded_box(std, 0, channels, posy + pdimy, posx + pdimx, 0)){
    ncplane_destroy(pbar);
    return NULL;
  }
  struct ncprogbar_options popts = {0};
  ncchannel_set_rgb8(&popts.ulchannel, 0x22, 0x22, 0x80);
  ncchannel_set_rgb8(&popts.urchannel, 0x22, 0x22, 0x80);
  ncchannel_set_rgb8(&popts.blchannel, 0x80, 0x80, 0x22);
  ncchannel_set_rgb8(&popts.brchannel, 0x80, 0x80, 0x22);
  struct ncprogbar* ncp = ncprogbar_create(pbar, &popts);
  if(ncp == NULL){
    return NULL;
  }
  return ncp;
}

static int
draw_block(struct ncplane* nn, uint32_t blockstart){
  unsigned dimx, dimy;
  ncplane_dim_yx(nn, &dimy, &dimx);
  nccell ul = NCCELL_TRIVIAL_INITIALIZER, ur = NCCELL_TRIVIAL_INITIALIZER;
  nccell ll = NCCELL_TRIVIAL_INITIALIZER, lr = NCCELL_TRIVIAL_INITIALIZER;
  nccell hl = NCCELL_TRIVIAL_INITIALIZER, vl = NCCELL_TRIVIAL_INITIALIZER;
  nccells_rounded_box(nn, 0, 0, &ul, &ur, &ll, &lr, &hl, &vl);
  nccell_set_bg_alpha(&ul, NCALPHA_TRANSPARENT);
  nccell_set_bg_alpha(&ur, NCALPHA_TRANSPARENT);
  nccell_set_bg_alpha(&ll, NCALPHA_TRANSPARENT);
  nccell_set_bg_alpha(&lr, NCALPHA_TRANSPARENT);
  nccell_set_fg_rgb8(&ul, 0xea, 0xaa, 0x00);
  nccell_set_fg_rgb8(&ur, 0x00, 0x30, 0x57);
  nccell_set_fg_rgb8(&ll, 0x00, 0x30, 0x57);
  nccell_set_fg_rgb8(&lr, 0xea, 0xaa, 0x00);
  // see https://github.com/dankamongmen/notcurses/issues/259. we use a random
  // (but dark) background for the perimeter to force refreshing on the box,
  // when it might otherwise be molested by RTL text. hacky and gross :( FIXME
  int rbg = rand() % 20;
  nccell_set_bg_rgb(&ul, rbg);
  nccell_set_bg_rgb(&ur, rbg);
  nccell_set_bg_rgb(&ll, rbg);
  nccell_set_bg_rgb(&lr, rbg);
  nccell_set_fg_rgb8(&hl, 255, 255, 255);
  nccell_set_fg_rgb8(&vl, 255, 255, 255);
  nccell_set_bg_rgb8(&hl, 0, 0, 0);
  nccell_set_bg_rgb8(&vl, 0, 0, 0);
  ncplane_home(nn);
  unsigned control = NCBOXGRAD_TOP | NCBOXGRAD_BOTTOM | NCBOXGRAD_LEFT | NCBOXGRAD_RIGHT;
  if(ncplane_box_sized(nn, &ul, &ur, &ll, &lr, &hl, &vl, dimy, dimx, control)){
    return -1;
  }
  nccell_release(nn, &ul); nccell_release(nn, &ur); nccell_release(nn, &hl);
  nccell_release(nn, &ll); nccell_release(nn, &lr); nccell_release(nn, &vl);
  int chunk;
  for(chunk = 0 ; chunk < BLOCKSIZE / CHUNKSIZE ; ++chunk){
    int z;
    ncplane_set_bg_rgb8(nn, 8 * chunk, 8 * chunk, 8 * chunk);
    for(z = 0 ; z < CHUNKSIZE ; ++z){
      uint32_t w = blockstart + chunk * CHUNKSIZE + z;
      if(ncplane_putstr_yx(nn, chunk + 1, z * 2 + 1, "  ") < 0){
        return -1;
      }
      // problematic characters FIXME (see TERMINALS.md)
      if(w != 0x070f && w != 0x08e2 && w != 0x06dd){
        ncplane_set_fg_rgb8(nn, 0xad + z * 2, 0xff, 0x2f - z * 2);
        ncplane_putwc_yx(nn, chunk + 1, z * 2 + 1, w);
      }
    }
  }
  return 0;
}

int uniblock_demo(struct notcurses* nc, uint64_t startns){
  (void)startns;
  unsigned maxx, maxy;
  struct ncplane* n = notcurses_stddim_yx(nc, &maxy, &maxx);
  // some blocks are good for the printing, some less so. some are only
  // marginally covered by mainstream fonts, some not at all. we explicitly
  // list the ones we want.
  const struct {
    const char* name;
    uint32_t start;
  } blocks[] = {
    // FIXME use full titles, and give several rows to the block name, using
    //       ncplane_puttext(NCALIGN_CENTER). some here are truncated.
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
    { .name = "Syloti Nagri, Phags-pa, Kayah Li, Javanese", .start = 0xa800, },
    { .name = "Cham, Tai Viet, Cherokee Supplement", .start = 0xaa00, },
    { .name = "Hangul Syllables", .start = 0xac00, },
    { .name = "Hangul Syllables", .start = 0xae00, },
    { .name = "CJK Compatibility Ideographs", .start = 0xf800, },
    { .name = "CJK Compatibility Ideographs, Alphabetic Presentation Forms", .start = 0xfa00, },
    { .name = "Arabic Presentation Forms-A", .start = 0xfc00, },
    { .name = "Halfwidth and Fullwidth Forms", .start = 0xfe00, },
    { .name = "Linear B Syllabary, Linear B Ideograms, Aegean, Phaistos Disc", .start = 0x10000, },
    { .name = "Lycian, Carian, Coptic Epact, Old Italic, Gothic, Old Permic", .start = 0x10200, },
    { .name = "Cuneiform", .start = 0x12000, },
    { .name = "Cuneiform (cont.)", .start = 0x12200, },
    { .name = "Byzantine Musical Symbols, Musical Symbols", .start = 0x1d000, },
    { .name = "Greek Musical Notation, Mayan Numerals, Tai Xuan Jing, Counting Rods", .start = 0x1d200, },
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

  // this demo is completely meaningless outside UTF-8 mode
  if(!notcurses_canutf8(nc)){
    return 0;
  }
  ncplane_greyscale(notcurses_stdplane(nc));
  unsigned pbarrow = 4 + BLOCKSIZE / CHUNKSIZE + 4;
  if(pbarrow > maxy - 2){
    pbarrow = -1;
  }
  struct ncprogbar* pbar = pbar_make(nc, pbarrow);
  if(!pbar){
    return -1;
  }
  size_t sindex;
  // we don't want a full delay period for each one, urk...or do we?
  struct timespec subdelay;
  uint64_t nstotal = timespec_to_ns(&demodelay);
  ns_to_timespec(nstotal / 5, &subdelay);
  ncplane_options nopts = {
    .y = 2,
    .x = NCALIGN_CENTER,
    .rows = 2,
    .cols = (CHUNKSIZE * 2) - 2,
    .flags = NCPLANE_OPTION_HORALIGNED,
  };
  struct ncplane* header = ncplane_create(n, &nopts);
  if(header == NULL){
    return -1;
  }
  uint64_t channels = 0;
  ncchannels_set_fg_alpha(&channels, NCALPHA_BLEND);
  ncchannels_set_fg_rgb(&channels, 0x004000);
  ncchannels_set_bg_rgb(&channels, 0x0);
  ncplane_set_base(header, "", 0, channels);
  for(sindex = 0 ; sindex < sizeof(blocks) / sizeof(*blocks) ; ++sindex){
    ncplane_set_bg_rgb8(n, 0, 0, 0);
    uint32_t blockstart = blocks[sindex].start;
    const char* description = blocks[sindex].name;
    ncplane_set_bg_rgb(header, 0);
    ncplane_set_fg_rgb(header, 0xbde8f6);
    if(ncplane_printf_aligned(header, 1, NCALIGN_CENTER, "Unicode points 0x%05x—0x%05x (%u—%u)",
                              blockstart, blockstart + BLOCKSIZE,
                              blockstart, blockstart + BLOCKSIZE) <= 0){
      return -1;
    }
    struct ncplane* nn;
    nopts.rows = BLOCKSIZE / CHUNKSIZE + 2;
    nopts.cols = (CHUNKSIZE * 2) + 2;
    nopts.y = 4;
    if((nn = ncplane_create(n, &nopts)) == NULL){
      return -1;
    }
    if(draw_block(nn, blockstart)){
      return -1;
    }
    if(ncplane_set_fg_rgb8(n, 0x40, 0xc0, 0x40)){
      return -1;
    }
    if(ncplane_cursor_move_yx(n, 6 + BLOCKSIZE / CHUNKSIZE, 4)){
      return -1;
    }
    if(ncplane_printf(n, "%*.*s", maxx - 8, maxx - 8, "") <= 0){
      return -1;
    }
    if(ncplane_printf_aligned(n, 6 + BLOCKSIZE / CHUNKSIZE, NCALIGN_CENTER, "%s", description) <= 0){
      return -1;
    }
    int err;
    if( (err = fade_block(nc, nn, &subdelay, pbar, sindex + 1, sizeof(blocks) / sizeof(*blocks))) ){ // destroys nn
      return err;
    }
    // for a 32-bit wchar_t, we would want up through 24 bits of block ID. but
    // really, the vast majority of space is unused.
  }
  ncplane_destroy(header);
  ncprogbar_destroy(pbar);
  return 0;
}

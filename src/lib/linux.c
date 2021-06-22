#include "internal.h"

#ifdef __linux__
#include <linux/kd.h>
#include <sys/ioctl.h>

// each row is a contiguous set of bits, starting at the msb
static inline size_t
row_bytes(const struct console_font_op* cfo){
  return (cfo->width + 7) / 8;
}

static inline size_t
glyph_bytes(const struct console_font_op* cfo){
  size_t minb = row_bytes(cfo) * cfo->height;
  return (minb + 31) / 32 * 32;
}

static unsigned char*
get_glyph(struct console_font_op* cfo, unsigned idx){
  if(idx >= cfo->charcount){
    return NULL;
  }
  return (unsigned char*)cfo->data + glyph_bytes(cfo) * idx;
}

// idx is the glyph index within cfo->data. qbits are the occupied quadrants:
//  0x8 = upper left
//  0x4 = upper right
//  0x2 = lower left
//  0x1 = lower right
static int
shim_quad_block(struct console_font_op* cfo, unsigned idx, unsigned qbits){
  unsigned char* glyph = get_glyph(cfo, idx);
  if(glyph == NULL){
    return -1;
  }
  unsigned r;
  for(r = 0 ; r < cfo->height / 2 ; ++r){
    unsigned char mask = 0x80;
    unsigned char* row = glyph + row_bytes(cfo) * r;
    unsigned x;
    *row = 0;
    for(x = 0 ; x < cfo->width / 2 ; ++x){
      if(qbits & 0x8){
        *row |= mask;
      }
      if((mask >>= 1) == 0){
        mask = 0x80;
        *++row = 0;
      }
    }
    while(x < cfo->width){
      if(qbits & 0x4){
        *row |= mask;
      }
      if((mask >>= 1) == 0){
        mask = 0x80;
        *++row = 0;
      }
      ++x;
    }
  }
  while(r < cfo->height){
    unsigned char mask = 0x80;
    unsigned char* row = glyph + row_bytes(cfo) * r;
    unsigned x;
    *row = 0;
    for(x = 0 ; x < cfo->width / 2 ; ++x){
      if(qbits & 0x2){
        *row |= mask;
      }
      if((mask >>= 1) == 0){
        mask = 0x80;
        *++row = 0;
      }
    }
    while(x < cfo->width){
      if(qbits & 0x1){
        *row |= mask;
      }
      if((mask >>= 1) == 0){
        mask = 0x80;
        *++row = 0;
      }
      ++x;
    }
    ++r;
  }
  return 0;
}

// use for drawing 1, 2, 3, 5, 6, and 7/8ths
static int
shim_lower_eighths(struct console_font_op* cfo, unsigned idx, int eighths){
  unsigned char* glyph = get_glyph(cfo, idx);
  if(glyph == NULL){
    return -1;
  }
  unsigned ten8ths = cfo->height * 10 / 8;
  unsigned start = cfo->height - (eighths * ten8ths / 10);
  unsigned r;
  for(r = 0 ; r < start ; ++r){
    unsigned char* row = glyph + row_bytes(cfo) * r;
    for(unsigned x = 0 ; x < cfo->width ; x += 8){
      row[x / 8] = 0;
    }
  }
  while(r < cfo->height){
    unsigned char* row = glyph + row_bytes(cfo) * r;
    for(unsigned x = 0 ; x < cfo->width ; x += 8){
      row[x / 8] = 0xff;
    }
    ++r;
  }
  return 0;
}

// add UCS2 codepoint |w| to |map| for font idx |fidx|
static int
add_to_map(struct unimapdesc* map, wchar_t w, unsigned fidx){
  logdebug("Adding mapping U+%04x -> %03u\n", w, fidx);
  struct unipair* tmp = realloc(map->entries, sizeof(*map->entries) * (map->entry_ct + 1));
  if(tmp == NULL){
    return -1;
  }
  map->entries = tmp;
  map->entries[map->entry_ct].unicode = w;
  map->entries[map->entry_ct].fontpos = fidx;
  ++map->entry_ct;
  return 0;
}

static int
program_line_drawing_chars(const notcurses* nc, struct unimapdesc* map){
  struct simset {
    wchar_t* ws;
  } sets[] = {
    {
      .ws = L"/╱",
    }, {
      .ws = L"\\╲",
    }, {
      .ws = L"X╳☒",
    }, {
      .ws = L"O☐",
    }, {
      .ws = L"└┕┖┗╘╙╚╰",
    }, {
      .ws = L"┘┙┚┛╛╜╝╯",
    }, {
      .ws = L"┌┍┎┏╒╓╔╭",
    }, {
      .ws = L"┐┑┒┓╕╖╗╮",
    }, {
      .ws = L"─━┄┅┈┉╌╍═╼╾",
    }, {
      .ws = L"│┃┆┇┊┋╎╏║╽╿",
    }, {
      .ws = L"├┝┞┟┠┡┢┣╞╟╠",
    }, {
      .ws = L"┤┥┦┧┨┩┪┫╡╢╣",
    }, {
      .ws = L"┬┭┮┯┰┱┲┳╤╥╦",
    }, {
      .ws = L"┴┵┶┷┸┹┺┻╧╨╩",
    }, {
      .ws = L"┼┽┾┿╀╁╂╃╄╅╆╇╈╉╊╋╪╫╬",
    },
  };
  int toadd = 0;
  for(size_t sidx = 0 ; sidx < sizeof(sets) / sizeof(*sets) ; ++sidx){
    int fontidx = -1;
    struct simset* s = &sets[sidx];
    bool found[wcslen(s->ws)];
    memset(found, 0, sizeof(found));
    for(unsigned idx = 0 ; idx < map->entry_ct ; ++idx){
      for(size_t widx = 0 ; widx < wcslen(s->ws) ; ++widx){
        if(map->entries[idx].unicode == s->ws[widx]){
          logtrace("Found desired character U+%04x -> %03u\n",
                   map->entries[idx].unicode, map->entries[idx].fontpos);
          found[widx] = true;
          if(fontidx == -1){
            fontidx = map->entries[idx].fontpos;
          }
        }
      }
    }
    if(fontidx > -1){
      for(size_t widx = 0 ; widx < wcslen(s->ws) ; ++widx){
        if(!found[widx]){
          if(add_to_map(map, s->ws[widx], fontidx)){
            return -1;
          }
          ++toadd;
        }
      }
    }else{
      logwarn("Couldn't find any glyphs for set %zu\n", sidx);
    }
  }
  if(toadd == 0){
    return 0;
  }
  if(ioctl(nc->ttyfd, PIO_UNIMAP, map)){
    logwarn("Error setting kernel unicode map (%s)\n", strerror(errno));
    return -1;
  }
  loginfo("Successfully added %d kernel unicode mapping%s\n",
          toadd, toadd == 1 ? "" : "s");
  return 0;
}

static int
program_block_drawing_chars(const notcurses* nc, struct console_font_op* cfo,
                            struct unimapdesc* map){
  struct shimmer {
    unsigned qbits;
    wchar_t w;
    bool found;
  } shimmers[] = {
    { .qbits = 0xc, .w = L'▀', .found = false, },
    { .qbits = 0x3, .w = L'▄', .found = false, },
    { .qbits = 0xa, .w = L'▌', .found = false, },
    { .qbits = 0x5, .w = L'▐', .found = false, },
    { .qbits = 0x8, .w = L'▘', .found = false, },
    { .qbits = 0x4, .w = L'▝', .found = false, },
    { .qbits = 0x2, .w = L'▖', .found = false, },
    { .qbits = 0x1, .w = L'▗', .found = false, },
    { .qbits = 0x7, .w = L'▟', .found = false, },
    { .qbits = 0xb, .w = L'▙', .found = false, },
    { .qbits = 0xd, .w = L'▜', .found = false, },
    { .qbits = 0xe, .w = L'▛', .found = false, },
    { .qbits = 0x9, .w = L'▚', .found = false, },
    { .qbits = 0x6, .w = L'▞', .found = false, },
  };
  struct shimmer eighths[] = {
    { .qbits = 7, .w = L'▇', .found = false, },
    { .qbits = 6, .w = L'▆', .found = false, },
    { .qbits = 5, .w = L'▅', .found = false, },
    { .qbits = 3, .w = L'▃', .found = false, },
    { .qbits = 2, .w = L'▂', .found = false, },
    { .qbits = 1, .w = L'▁', .found = false, },
  };
  // first, take a pass to see which glyphs we already have
  for(unsigned i = 0 ; i < cfo->charcount ; ++i){
    if(map->entries[i].unicode >= 0x2580 && map->entries[i].unicode <= 0x259f){
      for(size_t s = 0 ; s < sizeof(shimmers) / sizeof(*shimmers) ; ++s){
        if(map->entries[i].unicode == shimmers[s].w){
          logdebug("Found %lc at fontidx %u\n", shimmers[s].w, i);
          shimmers[s].found = true;
          break;
        }
      }
      for(size_t s = 0 ; s < sizeof(eighths) / sizeof(*eighths) ; ++s){
        if(map->entries[i].unicode == eighths[s].w){
          logdebug("Found %lc at fontidx %u\n", eighths[s].w, i);
          eighths[s].found = true;
          break;
        }
      }
    }
  }
  int added = 0;
  unsigned candidate = cfo->charcount;
  for(size_t s = 0 ; s < sizeof(shimmers) / sizeof(*shimmers) ; ++s){
    if(!shimmers[s].found){
      while(--candidate){
        if(map->entries[candidate].unicode < 0x2580 || map->entries[candidate].unicode > 0x259f){
          break;
        }
      }
      if(candidate == 0){
        logwarn("Ran out of replaceable glyphs for U+%04lx\n", (long)shimmers[s].w);
        return -1;
      }
      if(shim_quad_block(cfo, candidate, shimmers[s].qbits)){
        logwarn("Error replacing glyph for U+%04lx at %u\n", (long)shimmers[s].w, candidate);
        return -1;
      }
      if(add_to_map(map, shimmers[s].w, candidate)){
        return -1;
      }
      ++added;
    }
  }
  for(size_t s = 0 ; s < sizeof(eighths) / sizeof(*eighths) ; ++s){
    if(!eighths[s].found){
      while(--candidate){
        if(map->entries[candidate].unicode < 0x2580 || map->entries[candidate].unicode > 0x259f){
          break;
        }
      }
      if(candidate == 0){
        logwarn("Ran out of replaceable glyphs for U+%04lx\n", (long)eighths[s].w);
        return -1;
      }
      if(shim_lower_eighths(cfo, candidate, eighths[s].qbits)){
        logwarn("Error replacing glyph for U+%04lx at %u\n", (long)eighths[s].w, candidate);
        return -1;
      }
      if(add_to_map(map, eighths[s].w, candidate)){
        return -1;
      }
      ++added;
    }
  }
  cfo->op = KD_FONT_OP_SET;
  if(ioctl(nc->ttyfd, KDFONTOP, cfo)){
    logwarn("Error programming kernel font (%s)\n", strerror(errno));
    return -1;
  }
  if(ioctl(nc->ttyfd, PIO_UNIMAP, map)){
    logwarn("Error setting kernel unicode map (%s)\n", strerror(errno));
    return -1;
  }
  loginfo("Successfully added %d kernel font glyph%s\n", added, added == 1 ? "" : "s");
  return 0;
}

static int
reprogram_linux_font(const notcurses* nc, struct console_font_op* cfo,
                     struct unimapdesc* map){
  if(ioctl(nc->ttyfd, KDFONTOP, cfo)){
    logwarn("Error reading Linux kernelfont (%s)\n", strerror(errno));
    return -1;
  }
  loginfo("Kernel font size (glyphcount): %hu\n", cfo->charcount);
  loginfo("Kernel font character geometry: %hux%hu\n", cfo->width, cfo->height);
  if(cfo->charcount > 512){
    logwarn("Warning: kernel returned excess charcount\n");
    return -1;
  }
  if(ioctl(nc->ttyfd, GIO_UNIMAP, map)){
    logwarn("Error reading Linux unimap (%s)\n", strerror(errno));
    return -1;
  }
  loginfo("Kernel Unimap size: %hu/%hu\n", map->entry_ct, USHRT_MAX);
  // for certain sets of characters, we're not going to draw them in, but we
  // do want to ensure they map to something plausible...
  if(program_line_drawing_chars(nc, map)){
    return -1;
  }
  if(program_block_drawing_chars(nc, cfo, map)){
    return -1;
  }
  return 0;
}

static int
reprogram_console_font(const notcurses* nc){
  struct console_font_op cfo = {
    .op = KD_FONT_OP_GET,
    .charcount = 512,
    .height = 32,
    .width = 32,
  };
  size_t totsize = 128 * cfo.charcount; // FIXME enough?
  cfo.data = malloc(totsize);
  if(cfo.data == NULL){
    logwarn("Error acquiring %zub for font descriptors (%s)\n", totsize, strerror(errno));
    return -1;
  }
  struct unimapdesc map = {};
  map.entry_ct = USHRT_MAX;
  totsize = map.entry_ct * sizeof(struct unipair);
  map.entries = malloc(totsize);
  if(map.entries == NULL){
    logwarn("Error acquiring %zub for Unicode font map (%s)\n", totsize, strerror(errno));
    free(cfo.data);
    return -1;
  }
  int r = reprogram_linux_font(nc, &cfo, &map);
  free(cfo.data);
  free(map.entries);
  return r;
}

// is the provided fd a Linux console?
bool is_linux_console(const notcurses* nc, unsigned no_font_changes){
  if(nc->ttyfd < 0){
    return false;
  }
  int mode;
  if(ioctl(nc->ttyfd, KDGETMODE, &mode)){
    logdebug("Not a Linux console, KDGETMODE failed\n");
    return false;
  }
  loginfo("Verified Linux console, mode %d\n", mode);
  if(no_font_changes){
    logdebug("Not reprogramming the console font due to option\n");
    return true;
  }
  reprogram_console_font(nc);
  return true;
}
#else
bool is_linux_console(const notcurses* nc, unsigned no_font_changes){
  (void)nc;
  (void)no_font_changes;
  return false;
}
#endif

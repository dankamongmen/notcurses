#include "internal.h"

#ifdef __linux__
#include <linux/kd.h>
#include <sys/ioctl.h>

static unsigned char*
get_glyph(struct consolefontdesc* cfd, unsigned idx){
  if(idx >= cfd->charcount){
    return NULL;
  }
  return (unsigned char*)cfd->chardata + 32 * idx;
}

static int // insert U+2580 (upper half block)
shim_upper_half_block(struct consolefontdesc* cfd, unsigned idx){
  unsigned char* glyph = get_glyph(cfd, idx);
  if(glyph == NULL){
    return -1;
  }
  unsigned r;
  for(r = 0 ; r < cfd->charheight / 2 ; ++r, ++glyph){
    *glyph = 0xff;
  }
  while(r < cfd->charheight){
    *glyph = 0;
    ++glyph;
    ++r;
  }
  return 0;
}

static int // insert U+2584 (lower half block)
shim_lower_half_block(struct consolefontdesc* cfd, unsigned idx){
  unsigned char* glyph = get_glyph(cfd, idx);
  if(glyph == NULL){
    return -1;
  }
  unsigned r;
  for(r = 0 ; r < cfd->charheight / 2 ; ++r, ++glyph){
    *glyph = 0;
  }
  while(r < cfd->charheight){
    *glyph = 0xff;
    ++glyph;
    ++r;
  }
  return 0;
}

static int // insert U+258c (left half block)
shim_left_half_block(struct consolefontdesc* cfd, unsigned idx){
  unsigned char* glyph = get_glyph(cfd, idx);
  if(glyph == NULL){
    return -1;
  }
  for(unsigned r = 0 ; r < cfd->charheight ; ++r, ++glyph){
    *glyph = 0xf0;
  }
  return 0;
}

static int // insert U+2590 (right half block)
shim_right_half_block(struct consolefontdesc* cfd, unsigned idx){
  unsigned char* glyph = get_glyph(cfd, idx);
  if(glyph == NULL){
    return -1;
  }
  for(unsigned r = 0 ; r < cfd->charheight ; ++r, ++glyph){
    *glyph = 0x0f;
  }
  return 0;
}

static int // insert U+2598 (quadrant upper left)
shim_upper_left_quad(struct consolefontdesc* cfd, unsigned idx){
  unsigned char* glyph = get_glyph(cfd, idx);
  if(glyph == NULL){
    return -1;
  }
  for(unsigned r = 0 ; r < cfd->charheight / 2 ; ++r, ++glyph){
    *glyph = 0xf0;
  }
  for(unsigned r = cfd->charheight / 2 ; r < cfd->charheight ; ++r, ++glyph){
    *glyph = 0;
  }
  return 0;
}

static int // insert U+259D (quadrant upper right)
shim_upper_right_quad(struct consolefontdesc* cfd, unsigned idx){
  unsigned char* glyph = get_glyph(cfd, idx);
  if(glyph == NULL){
    return -1;
  }
  for(unsigned r = 0 ; r < cfd->charheight / 2 ; ++r, ++glyph){
    *glyph = 0x0f;
  }
  for(unsigned r = cfd->charheight / 2 ; r < cfd->charheight ; ++r, ++glyph){
    *glyph = 0;
  }
  return 0;
}

static int // insert U+2598 (quadrant lower left)
shim_lower_left_quad(struct consolefontdesc* cfd, unsigned idx){
  unsigned char* glyph = get_glyph(cfd, idx);
  if(glyph == NULL){
    return -1;
  }
  for(unsigned r = 0 ; r < cfd->charheight / 2 ; ++r, ++glyph){
    *glyph = 0;
  }
  for(unsigned r = cfd->charheight / 2 ; r < cfd->charheight ; ++r, ++glyph){
    *glyph = 0xf0;
  }
  return 0;
}

static int // insert U+2597 (quadrant lower right)
shim_lower_right_quad(struct consolefontdesc* cfd, unsigned idx){
  unsigned char* glyph = get_glyph(cfd, idx);
  if(glyph == NULL){
    return -1;
  }
  for(unsigned r = 0 ; r < cfd->charheight / 2 ; ++r, ++glyph){
    *glyph = 0;
  }
  for(unsigned r = cfd->charheight / 2 ; r < cfd->charheight ; ++r, ++glyph){
    *glyph = 0x0f;
  }
  return 0;
}

static int
shim_no_upper_left_quad(struct consolefontdesc* cfd, unsigned idx){
  unsigned char* glyph = get_glyph(cfd, idx);
  if(glyph == NULL){
    return -1;
  }
  for(unsigned r = 0 ; r < cfd->charheight / 2 ; ++r, ++glyph){
    *glyph = 0x0f;
  }
  for(unsigned r = cfd->charheight / 2 ; r < cfd->charheight ; ++r, ++glyph){
    *glyph = 0xff;
  }
  return 0;
}

static int
shim_no_upper_right_quad(struct consolefontdesc* cfd, unsigned idx){
  unsigned char* glyph = get_glyph(cfd, idx);
  if(glyph == NULL){
    return -1;
  }
  for(unsigned r = 0 ; r < cfd->charheight / 2 ; ++r, ++glyph){
    *glyph = 0xf0;
  }
  for(unsigned r = cfd->charheight / 2 ; r < cfd->charheight ; ++r, ++glyph){
    *glyph = 0xff;
  }
  return 0;
}

static int
shim_no_lower_left_quad(struct consolefontdesc* cfd, unsigned idx){
  unsigned char* glyph = get_glyph(cfd, idx);
  if(glyph == NULL){
    return -1;
  }
  for(unsigned r = 0 ; r < cfd->charheight / 2 ; ++r, ++glyph){
    *glyph = 0xff;
  }
  for(unsigned r = cfd->charheight / 2 ; r < cfd->charheight ; ++r, ++glyph){
    *glyph = 0x0f;
  }
  return 0;
}

static int
shim_no_lower_right_quad(struct consolefontdesc* cfd, unsigned idx){
  unsigned char* glyph = get_glyph(cfd, idx);
  if(glyph == NULL){
    return -1;
  }
  for(unsigned r = 0 ; r < cfd->charheight / 2 ; ++r, ++glyph){
    *glyph = 0xff;
  }
  for(unsigned r = cfd->charheight / 2 ; r < cfd->charheight ; ++r, ++glyph){
    *glyph = 0xf0;
  }
  return 0;
}

static int
shim_quad_ul_lr(struct consolefontdesc* cfd, unsigned idx){
  unsigned char* glyph = get_glyph(cfd, idx);
  if(glyph == NULL){
    return -1;
  }
  for(unsigned r = 0 ; r < cfd->charheight / 2 ; ++r, ++glyph){
    *glyph = 0xf0;
  }
  for(unsigned r = cfd->charheight / 2 ; r < cfd->charheight ; ++r, ++glyph){
    *glyph = 0x0f;
  }
  return 0;
}

static int
shim_quad_ll_ur(struct consolefontdesc* cfd, unsigned idx){
  unsigned char* glyph = get_glyph(cfd, idx);
  if(glyph == NULL){
    return -1;
  }
  for(unsigned r = 0 ; r < cfd->charheight / 2 ; ++r, ++glyph){
    *glyph = 0x0f;
  }
  for(unsigned r = cfd->charheight / 2 ; r < cfd->charheight ; ++r, ++glyph){
    *glyph = 0xf0;
  }
  return 0;
}

static int
shim_lower_seven_eighth(struct consolefontdesc* cfd, unsigned idx){
  unsigned char* glyph = get_glyph(cfd, idx);
  if(glyph == NULL){
    return -1;
  }
  for(unsigned r = 0 ; r < cfd->charheight / 8 ; ++r, ++glyph){
    *glyph = 0;
  }
  for(unsigned r = cfd->charheight / 8 ; r < cfd->charheight ; ++r, ++glyph){
    *glyph = 0xff;
  }
  return 0;
}

static int
shim_lower_three_quarter(struct consolefontdesc* cfd, unsigned idx){
  unsigned char* glyph = get_glyph(cfd, idx);
  if(glyph == NULL){
    return -1;
  }
  for(unsigned r = 0 ; r < cfd->charheight / 4 ; ++r, ++glyph){
    *glyph = 0;
  }
  for(unsigned r = cfd->charheight / 4 ; r < cfd->charheight ; ++r, ++glyph){
    *glyph = 0xff;
  }
  return 0;
}

static int
shim_lower_five_eighth(struct consolefontdesc* cfd, unsigned idx){
  unsigned char* glyph = get_glyph(cfd, idx);
  if(glyph == NULL){
    return -1;
  }
  for(unsigned r = 0 ; r < cfd->charheight * 5 / 8 ; ++r, ++glyph){
    *glyph = 0;
  }
  for(unsigned r = cfd->charheight * 5 / 8 ; r < cfd->charheight ; ++r, ++glyph){
    *glyph = 0xff;
  }
  return 0;
}

static int
shim_lower_three_eighth(struct consolefontdesc* cfd, unsigned idx){
  unsigned char* glyph = get_glyph(cfd, idx);
  if(glyph == NULL){
    return -1;
  }
  for(unsigned r = 0 ; r < cfd->charheight * 3 / 8 ; ++r, ++glyph){
    *glyph = 0;
  }
  for(unsigned r = cfd->charheight * 3 / 8 ; r < cfd->charheight ; ++r, ++glyph){
    *glyph = 0xff;
  }
  return 0;
}

static int
shim_lower_quarter(struct consolefontdesc* cfd, unsigned idx){
  unsigned char* glyph = get_glyph(cfd, idx);
  if(glyph == NULL){
    return -1;
  }
  for(unsigned r = 0 ; r < cfd->charheight * 3 / 4 ; ++r, ++glyph){
    *glyph = 0;
  }
  for(unsigned r = cfd->charheight * 3 / 4 ; r < cfd->charheight ; ++r, ++glyph){
    *glyph = 0xff;
  }
  return 0;
}

static int
shim_lower_eighth(struct consolefontdesc* cfd, unsigned idx){
  unsigned char* glyph = get_glyph(cfd, idx);
  if(glyph == NULL){
    return -1;
  }
  for(unsigned r = 0 ; r < cfd->charheight * 7 / 8 ; ++r, ++glyph){
    *glyph = 0;
  }
  for(unsigned r = cfd->charheight * 7 / 8 ; r < cfd->charheight ; ++r, ++glyph){
    *glyph = 0xff;
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
program_block_drawing_chars(const notcurses* nc, struct consolefontdesc* cfd,
                            struct unimapdesc* map){
  struct shimmer {
    int (*glyphfxn)(struct consolefontdesc* cfd, unsigned idx);
    wchar_t w;
    bool found;
  } shimmers[] = {
    { .glyphfxn = shim_upper_half_block, .w = L'▀', .found = false, },
    { .glyphfxn = shim_lower_half_block, .w = L'▄', .found = false, },
    { .glyphfxn = shim_left_half_block, .w = L'▌', .found = false, },
    { .glyphfxn = shim_right_half_block, .w = L'▐', .found = false, },
    { .glyphfxn = shim_upper_left_quad, .w = L'▘', .found = false, },
    { .glyphfxn = shim_upper_right_quad, .w = L'▝', .found = false, },
    { .glyphfxn = shim_lower_left_quad, .w = L'▖', .found = false, },
    { .glyphfxn = shim_lower_right_quad, .w = L'▗', .found = false, },
    { .glyphfxn = shim_no_upper_left_quad, .w = L'▟', .found = false, },
    { .glyphfxn = shim_no_upper_right_quad, .w = L'▙', .found = false, },
    { .glyphfxn = shim_no_lower_left_quad, .w = L'▜', .found = false, },
    { .glyphfxn = shim_no_lower_right_quad, .w = L'▛', .found = false, },
    { .glyphfxn = shim_quad_ul_lr, .w = L'▚', .found = false, },
    { .glyphfxn = shim_quad_ll_ur, .w = L'▞', .found = false, },
    { .glyphfxn = shim_lower_seven_eighth, .w = L'▇', .found = false, },
    { .glyphfxn = shim_lower_three_quarter, .w = L'▆', .found = false, },
    { .glyphfxn = shim_lower_five_eighth, .w = L'▅', .found = false, },
    { .glyphfxn = shim_lower_three_eighth, .w = L'▃', .found = false, },
    { .glyphfxn = shim_lower_quarter, .w = L'▂', .found = false, },
    { .glyphfxn = shim_lower_eighth, .w = L'▁', .found = false, },
  };
  // first, take a pass to see which glyphs we already have
  for(unsigned i = 0 ; i < cfd->charcount ; ++i){
    if(map->entries[i].unicode >= 0x2580 && map->entries[i].unicode <= 0x259f){
      for(size_t s = 0 ; s < sizeof(shimmers) / sizeof(*shimmers) ; ++s){
        if(map->entries[i].unicode == shimmers[s].w){
          logdebug("Found %lc at fontidx %u\n", shimmers[s].w, i);
          shimmers[s].found = true;
          break;
        }
      }
    }
  }
  int added = 0;
  unsigned candidate = cfd->charcount;
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
      if(shimmers[s].glyphfxn(cfd, candidate)){
        logwarn("Error replacing glyph for U+%04lx at %u\n", (long)shimmers[s].w, candidate);
        return -1;
      }
      if(add_to_map(map, shimmers[s].w, candidate)){
        return -1;
      }
      ++added;
    }
  }
  if(ioctl(nc->ttyfd, PIO_FONTX, cfd)){
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
reprogram_linux_font(const notcurses* nc, struct consolefontdesc* cfd,
                     struct unimapdesc* map){
  if(ioctl(nc->ttyfd, GIO_FONTX, cfd)){
    logwarn("Error reading Linux kernelfont (%s)\n", strerror(errno));
    return -1;
  }
  loginfo("Kernel font size (glyphcount): %hu\n", cfd->charcount);
  loginfo("Kernel font character geometry: 8x%hu\n", cfd->charheight);
  if(cfd->charcount > 512){
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
  if(program_block_drawing_chars(nc, cfd, map)){
    return -1;
  }
  return 0;
}

static int
reprogram_console_font(const notcurses* nc){
  struct consolefontdesc cfd = {};
  cfd.charcount = 512;
  size_t totsize = 32 * cfd.charcount;
  cfd.chardata = malloc(totsize);
  if(cfd.chardata == NULL){
    logwarn("Error acquiring %zub for font descriptors (%s)\n", totsize, strerror(errno));
    return -1;
  }
  struct unimapdesc map = {};
  map.entry_ct = USHRT_MAX;
  totsize = map.entry_ct * sizeof(struct unipair);
  map.entries = malloc(totsize);
  if(map.entries == NULL){
    logwarn("Error acquiring %zub for Unicode font map (%s)\n", totsize, strerror(errno));
    free(cfd.chardata);
    return -1;
  }
  int r = reprogram_linux_font(nc, &cfd, &map);
  free(cfd.chardata);
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

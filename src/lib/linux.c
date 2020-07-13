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
program_line_drawing_chars(const notcurses* nc, struct unimapdesc* map){
  struct simset {
    bool found[10];
    wchar_t ws[10];
    int fontidx;
  } sets[] = {
    {
      .found = {},
      .ws = L"└┕┖┗╘╙╚╰",
      .fontidx = -1,
    }, {
      .found = {},
      .ws = L"┘┙┚┛╛╜╝╯",
      .fontidx = -1,
    }, {
      .found = {},
      .ws = L"┌┍┎┏╒╓╔╭",
      .fontidx = -1,
    }, {
      .found = {},
      .ws = L"┐┑┒┓╕╖╗╮",
      .fontidx = -1,
    }, {
      .found = {},
      .ws = L"─━┄┅┈┉╌╍═",
      .fontidx = -1,
    }, {
      .found = {},
      .ws = L"│┃┆┇┊┋╎╏║",
      .fontidx = -1,
    }
  };
  int toadd = 0;
  for(size_t sidx = 0 ; sidx < sizeof(sets) / sizeof(*sets) ; ++sidx){
    struct simset* s = &sets[sidx];
    for(unsigned idx = 0 ; idx < map->entry_ct ; ++idx){
      for(size_t widx = 0 ; widx < wcslen(s->ws) ; ++widx){
        if(map->entries[idx].unicode == s->ws[widx]){
          logtrace(nc, "Found desired character U+%04x -> %03u\n",
                   map->entries[idx].unicode, map->entries[idx].fontpos);
          s->found[widx] = true;
          if(s->fontidx == -1){
            s->fontidx = map->entries[idx].fontpos;
          }
        }
      }
    }
    if(s->fontidx > -1){
      for(size_t widx = 0 ; widx < wcslen(s->ws) ; ++widx){
        if(!s->found[widx]){
          logdebug(nc, "Adding mapping U+%04x -> %03u\n",
                   s->ws[widx], s->fontidx);
          struct unipair* tmp = realloc(map->entries, sizeof(*map->entries) * (map->entry_ct + 1));
          if(tmp == NULL){
            return -1;
          }
          map->entries = tmp;
          map->entries[map->entry_ct].unicode = s->ws[widx];
          map->entries[map->entry_ct].fontpos = s->fontidx;
          ++map->entry_ct;
          ++toadd;
        }
      }
    }else{
      logwarning(nc, "Couldn't find any glyphs for set %zu\n", sidx);
    }
  }
  if(toadd == 0){
    return 0;
  }
  if(ioctl(nc->ttyfd, PIO_UNIMAP, map)){
    logwarning(nc, "Error setting kernel unicode map (%s)\n", strerror(errno));
    return -1;
  }
  loginfo(nc, "Successfully added %d kernel unicode mapping%s\n",
          toadd, toadd == 1 ? "" : "s");
  return 0;
}

static int
reprogram_linux_font(const notcurses* nc, struct consolefontdesc* cfd,
                     struct unimapdesc* map){
  if(ioctl(nc->ttyfd, GIO_FONTX, cfd)){
    logwarning(nc, "Error reading Linux kernelfont (%s)\n", strerror(errno));
    return -1;
  }
  loginfo(nc, "Kernel font size (glyphcount): %hu\n", cfd->charcount);
  loginfo(nc, "Kernel font character geometry: 8x%hu\n", cfd->charheight);
  if(cfd->charcount > 512){
    logwarning(nc, "Warning: kernel returned excess charcount\n");
    return -1;
  }
  if(ioctl(nc->ttyfd, GIO_UNIMAP, map)){
    logwarning(nc, "Error reading Linux unimap (%s)\n", strerror(errno));
    return -1;
  }
  loginfo(nc, "Kernel Unimap size: %hu/%hu\n", map->entry_ct, USHRT_MAX);
  // for certain sets of characters, we're not going to draw them in, but we
  // do want to ensure they map to something plausible...
  if(program_line_drawing_chars(nc, map)){
    return -1;
  }
  for(unsigned idx = 0 ; idx < map->entry_ct ; ++idx){
    // FIXME check to see if our desired codepoints already map
    //  if already declared, trust it?
    // if not, see if we ought add one or reuse something
    // if we want to add, find a good place
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
    logwarning(nc, "Error acquiring %zub for font descriptors (%s)\n", totsize, strerror(errno));
    return -1;
  }
  struct unimapdesc map = {};
  map.entry_ct = USHRT_MAX;
  totsize = map.entry_ct * sizeof(struct unipair);
  map.entries = malloc(totsize);
  if(map.entries == NULL){
    logwarning(nc, "Error acquiring %zub for Unicode font map (%s)\n", totsize, strerror(errno));
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
  int mode, r;
  if( (r = ioctl(nc->ttyfd, KDGETMODE, &mode)) ){
    logdebug(nc, "Not a Linux console, KDGETMODE failed\n");
    return false;
  }
  loginfo(nc, "Verified Linux console, mode %d\n", mode);
  if(no_font_changes){
    logdebug(nc, "Not reprogramming the console font due to option\n");
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

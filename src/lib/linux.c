#include "internal.h"

#ifdef __linux__
#include <linux/kd.h>
#include <sys/ioctl.h>

// is the provided fd a Linux console?
bool is_linux_console(const notcurses* nc){
  int mode, r;
  if( (r = ioctl(nc->ttyfd, KDGETMODE, &mode)) ){
    logdebug(nc, "Not a Linux console, KDGETMODE failed\n");
    return false;
  }
  loginfo(nc, "Verified Linux console, mode %d\n", mode);
  return true;
}
#else
bool is_linux_console(const notcurses* nc){
  (void)nc;
  return false;
}

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

#endif

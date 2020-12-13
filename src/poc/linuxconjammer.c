#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <locale.h>
#include <wctype.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#ifdef __linux__
#include <linux/kd.h>
#include <sys/ioctl.h>

static void
usage(const char* argv){
  fprintf(stderr, "usage: %s [ ttydev ]\n", argv);
}

static int
get_tty_fd(const char* dev){
  if(dev == NULL){
    dev = "/dev/tty";
  }
  int fd = open(dev, O_RDWR | O_CLOEXEC);
  if(fd < 0){
    fprintf(stderr, "Error opening %s (%s)\n", dev, strerror(errno));
  }
  return fd;
}

static bool
is_linux_console(int fd){
  int mode;
  if(ioctl(fd, KDGETMODE, &mode)){
    fprintf(stderr, "Not a Linux console, KDGETMODE failed (%s)\n", strerror(errno));
    return false;
  }
  printf("Verified Linux console, %s mode\n",
         mode == KD_TEXT ? "text" :
         mode == KD_GRAPHICS ? "graphics" : "unknown");
  return true;
}

// FIXME assumes a width of 8. it is apparently possible to have widths
// other than 8, but they don't work properly with GIO_FONTX according to
// the showconsolefont source code. use the KDFONTOP ioctl to learn true
// font width.
static int
explode_glyph_row(const unsigned char** row){
  printf("%s%s%s%s%s%s%s%s ",
          **row & 0x80 ? "*": " ",
          **row & 0x40 ? "*": " ",
          **row & 0x20 ? "*": " ",
          **row & 0x10 ? "*": " ",
          **row & 0x08 ? "*": " ",
          **row & 0x04 ? "*": " ",
          **row & 0x02 ? "*": " ",
          **row & 0x01 ? "*": " ");
  ++*row;
  return 0;
}

static int
shim_upper_half_block(struct consolefontdesc* cfd, unsigned idx){
  if(idx >= cfd->charcount){
    return -1;
  }
  unsigned char* glyph = (unsigned char*)cfd->chardata + 32 * idx;
  unsigned r;
  for(r = 0 ; r < cfd->charheight / 2 ; ++r){
    *glyph = 0xff;
    ++glyph;
  }
  while(r < cfd->charheight){
    *glyph = 0;
    ++glyph;
    ++r;
  }
  return 0;
}

static int
shim_lower_half_block(struct consolefontdesc* cfd, unsigned idx){
  if(idx >= cfd->charcount){
    return -1;
  }
  unsigned char* glyph = (unsigned char*)cfd->chardata + 32 * idx;
  unsigned r;
  for(r = 0 ; r < cfd->charheight / 2 ; ++r){
    *glyph = 0;
    ++glyph;
  }
  while(r < cfd->charheight){
    *glyph = 0xff;
    ++glyph;
    ++r;
  }
  return 0;
}

// force upper and lower half blocks into the console font, returning the font
// positions in |*upper| and |*lower|.
static int
jam_linux_consolefont(int fd, unsigned showglyphs, unsigned* upper, unsigned* lower){
  struct consolefontdesc cfd = {};
  cfd.charcount = 512;
  cfd.chardata = malloc(32 * cfd.charcount);
  if(cfd.chardata == NULL){
    return -1;
  }
  if(ioctl(fd, GIO_FONTX, &cfd)){
    fprintf(stderr, "Error reading Linux kernelfont (%s)\n", strerror(errno));
    free(cfd.chardata);
    return -1;
  }
  printf("Kernel font size (glyphcount): %hu\n", cfd.charcount);
  printf("Kernel font character height: %hu\n", cfd.charheight);
  if(cfd.charcount > 512){
    fprintf(stderr, "Warning: kernel returned excess charcount\n");
    free(cfd.chardata);
    return -1;
  }
  *upper = cfd.charcount - 2;
  *lower = cfd.charcount - 1;
  // FIXME find best place. could be whatever the fewest unicodes map to, or
  // something similar to our target, or who knows...
  if(shim_upper_half_block(&cfd, *upper) || shim_lower_half_block(&cfd, *lower)){
    fprintf(stderr, "Failed to shim font\n");
    free(cfd.chardata);
    return -1;
  }
  if(ioctl(fd, PIO_FONTX, &cfd)){
    fprintf(stderr, "Failed to set font (%s)\n", strerror(errno));
    free(cfd.chardata);
    return -1;
  }
  if(showglyphs){
    for(unsigned i = 0 ; i < cfd.charcount ; i += 7){
      const unsigned char* g1 = (unsigned char*)cfd.chardata + 32 * i;
      const unsigned char* g2 = g1 + 32;
      const unsigned char* g3 = g2 + 32;
      const unsigned char* g4 = g3 + 32;
      const unsigned char* g5 = g4 + 32;
      const unsigned char* g6 = g5 + 32;
      const unsigned char* g7 = g6 + 32;
      for(unsigned row = 0 ; row < cfd.charheight ; ++row){
        explode_glyph_row(&g1);
        if(i < cfd.charcount - 1u){
          explode_glyph_row(&g2);
          if(i < cfd.charcount - 2u){
            explode_glyph_row(&g3);
            if(i < cfd.charcount - 3u){
              explode_glyph_row(&g4);
              if(i < cfd.charcount - 4u){
              explode_glyph_row(&g5);
                if(i < cfd.charcount - 5u){
                  explode_glyph_row(&g6);
                  if(i < cfd.charcount - 6u){
                    explode_glyph_row(&g7);
                  }
                }
              }
            }
          }
        }
        printf("\n");
      }
    }
  }
  free(cfd.chardata);
  return 0;
}

static int
jam_linux_consolemap(int fd, unsigned upper, unsigned lower){
  struct unimapdesc map = {};
  map.entry_ct = USHRT_MAX;
  map.entries = malloc(map.entry_ct * sizeof(struct unipair));
  if(ioctl(fd, GIO_UNIMAP, &map)){
    fprintf(stderr, "Error reading Unicode->font map (%s)\n", strerror(errno));
    free(map.entries);
    return -1;
  }
  printf("Unicode->font entries: %hu\n", map.entry_ct);
  unsigned found_up = 0, found_low = 0;
  for(int i = 0 ; i < map.entry_ct ; ++i){
    if(map.entries[i].unicode == 0x2580){ // upper half block
      map.entries[i].fontpos = upper;
      found_up = 1;
    }else if(map.entries[i].unicode == 0x2584){ // lower half block
      map.entries[i].fontpos = lower;
      found_low = 1;
    }
  }
  const int need_insert = !found_up + !found_low;
  if(map.entry_ct > USHRT_MAX - need_insert){
    fprintf(stderr, "Too many damn entries (need %d)!\n", need_insert);
    free(map.entries);
    return -1;
  }
  if(!found_up){
    map.entries[map.entry_ct].unicode = 0x2580;
    map.entries[map.entry_ct].fontpos = upper;
    ++map.entry_ct;
  }
  if(!found_low){
    map.entries[map.entry_ct].unicode = 0x2584;
    map.entries[map.entry_ct].fontpos = lower;
    ++map.entry_ct;
  }
  if(ioctl(fd, PIO_UNIMAP, &map)){
    fprintf(stderr, "Error writing Unicode->font map (%s)\n", strerror(errno));
    free(map.entries);
    return -1;
  }
  for(int i = 0 ; i < map.entry_ct ; ++i){
    wchar_t w = map.entries[i].unicode;
    printf(" %05hx (%lc)->%3hu ", map.entries[i].unicode,
           (wint_t)(iswprint(w) ? w : L' '), map.entries[i].fontpos);
    if(i % 4 == 3){
      printf("\n");
    }
  }
  free(map.entries);
  if(map.entry_ct % 4){
    printf("\n");
  }
  return 0;
}

// the objective is to reprogram the linux console font and unicode mapping
// so that they include a half block suitable for ncvisuals
int main(int argc, char** argv){
  if(setlocale(LC_ALL, "") == NULL){
    fprintf(stderr, "Error setting locale\n");
    return EXIT_FAILURE;
  }
  if(argc > 2){
    usage(argv[0]);
    return EXIT_FAILURE;
  }
  int fd = get_tty_fd(argv[1]);
  if(fd < 0){
    return EXIT_FAILURE;
  }
  if(!is_linux_console(fd)){
    return EXIT_FAILURE;
  }
  int r = -1;
  unsigned upper, lower;
  if(jam_linux_consolefont(fd, true, &upper, &lower) == 0){
    if(jam_linux_consolemap(fd, upper, lower) == 0){
      r = 0;
    }
  }
  if(close(fd)){
    fprintf(stderr, "Error closing %d (%s)\n", fd, strerror(errno));
  }
  return r ? EXIT_FAILURE : EXIT_SUCCESS;
}
#else
int main(void){
  fprintf(stderr, "This is a Linux-only program\n");
  return EXIT_FAILURE;
}
#endif

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

static inline size_t
row_bytes(const struct console_font_op* cfo){
  return (cfo->width + 7) / 8;
}

static inline size_t
Bper(const struct console_font_op* cfo){
  size_t minb = row_bytes(cfo) * cfo->height;
  return (minb + 31) / 32 * 32;
}

static unsigned char*
get_glyph(struct console_font_op* cfo, unsigned idx){
  if(idx >= cfo->charcount){
    return NULL;
  }
  return (unsigned char*)cfo->data + Bper(cfo) * idx;
}

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

// each row is laid out as bytes, one bit per pixel, rounded up to the
// lowest sufficient number of bytes. each character is thus
//
//  height * rowbytes, where rowbytes = width + 7 / 8
static int
explode_glyph_row(unsigned char** row, unsigned width){
  unsigned char mask = 0x80;
  while(width--){
    printf("%s", **row & mask ? "*" : " ");
    if((mask >>= 1) == 0 && width){
      mask = 0x80;
      ++*row;
    }
  }
  printf(" ");
  ++*row;
  return 0;
}

// idx is the glyph index within cfo->data. qbits are the occupied quadrants:
//  0x8 = upper left
//  0x4 = upper right
//  0x2 = lower left
//  0x1 = lower right
static int
shim_quad_block(struct console_font_op* cfo, unsigned idx, unsigned qbits){
fprintf(stderr, "REWRITING %u with 0x%01x\n", idx, qbits);
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
fprintf(stderr, "*");
      }
else fprintf(stderr, " ");
      if((mask >>= 1) == 0){
        mask = 0x80;
        *++row = 0;
      }
    }
    while(x < cfo->width){
      if(qbits & 0x4){
        *row |= mask;
fprintf(stderr, "*");
      }
else fprintf(stderr, " ");
      if((mask >>= 1) == 0){
        mask = 0x80;
        *++row = 0;
      }
      ++x;
    }
fprintf(stderr, "\n");
  }
  while(r < cfo->height){
    unsigned char mask = 0x80;
    unsigned char* row = glyph + row_bytes(cfo) * r;
    unsigned x;
    *row = 0;
    for(x = 0 ; x < cfo->width / 2 ; ++x){
      if(qbits & 0x2){
        *row |= mask;
fprintf(stderr, "*");
      }
else fprintf(stderr, " ");
      if((mask >>= 1) == 0){
        mask = 0x80;
        *++row = 0;
      }
    }
    while(x < cfo->width){
      if(qbits & 0x1){
        *row |= mask;
fprintf(stderr, "*");
      }
else fprintf(stderr, " ");
      if((mask >>= 1) == 0){
        mask = 0x80;
        *++row = 0;
      }
      ++x;
    }
fprintf(stderr, "\n");
    ++r;
  }
  return 0;
}

// force upper and lower half blocks into the console font, returning the font
// positions in |*upper| and |*lower|.
static int
jam_linux_consolefont(int fd, unsigned showglyphs, unsigned* upper, unsigned* lower){
  struct console_font_op cfo = {0};
  cfo.op = KD_FONT_OP_GET;
  cfo.charcount = 512;
  cfo.data = malloc(128 * cfo.charcount);
  cfo.width = 32;
  cfo.height = 32;
  if(cfo.data == NULL){
    return -1;
  }
  if(ioctl(fd, KDFONTOP, &cfo)){
    fprintf(stderr, "Error reading Linux kernelfont (%s)\n", strerror(errno));
    free(cfo.data);
    return -1;
  }
  printf("Kernel font size (glyphcount): %hu\n", cfo.charcount);
  printf("Kernel font character geometry: %hux%hu\n", cfo.height, cfo.width);
  if(cfo.charcount > 512){
    fprintf(stderr, "Warning: kernel returned excess charcount\n");
    free(cfo.data);
    return -1;
  }
  *upper = cfo.charcount - 2;
  *lower = cfo.charcount - 1;
  // FIXME find best place. could be whatever the fewest unicodes map to, or
  // something similar to our target, or who knows...
  if(shim_quad_block(&cfo, *upper, 0xc) || shim_quad_block(&cfo, *lower, 0x3)){
    fprintf(stderr, "Failed to shim font\n");
    free(cfo.data);
    return -1;
  }
  cfo.op = KD_FONT_OP_SET;
  if(ioctl(fd, KDFONTOP, &cfo)){
    fprintf(stderr, "Failed to set font (%s)\n", strerror(errno));
    free(cfo.data);
    return -1;
  }
  if(showglyphs){
    // FIXME get real screen width
    const int atonce = 80 / (cfo.width + 1);
    unsigned char** g = malloc(sizeof(*g) * atonce);
    for(unsigned i = 0 ; i < cfo.charcount ; i += atonce){
      for(int o = 0 ; o < atonce ; ++o){
        g[o] = (unsigned char*)cfo.data + Bper(&cfo) * (i + o);
fprintf(stderr, "Bper: %zu %d at %p\n", Bper(&cfo), i + o, g[o]);
      }
      for(unsigned row = 0 ; row < cfo.height ; ++row){
        for(int o = 0 ; o < atonce ; ++o){
          explode_glyph_row(&g[o], cfo.width);
        }
        printf("\n");
      }
    }
    free(g);
  }
  free(cfo.data);
  return 0;
}

static int
jam_linux_consolemap(int fd, unsigned upper, unsigned lower){
  struct unimapdesc map = {0};
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

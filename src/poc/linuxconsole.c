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

static int
get_linux_colormap(int fd){
  unsigned char cmap[48];
  if(ioctl(fd, GIO_CMAP, cmap)){
    fprintf(stderr, "Error reading Linux colormap (%s)\n", strerror(errno));
    return -1;
  }
  for(size_t i = 0 ; i < sizeof(cmap) / 12 ; ++i){
    const int c1 = i * 4;
    const int c2 = c1 + 1;
    const int c3 = c2 + 1;
    const int c4 = c3 + 1;
    printf(" |%02d| %3u %3u %3u |%02d| %3u %3u %3u |%02d| %3u %3u %3u |%02d| %3u %3u %3u\n",
           c1, cmap[c1 * 3], cmap[c1 * 3 + 1], cmap[c1 * 3 + 2],
           c2, cmap[c2 * 3], cmap[c2 * 3 + 1], cmap[c2 * 3 + 2],
           c3, cmap[c3 * 3], cmap[c3 * 3 + 1], cmap[c3 * 3 + 2],
           c4, cmap[c4 * 3], cmap[c4 * 3 + 1], cmap[c4 * 3 + 2]);
  }
  return 0;
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
get_linux_consolefont(int fd, unsigned showglyphs){
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
get_linux_consolemap(int fd){
  struct unimapdesc map = {};
  map.entry_ct = USHRT_MAX;
  map.entries = malloc(map.entry_ct * sizeof(struct unipair));
  if(ioctl(fd, GIO_UNIMAP, &map)){
    fprintf(stderr, "Error reading Unicode->font map (%s)\n", strerror(errno));
    free(map.entries);
    return -1;
  }
  printf("Unicode->font entries: %hu\n", map.entry_ct);
  for(int i = 0 ; i < map.entry_ct ; ++i){
    wint_t w = map.entries[i].unicode;
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

static int
get_linux_unimap(int fd){
  unsigned short map[E_TABSZ];
  if(ioctl(fd, GIO_UNISCRNMAP, map)){
    fprintf(stderr, "Error reading Unicode->screen map (%s)\n", strerror(errno));
    return -1;
  }
  printf("Unicode->screen entries: %d\n", E_TABSZ);
  int standard = 0;
  for(size_t i = 0 ; i < sizeof(map) / sizeof(*map) ; ++i){
    if(map[i] != 0xf000 + i){
      printf(" |%03zu| %hx\n", i, map[i]);
    }else{
      ++standard;
    }
  }
  printf(" %d/%d direct-to-font mapping%s\n", standard,
         E_TABSZ, standard == 1 ? "" : "s");
  return 0;
}

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
  int r = 0;
  r |= get_linux_colormap(fd);
  r |= get_linux_consolefont(fd, true);
  r |= get_linux_consolemap(fd);
  r |= get_linux_unimap(fd);
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

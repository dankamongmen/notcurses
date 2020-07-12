#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
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
  int mode, r;
  if( (r = ioctl(fd, KDGETMODE, &mode)) ){
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
    fprintf(stderr, " |%02d| %3u %3u %3u |%02d| %3u %3u %3u |%02d| %3u %3u %3u |%02d| %3u %3u %3u\n",
            c1, cmap[c1 * 3], cmap[c1 * 3 + 1], cmap[c1 * 3 + 2],
            c2, cmap[c2 * 3], cmap[c2 * 3 + 1], cmap[c2 * 3 + 2],
            c3, cmap[c3 * 3], cmap[c3 * 3 + 1], cmap[c3 * 3 + 2],
            c4, cmap[c4 * 3], cmap[c4 * 3 + 1], cmap[c4 * 3 + 2]);
  }
  return 0;
}

static int
get_linux_consolefont(int fd){
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
  // FIXME
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
    printf("%05hx->%3hu ", map.entries[i].unicode, map.entries[i].fontpos);
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

int main(int argc, char** argv){
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
  r |= get_linux_consolefont(fd);
  r |= get_linux_consolemap(fd);
  if(close(fd)){
    fprintf(stderr, "Error closing %d (%s)\n", fd, strerror(errno));
  }
  return r ? EXIT_FAILURE : EXIT_SUCCESS;
}

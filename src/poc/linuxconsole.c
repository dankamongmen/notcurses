#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
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
  printf("Verified Linux console, mode %d\n", mode);
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
    fprintf(stderr, "|%02d| %3u %3u %3u |%02d| %3u %3u %3u |%02d| %3u %3u %3u |%02d| %3u %3u %3u\n",
            c1, cmap[c1 * 3], cmap[c1 * 3 + 1], cmap[c1 * 3 + 2],
            c2, cmap[c2 * 3], cmap[c2 * 3 + 1], cmap[c2 * 3 + 2],
            c3, cmap[c3 * 3], cmap[c3 * 3 + 1], cmap[c3 * 3 + 2],
            c4, cmap[c4 * 3], cmap[c4 * 3 + 1], cmap[c4 * 3 + 2]);
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
  get_linux_colormap(fd);
  if(close(fd)){
    fprintf(stderr, "Error closing %d (%s)\n", fd, strerror(errno));
  }
  return EXIT_SUCCESS;
}

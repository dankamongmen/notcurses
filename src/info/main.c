#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <notcurses/notcurses.h>

// write(2) with retry on partial write or interrupted write
static inline ssize_t
writen(int fd, const void* buf, size_t len){
  ssize_t r;
  size_t w = 0;
  while(w < len){
    if((r = write(fd, (const char*)buf + w, len - w)) < 0){
      if(errno == EAGAIN || errno == EBUSY || errno == EINTR){
        continue;
      }
      return -1;
    }
    w += r;
  }
  return w;
}

int main(void){
  char* mbuf = NULL;
  size_t len = 0;
  FILE* mstream;
  if((mstream = open_memstream(&mbuf, &len)) == NULL){
    return EXIT_FAILURE;
  }
  if(fprintf(mstream, "\n") != 1){
    return EXIT_FAILURE;
  }
  notcurses_options nopts = {
    .flags = NCOPTION_NO_ALTERNATE_SCREEN,
  };
  struct notcurses* nc = notcurses_init(&nopts, NULL);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  notcurses_debug_caps(nc, mstream);
  if(fclose(mstream)){
    notcurses_stop(nc);
    fprintf(stderr, "Error closing memstream after %zuB\n", len);
    return EXIT_FAILURE;
  }
  if(writen(fileno(stdout), mbuf, len) < 0){
    notcurses_stop(nc);
    fprintf(stderr, "Error writing %zuB memstream\n", len);
    return EXIT_FAILURE;
  }
  free(mbuf);
  return notcurses_stop(nc) ? EXIT_FAILURE : EXIT_SUCCESS;
}

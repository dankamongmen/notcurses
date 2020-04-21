#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <notcurses/notcurses.h>

static bool fddone;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static int
cb(struct ncfdplane* ncfd, const void* data, size_t len, void* curry){
  int ret = -1;
  if(ncplane_putstr(ncfdplane_plane(ncfd), data) >= 0){
    if(!notcurses_render(ncplane_notcurses(ncfdplane_plane(ncfd)))){
      ret = 0;
    }
  }
  (void)len;
  (void)curry;
  return ret;
}

static int
eofcb(struct ncfdplane* ncfd, int nerrno, void* curry){
  (void)nerrno;
  (void)curry;
  pthread_mutex_lock(&lock);
  fddone = true;
  pthread_mutex_unlock(&lock);
  pthread_cond_signal(&cond);
  return ncfdplane_destroy(ncfd);
}

int main(int argc, char** argv){
  setlocale(LC_ALL, "");
  notcurses_options opts = {};
  opts.inhibit_alternate_screen = true;
  struct notcurses* nc = notcurses_init(&opts, stdout);
  struct ncplane* n = notcurses_stdplane(nc);
  int ret = -1;
  while(*++argv){
    int fd = open(*argv, O_RDONLY|O_CLOEXEC);
    if(fd < 0){
      fprintf(stderr, "Couldn't open %s (%s)\n", *argv, strerror(errno));
      goto done;
    }
    ncfdplane_options nopts = {};
    struct ncfdplane* ncfp = ncfdplane_create(n, &nopts, fd, cb, eofcb);
    pthread_mutex_lock(&lock);
    while(!fddone){
      pthread_cond_wait(&cond, &lock);
    }
    fddone = false;
    pthread_mutex_unlock(&lock);
  }

done:
  if(notcurses_stop(nc) || ret){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

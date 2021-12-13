#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <compat/compat.h>
#include <notcurses/notcurses.h>

static bool fddone;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static int
cb(struct ncfdplane* ncfd, const void* data, size_t len, void* curry){
  int ret = -1;
  if(ncplane_putnstr(ncfdplane_plane(ncfd), len, data) >= 0){
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
  (void)ncfd;
  (void)nerrno;
  (void)curry;
  pthread_mutex_lock(&lock);
  fddone = true;
  pthread_mutex_unlock(&lock);
  pthread_cond_signal(&cond);
  return nerrno;
}

int main(int argc, char** argv){
  if(argc < 2){
    fprintf(stderr, "usage: fileroller file [ files...]\n");
    return EXIT_FAILURE;
  }
  setlocale(LC_ALL, "");
  notcurses_options opts = {
    .flags = NCOPTION_INHIBIT_SETLOCALE
             | NCOPTION_SUPPRESS_BANNERS
             | NCOPTION_DRAIN_INPUT,
  };
  struct notcurses* nc = notcurses_core_init(&opts, NULL);
  struct ncplane* n = notcurses_stdplane(nc);
  ncplane_set_scrolling(n, true);
  while(*++argv){
    int fd = open(*argv, O_RDONLY|O_CLOEXEC);
    if(fd < 0){
      fprintf(stderr, "Couldn't open %s (%s)\n", *argv, strerror(errno));
      goto done;
    }
    ncfdplane_options nopts = {0};
    struct ncfdplane* ncfp = ncfdplane_create(n, &nopts, fd, cb, eofcb);
    pthread_mutex_lock(&lock);
    while(!fddone){
      pthread_cond_wait(&cond, &lock);
    }
    fddone = false;
    pthread_mutex_unlock(&lock);
    if(ncfdplane_destroy(ncfp)){
      notcurses_stop(nc);
      return EXIT_FAILURE;
    }
    ncplane_set_fg_rgb(n, 0x00bcaa);
    ncplane_printf_aligned(n, -1, NCALIGN_CENTER, "press any key to continue (%s)", *argv);
    notcurses_render(nc);
    notcurses_get(nc, NULL, NULL);
  }

done:
  if(notcurses_stop(nc)){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

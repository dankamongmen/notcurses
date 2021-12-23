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
  ncplane_set_scrolling(ncfdplane_plane(ncfd), true);
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
    fprintf(stderr, "usage: %s binary [ args... ]\n", *argv);
    return EXIT_FAILURE;
  }
  setlocale(LC_ALL, "");
  notcurses_options opts = {
    .flags = NCOPTION_INHIBIT_SETLOCALE
              | NCOPTION_SUPPRESS_BANNERS
              | NCOPTION_DRAIN_INPUT,
  };
  struct notcurses* nc = notcurses_core_init(&opts, NULL);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  ++argv;
  struct ncplane* std = notcurses_stdplane(nc);
  ncplane_set_scrolling(std, true);
  ncsubproc_options nopts = {0};
  struct ncsubproc* nsproc = ncsubproc_createvp(std, &nopts, *argv,
                              (const char* const*)argv, cb, eofcb);
  if(nsproc == NULL){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  pthread_mutex_lock(&lock);
  while(!fddone){
    pthread_cond_wait(&cond, &lock);
  }
  pthread_mutex_unlock(&lock);
  notcurses_render(nc);
  if(ncsubproc_destroy(nsproc)){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  ncplane_set_fg_rgb(std, 0x00bcaa);
  ncplane_printf_aligned(std, -1, NCALIGN_CENTER, "press any key to continue (%s)", *argv);
  notcurses_render(nc);
  ncinput ni;
  do{
    notcurses_get_blocking(nc, &ni);
  }while(ni.evtype == NCTYPE_RELEASE);
  if(notcurses_stop(nc)){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

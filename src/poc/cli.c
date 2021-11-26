#include <poll.h>
#include <notcurses/notcurses.h>

int main(void){
  struct notcurses_options nopts = {
    .flags = NCOPTION_PRESERVE_CURSOR |
             NCOPTION_NO_CLEAR_BITMAPS |
             NCOPTION_NO_ALTERNATE_SCREEN,
  };
  struct notcurses* nc = notcurses_init(&nopts, NULL);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  struct ncplane* stdn = notcurses_stdplane(nc);
  ncplane_set_scrolling(stdn, true);
  ncinput ni;
  int fd = notcurses_inputready_fd(nc);
  do{
    if(ncplane_putstr(stdn, "press any key\n") < 0){
      goto err;
    }
    if(notcurses_render(nc)){
      goto err;
    }
    struct pollfd pfd = {
      .fd = fd,
      .events = POLLIN,
    };
    while(poll(&pfd, 1, -1) <= 0){
    }
    notcurses_get_blocking(nc, &ni);
  }while(ni.evtype == NCTYPE_RELEASE || ni.id != 'q');
  if(notcurses_render(nc)){
    goto err;
  }
  notcurses_stop(nc);
  return EXIT_SUCCESS;

err:
  notcurses_stop(nc);
  return EXIT_FAILURE;
}

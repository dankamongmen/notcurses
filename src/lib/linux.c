#include "internal.h"

#ifdef __linux__
#include <linux/kd.h>
#include <sys/ioctl.h>

// is the provided fd a Linux console?
bool is_linux_console(const notcurses* nc){
  int mode, r;
  if( (r = ioctl(nc->ttyfd, KDGETMODE, &mode)) ){
    logdebug(nc, "Not a Linux console, KDGETMODE failed\n");
    return false;
  }
  loginfo(nc, "Verified Linux console, mode %d\n", mode);
  return true;
}
#else
bool is_linux_console(const notcurses* nc){
  (void)nc;
  return false;
}
#endif

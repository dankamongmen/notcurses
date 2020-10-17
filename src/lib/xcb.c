#include "builddef.h"
#include "internal.h"
#ifdef USE_XCB
int x_connect(notcurses* nc){
  const char* disp = getenv("DISPLAY");
  if(disp == NULL){
    return -1;
  }
  nc->xcb = xcb_connect(NULL, NULL);
  if(nc->xcb == NULL){
    logwarn(nc, "Couldn't connect to X.Org at [%s]\n", disp);
    return -1;
  }
  return 0;
}

int x_disconnect(notcurses* nc){
  if(nc->xcb){
    xcb_disconnect(nc->xcb);
    nc->xcb = NULL;
  }
  return 0;
}
#else
int x_connect(notcurses* nc){
  nc->xcb = NULL;
  return -1;
}

int x_disconnect(notcurses* nc){
  nc->xcb = NULL;
  return 0;
}
#endif

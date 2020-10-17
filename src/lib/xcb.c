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
  if(xcb_connection_has_error(nc->xcb)){
    logwarn(nc, "xcb connection failed\n");
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

char* notcurses_title(const notcurses* nc){
  const xcb_window_t window = xcb_generate_id(nc->xcb);
  xcb_get_property_cookie_t cookie = xcb_get_property(nc->xcb, 0, window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 0, 0);
  xcb_get_property_reply_t *reply = xcb_get_property_reply(nc->xcb, cookie, NULL);
  if(reply == NULL){
    return NULL;
  }
  int len = xcb_get_property_value_length(reply);
  if(len == 0){
    free(reply);
    return NULL;
  }
  char* ret = strdup(xcb_get_property_value(reply));
  free(reply);
  return ret;
}

int notcurses_set_title(notcurses* nc, const char* title){
  const xcb_window_t window = xcb_generate_id(nc->xcb);
  return 0;
}
#else
int x_connect(notcurses* nc){
  nc->xcb = NULL;
  return -1;
}

char* notcurses_title(const notcurses* nc){
  (void)nc;
  return NULL;
}

int notcurses_set_title(notcurses* nc, const char* title){
  (void)nc;
  (void)title;
  return -1;
}

int x_disconnect(notcurses* nc){
  nc->xcb = NULL;
  return -1;
}
#endif

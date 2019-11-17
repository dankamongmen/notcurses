#include "notcurses.h"
#include "version.h"

static const char NOTCURSES_VERSION[] =
 notcurses_VERSION_MAJOR "."
 notcurses_VERSION_MINOR "."
 notcurses_VERSION_PATCH;

const char* notcurses_version(void){
  return NOTCURSES_VERSION;
}

int notcurses_init(void){
  int ret = 0;
  return ret;
}

int notcurses_stop(void){
  int ret = 0;
  return ret;
}

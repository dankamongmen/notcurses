#include "internal.h"
#ifdef USE_GPM
#undef buttons // defined by both term.h and gpm.h, ugh
#include <lib/gpm.h>
#include <gpm.h>

static Gpm_Connect gpmconn;    // gpm server handle

int gpm_connect(tinfo* ti){
  (void)ti;
  gpm_zerobased = 1;
  // get all of _MOVE, _DRAG, _DOWN, and _UP
  gpmconn.eventMask = ~0;
  gpmconn.defaultMask = 0;
  gpmconn.minMod = 0;
  gpmconn.maxMod = 0; // allow shift+drag to be used for direct copy+paste
  if(Gpm_Open(&gpmconn, 0) == -1){
    logerror("couldn't connect to gpm");
    return -1;
  }
  loginfo("connected to gpm on %d\n", gpm_fd);
  return gpm_fd;
}

int gpm_read(tinfo* ti, ncinput* ni){
  (void)ti;
  (void)ni;
  return -1;
}

int gpm_close(tinfo* ti){
  (void)ti;
  Gpm_Close();
  memset(&gpmconn, 0, sizeof(gpmconn));
  return 0;
}
#else
int gpm_connect(tinfo* ti){
  (void)ti;
  return -1;
}

int gpm_read(tinfo* ti, ncinput* ni){
  (void)ti;
  (void)ni;
  return -1;
}

int gpm_close(tinfo* ti){
  (void)ti;
  return -1;
}
#endif

#include "internal.h"
#ifdef USE_GPM
#undef buttons // defined by both term.h and gpm.h, ugh
#include <lib/gpm.h>
#include <gpm.h>

static Gpm_Connect gpmconn;    // gpm server handle

static void*
gpmwatcher(void* vti){
  tinfo* ti = vti;
  (void)ti; // FIXME
  Gpm_Event gev;
  while(true){
    if(Gpm_GetEvent(&gev)){
      logerror("error reading from gpm daemon\n");
      break;
    }
    loginfo("got gpm event\n");
    // FIXME
  }
  return NULL;
}

int gpm_connect(tinfo* ti){
  (void)ti;
  gpm_zerobased = 1;
  // get all of _MOVE, _DRAG, _DOWN, and _UP
  gpmconn.eventMask = ~0;
  gpmconn.defaultMask = 0;
  gpmconn.minMod = 0;
  gpmconn.maxMod = 0; // allow shift+drag to be used for direct copy+paste
  if(Gpm_Open(&gpmconn, 0) == -1){
    logerror("couldn't connect to gpm\n");
    return -1;
  }
  if(pthread_create(&ti->gpmthread, NULL, gpmwatcher, ti)){
    logerror("couldn't spawn gpm thread\n");
    Gpm_Close();
    memset(&gpmconn, 0, sizeof(gpmconn));
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
  if(pthread_cancel(ti->gpmthread)){
    logerror("couldn't cancel gpm thread\n"); // daemon might have died
  }
  void* thrres;
  if(pthread_join(ti->gpmthread, &thrres)){
    logerror("error joining gpm thread\n");
  }
  Gpm_Close();
  memset(&gpmconn, 0, sizeof(gpmconn));
  return 0;
}

const char* gpm_version(void){
  return Gpm_GetLibVersion(NULL);
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

const char* gpm_version(void){
  return "n/a";
}
#endif

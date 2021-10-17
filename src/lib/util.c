#ifndef __MINGW64__
#include <pwd.h>
#include <unistd.h>
#else
#include <winsock2.h>
#define SECURITY_WIN32
#include <secext.h>
#endif
#include "internal.h"

int set_loglevel_from_env(ncloglevel_e* llptr){
  const char* ll = getenv("NOTCURSES_LOGLEVEL");
  if(ll == NULL){
    return 0;
  }
  char* endl;
  long l = strtol(ll, &endl, 10);
  if(l < NCLOGLEVEL_PANIC || l > NCLOGLEVEL_TRACE){
    logpanic("Illegal NOTCURSES_LOGLEVEL: %s\n", ll);
    return -1;
  }
  *llptr = l;
  loginfo("Got loglevel from environment: %ld\n", l);
  return 0;
}

char* notcurses_accountname(void){
#ifndef __MINGW64__
  const char* un;
  if( (un = getenv("LOGNAME")) ){
    return strdup(un);
  }
  uid_t uid = getuid();
  struct passwd* p = getpwuid(uid);
  if(p == NULL){
    return NULL;
  }
  return strdup(p->pw_name);
#else
  DWORD unlen = UNLEN + 1;
  char* un = malloc(unlen);
  if(un == NULL){
    return NULL;
  }
  if(!GetUserNameExA(NameSamCompatible, un, &unlen)){
    logerror("couldn't get user name\n");
    free(un);
    return NULL;
  }
  return un;
#endif
}

char* notcurses_hostname(void){
#ifndef __MINGW64__
  char hostname[_POSIX_HOST_NAME_MAX + 1];
  if(gethostname(hostname, sizeof(hostname)) == 0){
    char* fqdn = strchr(hostname, '.');
    if(fqdn){
      *fqdn = '\0';
    }
    return strdup(hostname);
  }
#else // windows
  char lp[MAX_COMPUTERNAME_LENGTH + 1];
  DWORD s = sizeof(lp);
  if(GetComputerNameA(lp, &s)){
    return strdup(lp);
  }
#endif
  return NULL;
}

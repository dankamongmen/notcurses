#ifndef __MINGW64__
#include <pwd.h>
#include <unistd.h>
#else
#include <winsock2.h>
#endif
#include "internal.h"

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
  if(GetUserName(un, &unlen)){ // FIXME probably want GetUserNameEx
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
  LPDWORD s = sizeof(lp);
  if(GetComputerNameA(lp, &s)){
    return strdup(lp);
  }
#endif
  return NULL;
}

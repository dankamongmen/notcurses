#include "termdesc.h"
#include "internal.h"
#ifdef __MINGW64__
#include "windows.h"

// ti has been memset to all zeroes. windows configuration is static.
int prepare_windows_terminal(tinfo* ti, size_t* tablelen, size_t* tableused){
  HANDLE in = GetStdHandle(STD_INPUT_HANDLE);
  if(in == INVALID_HANDLE_VALUE){
    logerror("couldn't get input handle\n");
    return -1;
  }
  // if we're a true Windows Terminal, SetConsoleMode() ought succeed.
  // otherwise, we're something else; go ahead and query.
  if(!SetConsoleMode(in, ENABLE_MOUSE_INPUT
                         | ENABLE_VIRTUAL_TERMINAL_INPUT
                         | ENABLE_PROCESSED_INPUT
                         | ENABLE_WINDOW_INPUT)){
    logerror("couldn't set input console mode\n");
    return -1;
  }
  const struct wtermdesc {
    escape_e esc;
    const char* tinfo;
  } wterms[] = {
    { ESCAPE_CUP,   "\x1b[%i%p1%d;%p2%dH", },
    { ESCAPE_RMKX,  "\x1b[?1l", },
    { ESCAPE_SMKX,  "\x1b[?1h", },
    { ESCAPE_VPA,   "\x1b[%i%p1%dd", },
    { ESCAPE_HPA,   "\x1b[%i%p1%dG", },
    { ESCAPE_SC,    "\x1b[s", },
    { ESCAPE_RC,    "\x1b[u", },
    { ESCAPE_CLEAR, "\x1b[2J", },
    { ESCAPE_SMCUP, "\x1b[?1049h", },
    { ESCAPE_RMCUP, "\x1b[?1049l", },
    { ESCAPE_SETAF, "\x1b[38;5;%i%p1%dm", },
    { ESCAPE_SETAB, "\x1b[48;5;%i%p1%dm", },
    { ESCAPE_OP,    "\x1b[39;49m", },
    { ESCAPE_CIVIS, "\x1b[?25l", },
    { ESCAPE_CNORM, "\x1b[?25h", },
    { ESCAPE_U7,    "\x1b[6n", },
    { ESCAPE_BOLD,  "\x1b[1m", },
    { ESCAPE_SITM,  "\x1b[3m", },
    { ESCAPE_RITM,  "\x1b[23m", },
    { ESCAPE_SMUL,  "\x1b[4m", },
    { ESCAPE_RMUL,  "\x1b[24m", },
    { ESCAPE_SMULX, "\x1b[4:3m", },
    { ESCAPE_SMULNOX,"\x1b[4:0m", },
    { ESCAPE_SGR0,  "\x1b[0m", },
    { ESCAPE_MAX, NULL, }
  }, *w; 
  for(w = wterms ; w->tinfo; ++w){
    if(grow_esc_table(ti, w->tinfo, w->esc, tablelen, tableused)){
      return -1;
    }
  }
  ti->caps.rgb = true;
  ti->caps.colors = 256;
  ti->caps.quadrants = true;
  ti->caps.braille = true;
  ti->termname = "Windows Terminal";
  loginfo("prepared Windows Terminal\n");
  return 0;
}
#endif

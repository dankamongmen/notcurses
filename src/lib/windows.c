#include "termdesc.h"
#include "internal.h"
#include "windows.h"
#ifdef __MINGW64__
// ti has been memset to all zeroes. windows configuration is static.
int prepare_windows_terminal(tinfo* ti, size_t* tablelen, size_t* tableused){
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
  ti->inhandle = GetStdHandle(STD_INPUT_HANDLE);
  ti->outhandle = GetStdHandle(STD_OUTPUT_HANDLE);
  if(ti->inhandle == INVALID_HANDLE_VALUE){
    logerror("couldn't get input handle\n");
    return -1;
  }
  if(ti->outhandle == INVALID_HANDLE_VALUE){
    logerror("couldn't get output handle\n");
    return -1;
  }
  if(!SetConsoleOutputCP(65001)){
    logerror("couldn't set output page to utf8\n");
    return -1;
  }
  if(!SetConsoleCP(65001)){
    logerror("couldn't set input page to utf8\n");
    return -1;
  }
  DWORD inmode;
  if(!GetConsoleMode(ti->inhandle, &inmode)){
    logerror("couldn't get input console mode\n");
    return -1;
  }
  // we don't explicitly disable ENABLE_ECHO_INPUT and ENABLE_LINE_INPUT
  // yet; those are handled in cbreak_mode(). just get ENABLE_INSERT_MODE.
  inmode &= ~ENABLE_INSERT_MODE;
  inmode |= ENABLE_MOUSE_INPUT | ENABLE_PROCESSED_INPUT
            | ENABLE_QUICK_EDIT_MODE | ENABLE_EXTENDED_FLAGS
            | ENABLE_WINDOW_INPUT | ENABLE_VIRTUAL_TERMINAL_INPUT;
  if(!SetConsoleMode(ti->inhandle, inmode)){
    logerror("couldn't set input console mode\n");
    return -1;
  }
  // if we're a true Windows Terminal, SetConsoleMode() ought succeed.
  // otherwise, we're something else; go ahead and try.
  if(!SetConsoleMode(ti->outhandle, ENABLE_PROCESSED_OUTPUT
                     | ENABLE_WRAP_AT_EOL_OUTPUT
                     | ENABLE_VIRTUAL_TERMINAL_PROCESSING
                     | DISABLE_NEWLINE_AUTO_RETURN
                     | ENABLE_LVB_GRID_WORLDWIDE)){
    logerror("couldn't set output console mode\n");
    return -1;
  }
  ti->termname = "Windows Terminal";
  loginfo("verified Windows Terminal\n");
  return 0;
}
#endif

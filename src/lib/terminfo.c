#include <ncurses.h> // needed for some definitions, see terminfo(3ncurses)
#include "internal.h"

static bool
query_rgb(void){
  bool rgb = tigetflag("RGB") == 1;
  if(!rgb){
    // RGB terminfo capability being a new thing (as of ncurses 6.1), it's not commonly found in
    // terminal entries today. COLORTERM, however, is a de-facto (if imperfect/kludgy) standard way
    // of indicating TrueColor support for a terminal. The variable takes one of two case-sensitive
    // values:
    //
    //   truecolor
    //   24bit
    //
    // https://gist.github.com/XVilka/8346728#true-color-detection gives some more information about
    // the topic
    //
    const char* cterm = getenv("COLORTERM");
    rgb = cterm && (strcmp(cterm, "truecolor") == 0 || strcmp(cterm, "24bit") == 0);
  }
  return rgb;
}

int term_verify_seq(char** gseq, const char* name){
  char* seq;
  if(gseq == NULL){
    gseq = &seq;
  }
  *gseq = tigetstr(name);
  if(*gseq == NULL || *gseq == (char*)-1){
    return -1;
  }
  return 0;
}

int interrogate_terminfo(tinfo* ti){
  memset(ti, 0, sizeof(*ti));
  ti->RGBflag = query_rgb();
  if((ti->colors = tigetnum("colors")) <= 0){
    ti->colors = 1;
    ti->CCCflag = false;
    ti->RGBflag = false;
    ti->initc = NULL;
  }else{
    term_verify_seq(&ti->initc, "initc");
    if(ti->initc){
      ti->CCCflag = tigetflag("ccc") == 1;
    }else{
      ti->CCCflag = false;
    }
  }
  term_verify_seq(&ti->cup, "cup");
  if(ti->cup == NULL){
    fprintf(stderr, "Required terminfo capability 'cup' not defined\n");
    return -1;
  }
  ti->AMflag = tigetflag("am") == 1;
  if(!ti->AMflag){
    fprintf(stderr, "Required terminfo capability 'am' not defined\n");
    return -1;
  }
  term_verify_seq(&ti->civis, "civis");
  term_verify_seq(&ti->cnorm, "cnorm");
  term_verify_seq(&ti->standout, "smso"); // smso / rmso
  term_verify_seq(&ti->uline, "smul");
  term_verify_seq(&ti->reverse, "reverse");
  term_verify_seq(&ti->blink, "blink");
  term_verify_seq(&ti->dim, "dim");
  term_verify_seq(&ti->bold, "bold");
  term_verify_seq(&ti->italics, "sitm");
  term_verify_seq(&ti->italoff, "ritm");
  term_verify_seq(&ti->sgr, "sgr");
  term_verify_seq(&ti->sgr0, "sgr0");
  term_verify_seq(&ti->op, "op");
  term_verify_seq(&ti->oc, "oc");
  term_verify_seq(&ti->home, "home");
  term_verify_seq(&ti->clearscr, "clear");
  term_verify_seq(&ti->cleareol, "el");
  term_verify_seq(&ti->clearbol, "el1");
  term_verify_seq(&ti->cuu, "cuu"); // move N up
  term_verify_seq(&ti->cud, "cud"); // move N down
  term_verify_seq(&ti->hpa, "hpa");
  term_verify_seq(&ti->vpa, "vpa");
  term_verify_seq(&ti->cuf, "cuf"); // n non-destructive spaces
  term_verify_seq(&ti->cub, "cub"); // n non-destructive backspaces
  term_verify_seq(&ti->cuf1, "cuf1"); // non-destructive space
  term_verify_seq(&ti->cub1, "cub1"); // non-destructive backspace
  term_verify_seq(&ti->smkx, "smkx"); // set application mode
  if(ti->smkx){
    if(putp(tiparm(ti->smkx)) != OK){
      fprintf(stderr, "Error entering application mode\n");
      return -1;
    }
  }
  // Some terminals cannot combine certain styles with colors. Don't advertise
  // support for the style in that case.
  int nocolor_stylemask = tigetnum("ncv");
  if(nocolor_stylemask > 0){
    if(nocolor_stylemask & WA_STANDOUT){ // ncv is composed of terminfo bits, not ours
      ti->standout = NULL;
    }
    if(nocolor_stylemask & WA_UNDERLINE){
      ti->uline = NULL;
    }
    if(nocolor_stylemask & WA_REVERSE){
      ti->reverse = NULL;
    }
    if(nocolor_stylemask & WA_BLINK){
      ti->blink = NULL;
    }
    if(nocolor_stylemask & WA_DIM){
      ti->dim = NULL;
    }
    if(nocolor_stylemask & WA_BOLD){
      ti->bold = NULL;
    }
    if(nocolor_stylemask & WA_ITALIC){
      ti->italics = NULL;
    }
  }
  term_verify_seq(&ti->getm, "getm"); // get mouse events
  // Not all terminals support setting the fore/background independently
  term_verify_seq(&ti->setaf, "setaf");
  term_verify_seq(&ti->setab, "setab");
  term_verify_seq(&ti->smkx, "smkx");
  term_verify_seq(&ti->rmkx, "rmkx");
  return 0;
}

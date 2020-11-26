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

int terminfostr(char** gseq, const char* name){
  char* seq;
  if(gseq == NULL){
    gseq = &seq;
  }
  *gseq = tigetstr(name);
  if(*gseq == NULL || *gseq == (char*)-1){
    *gseq = NULL;
    return -1;
  }
  // terminfo syntax allows a number N of milliseconds worth of pause to be
  // specified using $<N> syntax. this is then honored by tputs(). but we don't
  // use tputs(), instead preferring the much faster stdio+tiparm(). to avoid
  // dumping "$<N>" sequences all over stdio, we chop them out.
  char* pause;
  if( (pause = strchr(*gseq, '$')) ){
    *pause = '\0';
  }
  return 0;
}

int interrogate_terminfo(tinfo* ti){
  memset(ti, 0, sizeof(*ti));
  ti->RGBflag = query_rgb();
  int colors = tigetnum("colors");
  if(colors <= 0){
    ti->colors = 1;
    ti->CCCflag = false;
    ti->RGBflag = false;
    ti->initc = NULL;
  }else{
    ti->colors = colors;
    terminfostr(&ti->initc, "initc");
    if(ti->initc){
      ti->CCCflag = tigetflag("ccc") == 1;
    }else{
      ti->CCCflag = false;
    }
  }
  // check that the terminal provides cursor addressing (absolute movement)
  terminfostr(&ti->cup, "cup");
  if(ti->cup == NULL){
    fprintf(stderr, "Required terminfo capability 'cup' not defined\n");
    return -1;
  }
  // check that the terminal provides automatic margins
  ti->AMflag = tigetflag("am") == 1;
  if(!ti->AMflag){
    fprintf(stderr, "Required terminfo capability 'am' not defined\n");
    return -1;
  }
  ti->BCEflag = tigetflag("bce") == 1;
  terminfostr(&ti->civis, "civis"); // cursor invisible
  if(ti->civis == NULL){
    terminfostr(&ti->civis, "chts");// hard-to-see cursor
  }
  terminfostr(&ti->cnorm, "cnorm"); // cursor normal (undo civis/cvvis)
  terminfostr(&ti->standout, "smso"); // begin standout mode
  terminfostr(&ti->uline, "smul");    // begin underline mode
  terminfostr(&ti->reverse, "rev");   // begin reverse video mode
  terminfostr(&ti->blink, "blink");   // turn on blinking
  terminfostr(&ti->dim, "dim");       // turn on half-bright mode
  terminfostr(&ti->bold, "bold");     // turn on extra-bright mode
  terminfostr(&ti->italics, "sitm");  // begin italic mode
  terminfostr(&ti->italoff, "ritm");  // end italic mode
  terminfostr(&ti->sgr, "sgr");       // define video attributes
  terminfostr(&ti->sgr0, "sgr0");     // turn off all video attributes
  terminfostr(&ti->op, "op");         // restore defaults to default pair
  terminfostr(&ti->oc, "oc");         // restore defaults to all colors
  terminfostr(&ti->home, "home");     // home the cursor
  terminfostr(&ti->clearscr, "clear");// clear screen, home cursor
  terminfostr(&ti->cleareol, "el");   // clear to end of line
  terminfostr(&ti->clearbol, "el1");  // clear to beginning of line
  terminfostr(&ti->cuu, "cuu"); // move N up
  terminfostr(&ti->cud, "cud"); // move N down
  terminfostr(&ti->hpa, "hpa"); // set horizontal position
  terminfostr(&ti->vpa, "vpa"); // set verical position
  terminfostr(&ti->cuf, "cuf"); // n non-destructive spaces
  terminfostr(&ti->cub, "cub"); // n non-destructive backspaces
  terminfostr(&ti->cuf1, "cuf1"); // non-destructive space
  terminfostr(&ti->cub1, "cub1"); // non-destructive backspace
  terminfostr(&ti->sc, "sc"); // push ("save") cursor
  terminfostr(&ti->rc, "rc"); // pop ("restore") cursor
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
  terminfostr(&ti->getm, "getm"); // get mouse events
  // Not all terminals support setting the fore/background independently
  terminfostr(&ti->setaf, "setaf"); // set forground color
  terminfostr(&ti->setab, "setab"); // set background color
  terminfostr(&ti->smkx, "smkx");   // enable keypad transmit
  terminfostr(&ti->rmkx, "rmkx");   // disable keypad transmit
  // if the keypad neen't be explicitly enabled, smkx is not present
  if(ti->smkx){
    if(putp(tiparm(ti->smkx)) != OK){
      fprintf(stderr, "Error entering keypad transmit mode\n");
      return -1;
    }
  }
  // some control sequences are unavailable from terminfo, and we must instead
  // hardcode them :/. use at your own peril!
  ti->struck = "\x1b[9m";
  ti->struckoff = "\x1b[29m";
  return 0;
}

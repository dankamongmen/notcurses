#include "internal.h"

int ncdirect_cursor_up(ncdirect* nc, int num){
  if(num < 0){
    return -1;
  }
  if(!nc->cuu){
    return -1;
  }
  return term_emit("cuu", tiparm(nc->cuu, num), nc->ttyfp, false);
}

int ncdirect_cursor_left(ncdirect* nc, int num){
  if(num < 0){
    return -1;
  }
  if(!nc->cub){
    return -1;
  }
  return term_emit("cub", tiparm(nc->cub, num), nc->ttyfp, false);
}

int ncdirect_cursor_right(ncdirect* nc, int num){
  if(num < 0){
    return -1;
  }
  if(!nc->cuf){ // FIXME fall back to cuf1
    return -1;
  }
  return term_emit("cuf", tiparm(nc->cuf, num), nc->ttyfp, false);
}

int ncdirect_cursor_down(ncdirect* nc, int num){
  if(num < 0){
    return -1;
  }
  if(!nc->cud){
    return -1;
  }
  return term_emit("cud", tiparm(nc->cud, num), nc->ttyfp, false);
}

int ncdirect_clear(ncdirect* nc){
  if(!nc->clear){
    return -1; // FIXME scroll output off the screen
  }
  return term_emit("clear", nc->clear, nc->ttyfp, true);
}

int ncdirect_dim_x(const ncdirect* nc){
  int x;
  if(update_term_dimensions(fileno(nc->ttyfp), NULL, &x) == 0){
    return x;
  }
  return -1;
}

int ncdirect_dim_y(const ncdirect* nc){
  int y;
  if(update_term_dimensions(fileno(nc->ttyfp), &y, NULL) == 0){
    return y;
  }
  return -1;
}

int ncdirect_cursor_enable(ncdirect* nc){
  if(!nc->cnorm){
    return -1;
  }
  return term_emit("cnorm", nc->cnorm, nc->ttyfp, true);
}

int ncdirect_cursor_disable(ncdirect* nc){
  if(!nc->civis){
    return -1;
  }
  return term_emit("civis", nc->civis, nc->ttyfp, true);
}

int ncdirect_cursor_move_yx(ncdirect* n, int y, int x){
  if(y == -1){ // keep row the same, horizontal move only
    if(!n->hpa){
      return -1;
    }
    return term_emit("hpa", tiparm(n->hpa, x), n->ttyfp, false);
  }else if(x == -1){ // keep column the same, vertical move only
    if(!n->vpa){
      return -1;
    }
    return term_emit("vpa", tiparm(n->vpa, y), n->ttyfp, false);
  }
  if(n->cup){
    return term_emit("cup", tiparm(n->cup, y, x), n->ttyfp, false);
  }else if(n->vpa && n->hpa){
    if(term_emit("hpa", tiparm(n->hpa, x), n->ttyfp, false) == 0 &&
       term_emit("vpa", tiparm(n->vpa, y), n->ttyfp, false) == 0){
      return 0;
    }
  }
  return -1;
}

// no terminfo capability for this. dangerous!
// FIXME need to empty input somehow and do other craps. make input available
// immediately etc...
int ncdirect_cursor_yx(ncdirect* n, int* y, int* x){
  FILE* fin = fopen("/dev/tty", "rb");
  if(fin == NULL){
    return -1;
  }
  if(fprintf(n->ttyfp, "\033[6n") != 4){
    fclose(fin);
    return -1;
  }
  int in;
  bool done = false;
  enum { // what we expect now
    CURSOR_ESC, // 27 (0x1b)
    CURSOR_LSQUARE,
    CURSOR_ROW, // delimited by a semicolon
    CURSOR_COLUMN,
    CURSOR_R,
  } state = CURSOR_ESC;
  int row = 0, column = 0;
  while((in = getc(fin)) != EOF){
    bool valid = false;
    switch(state){
      case CURSOR_ESC: valid = (in == '\x1b'); ++state; break;
      case CURSOR_LSQUARE: valid = (in == '['); ++state; break;
      case CURSOR_ROW:
        if(isdigit(in)){
          row *= 10;
          row += in - '0';
          valid = true;
        }else if(in == ';'){
          ++state;
          valid = true;
        }
        break;
      case CURSOR_COLUMN:
        if(isdigit(in)){
          column *= 10;
          column += in - '0';
          valid = true;
        }else if(in == 'R'){
          ++state;
          valid = true;
        }
        break;
      case CURSOR_R: default: // logical error, whoops
        break;
    }
    if(!valid){
      break;
    }
    if(state == CURSOR_R){
      done = true;
      break;
    }
  }
  int fc = fclose(fin);
  if(fc || !done){
    return -1;
  }
  if(y){
    *y = row;
  }
  if(x){
    *x = column;
  }
  return 0;
}

int ncdirect_cursor_push(ncdirect* n){
  if(n->sc == NULL){
    return -1;
  }
  return term_emit("sc", n->sc, n->ttyfp, false);
}

int ncdirect_cursor_pop(ncdirect* n){
  if(n->rc == NULL){
    return -1;
  }
  return term_emit("rc", n->rc, n->ttyfp, false);
}

int ncdirect_stop(ncdirect* nc){
  int ret = 0;
  if(nc){
    if(nc->op && term_emit("op", nc->op, nc->ttyfp, true)){
      ret = -1;
    }
    if(nc->sgr0 && term_emit("sgr0", nc->sgr0, nc->ttyfp, true)){
      ret = -1;
    }
    if(nc->oc && term_emit("oc", nc->oc, nc->ttyfp, true)){
      ret = -1;
    }
    free(nc);
  }
  return ret;
}

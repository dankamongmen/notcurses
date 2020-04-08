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


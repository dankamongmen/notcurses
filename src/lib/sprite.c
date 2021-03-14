#include "internal.h"

void sprixel_free(sprixel* s){
  if(s){
    free(s->glyph);
    free(s);
  }
}

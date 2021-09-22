#include "internal.h"

// we assumed escapes can only be composed of 7-bit chars
typedef struct esctrie {
  struct esctrie** trie;  // if non-NULL, next level of radix-128 trie
  ncinput ni;             // composed key terminating here
} esctrie;

uint32_t esctrie_id(const esctrie* e){
  return e->ni.id;
}

void esctrie_ni(const esctrie* e, ncinput* ni){
  memcpy(ni, &e->ni, sizeof(*ni));
}

esctrie** esctrie_trie(esctrie* e){
  return e->trie;
}

esctrie* create_esctrie_node(int special){
  esctrie* e = malloc(sizeof(*e));
  if(e){
    memset(e, 0, sizeof(*e));
    e->ni.id = special;
  }
  return e;
}

void input_free_esctrie(esctrie** eptr){
  esctrie* e;
  if( (e = *eptr) ){
    if(e->trie){
      int z;
      for(z = 0 ; z < 0x80 ; ++z){
        if(e->trie[z]){
          input_free_esctrie(&e->trie[z]);
        }
      }
      free(e->trie);
    }
    free(e);
  }
}

// multiple input escapes might map to the same input
int inputctx_add_input_escape(esctrie** eptr, const char* esc, uint32_t special,
                              unsigned shift, unsigned ctrl, unsigned alt){
  if(esc[0] != NCKEY_ESC || strlen(esc) < 2){ // assume ESC prefix + content
    logerror("not an escape (0x%x)\n", special);
    return -1;
  }
  if(*eptr == NULL){
    if((*eptr = create_esctrie_node(NCKEY_INVALID)) == NULL){
      return -1;
    }
  }
  esctrie* cur = *eptr;
  ++esc; // don't encode initial escape as a transition
  do{
    int valid = *esc;
    if(valid <= 0 || valid >= 0x80 || valid == NCKEY_ESC){
      logerror("invalid character %d in escape\n", valid);
      return -1;
    }
    if(cur->trie == NULL){
      const size_t tsize = sizeof(cur->trie) * 0x80;
      if((cur->trie = malloc(tsize)) == NULL){
        return -1;
      }
      memset(cur->trie, 0, tsize);
    }
    if(cur->trie[valid] == NULL){
      if((cur->trie[valid] = create_esctrie_node(NCKEY_INVALID)) == NULL){
        return -1;
      }
    }
    cur = cur->trie[valid];
    ++esc;
  }while(*esc);
  // it appears that multiple keys can be mapped to the same escape string. as
  // an example, see "kend" and "kc1" in st ("simple term" from suckless) :/.
  if(cur->ni.id != NCKEY_INVALID){ // already had one here!
    if(cur->ni.id != special){
      logwarn("already added escape (got 0x%x, wanted 0x%x)\n", cur->ni.id, special);
    }
  }else{
    cur->ni.id = special;
    cur->ni.shift = shift;
    cur->ni.ctrl = ctrl;
    cur->ni.alt = alt;
  }
  return 0;
}

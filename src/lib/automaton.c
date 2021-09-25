#include "automaton.h"
#include "internal.h"

// we assumed escapes can only be composed of 7-bit chars
typedef struct esctrie {
  // if non-NULL, this is the next level of radix-128 trie. it is NULL on
  // accepting nodes, since no valid control sequence is a prefix of another
  // valid control sequence.
  struct esctrie** trie;
  enum {
    NODE_SPECIAL,  // an accepting node, or pure transit (if ni.id == 0)
    NODE_NUMERIC,  // accumulates a number
    NODE_STRING,   // accumulates a string
    NODE_FUNCTION, // invokes a function
  } ntype;
  ncinput ni;      // composed key terminating here
  int number;      // accumulated number; reset to 0 on entry
  char* str;       // accumulated string; reset to NULL on entry
  triefunc fxn;    // function to call on match
} esctrie;

uint32_t esctrie_id(const esctrie* e){
  return e->ni.id;
}

const char* esctrie_string(const esctrie* e){
  return e->str;
}

esctrie** esctrie_trie(esctrie* e){
  return e->trie;
}

int esctrie_numeric(const esctrie* e){
  if(e->ntype != NODE_NUMERIC){
    return -1;
  }
  return e->number;
}

static inline esctrie*
create_esctrie_node(int special){
  esctrie* e = malloc(sizeof(*e));
  if(e){
    memset(e, 0, sizeof(*e));
    e->ntype = NODE_SPECIAL;
    if((e->ni.id = special) == 0){
      const size_t tsize = sizeof(*e->trie) * 0x80;
      if( (e->trie = malloc(tsize)) ){
        memset(e->trie, 0, tsize);
        return e;
      }
      free(e);
      return NULL;
    }
    return e;
  }
  return e;
}

static void
free_trienode(esctrie** eptr){
  esctrie* e;
  if( (e = *eptr) ){
    if(e->trie){
      int z;
      for(z = 0 ; z < 0x80 ; ++z){
        // don't recurse down a link to ourselves
        if(e->trie[z] && e->trie[z] != e){
          free_trienode(&e->trie[z]);
        }
        // if it's a numeric path, only recurse once
        if(z == '0'){
          if(e->trie['1'] == e->trie[z]){
            z = '9';
          }
        }
        // if it's an all-strings path, only recurse once
        if(z == ' '){
          if(e->trie['!'] == e->trie[z]){
            z = 0x80;
          }
        }
      }
      free(e->str);
      free(e->trie);
    }
    free(e);
  }
}

void input_free_esctrie(automaton* a){
  free_trienode(&a->escapes);
}

static int
esctrie_make_numeric(esctrie* e){
  if(e->ntype == NODE_NUMERIC){
    return 0;
  }
  if(e->ntype != NODE_SPECIAL){
    logerror("can't make node type %d numeric\n", e->ntype);
    return -1;
  }
  for(int i = '0' ; i < '9' ; ++i){
    if(e->trie[i]){
      logerror("can't make %c-followed numeric\n", i);
      return -1;
    }
  }
  e->ntype = NODE_NUMERIC;
  for(int i = '0' ; i < '9' ; ++i){
    e->trie[i] = e;
  }
  logdebug("made numeric: %p\n", e);
  return 0;
}

static int
esctrie_make_kleene(esctrie* e){
  if(e->ntype != NODE_SPECIAL){
    logerror("can't make node type %d string\n", e->ntype);
    return -1;
  }
  for(int i = 0 ; i < 0x80 ; ++i){
    if(e->trie[i] == NULL){
      e->trie[i] = e;
    }
  }
  logdebug("made kleene: %p\n", e);
  return 0;
}

static int
esctrie_make_function(esctrie* e, triefunc fxn){
  if(e->ntype != NODE_SPECIAL){
    logerror("can't make node type %d function\n", e->ntype);
    return -1;
  }
  if(e->trie){
    logerror("can't make followed function\n");
    return -1;
  }
  e->ntype = NODE_FUNCTION;
  e->fxn = fxn;
  logdebug("made function %p: %p\n", fxn, e);
  return 0;
}

static int
esctrie_make_string(esctrie* e, triefunc fxn){
  if(e->ntype == NODE_STRING){
    return 0;
  }
  if(e->ntype != NODE_SPECIAL){
    logerror("can't make node type %d string\n", e->ntype);
    return -1;
  }
  for(int i = 0 ; i < 0x80 ; ++i){
    if(!isprint(i)){
      continue;
    }
    if(e->trie[i]){
      logerror("can't make %c-followed string\n", i);
      return -1;
    }
  }
  esctrie* newe = create_esctrie_node(0);
  if(newe == NULL){
    return -1;
  }
  for(int i = 0 ; i < 0x80 ; ++i){
    if(!isprint(i)){
      continue;
    }
    e->trie[i] = newe;
  }
  e = newe;
  e->ntype = NODE_STRING;
  for(int i = 0 ; i < 0x80 ; ++i){
    if(!isprint(i)){
      continue;
    }
    e->trie[i] = newe;
  }
  if((e->trie[0x1b] = create_esctrie_node(0)) == NULL){
    return -1;
  }
  e = e->trie[0x1b];
  if((e->trie['\\'] = create_esctrie_node(NCKEY_INVALID)) == NULL){
    return -1;
  }
  e = e->trie['\\'];
  e->ni.id = 0;
  e->ntype = NODE_SPECIAL;
  if(esctrie_make_function(e, fxn)){
    return -1;
  }
  logdebug("made string: %p\n", e);
  return 0;
}

// accept any digit and transition to a numeric node.
static esctrie*
link_numeric(esctrie* e){
  // if we already have any digit links, we must have all 10, and they all must
  // be pointing to the same place. if so, use that linked node.
  esctrie* targ = e->trie['0'];
  for(int i = '1' ; i <= '9' ; ++i){
    if(e->trie[i] != targ){
      logerror("can't transition %p to a non-numeric %p != %p\n", e, e->trie[i], targ);
      return NULL;
    }
  }
  if(targ){
    if(targ->ntype != NODE_NUMERIC){
      logerror("not linked to a numeric (%d)\n", targ->ntype);
      return NULL;
    }
    return targ;
  }
  if( (targ = create_esctrie_node(0)) ){
    if(esctrie_make_numeric(targ)){
      free_trienode(&targ);
      return NULL;
    }
    for(int i = '0' ; i <= '9' ; ++i){
      e->trie[i] = targ;
    }
  }
  return targ;
}

// add a cflow path to the automaton
int inputctx_add_cflow(automaton* a, const char* csi, triefunc fxn){
  if(a->escapes == NULL){
    if((a->escapes = create_esctrie_node(0)) == NULL){
      return -1;
    }
  }
  esctrie* eptr = a->escapes;
  bool inescape = false;
  unsigned char c;
  while( (c = *csi++) ){
    if(c == '\\'){
      if(inescape){
        logerror("illegal escape: \\\n");
        return -1;
      }
      inescape = true;
    }else if(inescape){
      if(c == 'N'){
        eptr = link_numeric(eptr);
        if(eptr == NULL){
          return -1;
        }
      }else if(c == 'S'){
        if(esctrie_make_string(eptr, fxn)){
          return -1;
        }
        return 0;
      }else if(c == 'H'){
        // FIXME
      }else if(c == 'D'){ // drain (kleene closure)
        if(esctrie_make_kleene(eptr)){
          return -1;
        }
      }else{
        logerror("illegal escape: %u\n", c);
        return -1;
      }
      inescape = false;
    }else{
      if(eptr->trie[c] == NULL){
        if((eptr->trie[c] = create_esctrie_node(0)) == NULL){
          return -1;
        }
      }
      eptr = eptr->trie[c];
    }
  }
  free(eptr->trie);
  eptr->trie = NULL;
  return esctrie_make_function(eptr, fxn);
}

// multiple input escapes might map to the same input
int inputctx_add_input_escape(automaton* a, const char* esc, uint32_t special,
                              unsigned shift, unsigned ctrl, unsigned alt){
  if(esc[0] != NCKEY_ESC || strlen(esc) < 2){ // assume ESC prefix + content
    logerror("not an escape (0x%x)\n", special);
    return -1;
  }
  esctrie** eptr = &a->escapes;
  if(*eptr == NULL){
    if((*eptr = create_esctrie_node(0)) == NULL){
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
    if(cur->trie[valid] == NULL){
      if((cur->trie[valid] = create_esctrie_node(0)) == NULL){
        return -1;
      }
    }
    cur = cur->trie[valid];
    ++esc;
  }while(*esc);
  // it appears that multiple keys can be mapped to the same escape string. as
  // an example, see "kend" and "kc1" in st ("simple term" from suckless) :/.
  if(cur->ni.id){ // already had one here!
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

static int
growstring(automaton* a, esctrie* e, unsigned candidate){
  if(!isprint(candidate)){
    logerror("unexpected char %u in string\n", candidate);
    return -1;
  }
  char* tmp = realloc(e->str, a->stridx + 1);
  if(tmp == NULL){
    return -1;
  }
  e->str = tmp;
  e->str[a->stridx - 1] = candidate;
  e->str[a->stridx] = '\0';
  ++a->stridx;
  return 0;
}

// returns -1 for non-match, 0 for match, 1 for acceptance. if we are in the
// middle of a sequence, and receive an escape, *do not call this*, but
// instead call reset_automaton() after replaying the used characters to the
// bulk input buffer, and *then* call this with the escape.
int walk_automaton(automaton* a, struct inputctx* ictx, unsigned candidate,
                   ncinput* ni){
  if(candidate >= 0x80){
    logerror("eight-bit char %u in control sequence\n", candidate);
    return -1;
  }
  esctrie* e = a->state;
  logdebug("state: %p candidate: %c %u type: %d\n", e, candidate, candidate, e->ntype);
  // we ought not have been called for an escape with any state!
  if(candidate == 0x1b && !a->instring){
    assert(NULL == e);
    a->state = a->escapes;
    return 0;
  }
  if(e->ntype == NODE_NUMERIC){
    if(isdigit(candidate)){
      // FIXME check for overflow
      e->number = e->number * 10 + (candidate - '0');
      return 0;
    }
  }else if(e->ntype == NODE_STRING){
    if(candidate == 0x1b){
      a->state = e->trie[candidate];
      a->instring = 0;
    }else if(growstring(a, e, candidate)){
      return -1;
    }
    return 0;
  }
  if((a->state = e->trie[candidate]) == NULL){
    if(isprint(candidate)){
      if(e == a->escapes){
        memset(ni, 0, sizeof(*ni));
        ni->id = candidate;
        ni->alt = true;
        return 1;
      }
    }
    loginfo("unexpected transition %u\n", candidate);
    return -1;
  }
  e = a->state;
  switch(e->ntype){
    case NODE_NUMERIC:
      e->number = candidate - '0';
      break;
    case NODE_STRING:
      a->stridx = 1;
      a->instring = 1;
      if(growstring(a, e, candidate)){
        return -1;
      }
      break;
    case NODE_SPECIAL:
      if(e->ni.id){
        memcpy(ni, &e->ni, sizeof(*ni));
        a->state = NULL; // FIXME?
        return 1;
      }
      break;
    case NODE_FUNCTION:
      a->state = NULL; // FIXME?
      if(e->fxn == NULL){
        return 2;
      }
      return e->fxn(ictx);
      break;
  }
  return 0;
}

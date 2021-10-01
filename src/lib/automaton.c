#include "automaton.h"
#include "internal.h"

// the input automaton, walked for all escape sequences. an escape sequence is
// everything from an escape through recognized termination of that escape, or
// abort of the sequence via another escape, save the case of DCS sequences
// (those beginning with Escape-P), which are terminated by the ST sequence
// Escape-\. in the case of an aborted sequence, the sequence in its entirety
// is replayed as regular input. regular input is not driven through this
// automaton.
//
// one complication is that the user can just press escape themselves, followed
// by arbitrary other keypresses. when input is redirected from some source
// other than the connected terminal, this is no problem: we know control
// sequences to be coming in from the connected terminal, and everything else
// is bulk input.

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
  triefunc fxn;    // function to call on match
  struct esctrie* kleene; // kleene match
} esctrie;

uint32_t esctrie_id(const esctrie* e){
  return e->ni.id;
}

esctrie** esctrie_trie(esctrie* e){
  return e->trie;
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
esctrie_make_kleene(esctrie* e, unsigned follow, esctrie* term){
  if(e->ntype != NODE_SPECIAL){
    logerror("can't make node type %d string\n", e->ntype);
    return -1;
  }
  for(unsigned i = 0 ; i < 0x80 ; ++i){
    if(i == follow){
      e->trie[i] = term;
    }else if(e->trie[i] == NULL){
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

static esctrie*
link_kleene(esctrie* e, unsigned follow){
  if(e->kleene){
    return e->kleene;
  }
  esctrie* term = create_esctrie_node(0);
  if(term == NULL){
    return NULL;
  }
  esctrie* targ = NULL;
  if( (targ = create_esctrie_node(0)) ){
    if(esctrie_make_kleene(targ, follow, term)){
      free_trienode(&targ);
      free_trienode(&term);
      return NULL;
    }
  }
  // fill in all NULL numeric links with the new target
  for(unsigned int i = 0 ; i < 0x80 ; ++i){
    if(i == follow){
      if(e->trie[i]){
        logerror("drain terminator already registered\n");
        free_trienode(&targ);
        free_trienode(&term);
      }
      e->trie[follow] = term;
    }else if(e->trie[i] == NULL){
      e->trie[i] = targ;
      // FIXME travel to the ends and link targ there
    }
  }
  targ->kleene = targ;
  return e->trie[follow];
}

static void
fill_in_numerics(esctrie* e, esctrie* targ, unsigned follow, esctrie* efollow){
  // fill in all NULL numeric links with the new target
  for(int i = '0' ; i <= '9' ; ++i){
    if(e->trie[i] == NULL){
      e->trie[i] = targ;
    }else if(e->trie[i] != e){
      fill_in_numerics(e->trie[i], targ, follow, efollow);
    }
  }
  e->trie[follow] = efollow;
}

// accept any digit and transition to a numeric node.
static esctrie*
link_numeric(esctrie* e, unsigned follow){
  esctrie* targ = NULL;
  // find a linked NODE_NUMERIC, if one exists. we'll want to reuse it.
  for(int i = '0' ; i <= '9' ; ++i){
    targ = e->trie[i];
    if(targ && targ->ntype == NODE_NUMERIC){
      break;
    }
    targ = NULL;
  }
  // we either have a numeric target, or will make one now
  if(targ == NULL){
    if( (targ = create_esctrie_node(0)) ){
      if(esctrie_make_numeric(targ)){
        free_trienode(&targ);
        return NULL;
      }
    }
  }
  // targ is the numeric node we're either creating or coopting
  esctrie* efollow = targ->trie[follow];
  if(efollow == NULL){
    if((efollow = create_esctrie_node(0)) == NULL){
      return NULL;
    }
  }
  for(int i = '0' ; i <= '9' ; ++i){
    if(e->trie[i] == NULL){
      e->trie[i] = targ;
    }
    fill_in_numerics(e->trie[i], targ, follow, efollow);
  }
  return efollow;
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
        // a numeric must be followed by some terminator
        if(!*csi){
          logerror("illegal numeric terminator\n");
          return -1;
        }
        c = *csi++;
        eptr = link_numeric(eptr, c);
        if(eptr == NULL){
          return -1;
        }
      }else if(c == 'S'){
        if(esctrie_make_string(eptr, fxn)){
          return -1;
        }
        return 0;
      }else if(c == 'D'){ // drain (kleene closure)
        // a kleene must be followed by some terminator
        if(!*csi){
          logerror("illegal kleene terminator\n");
          return -1;
        }
        c = *csi++;
        eptr = link_kleene(eptr, c);
        if(eptr == NULL){
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
      }else if(eptr->trie[c] == eptr->kleene){
        if((eptr->trie[c] = create_esctrie_node(0)) == NULL){
          return -1;
        }
      }else if(eptr->trie[c]->ntype == NODE_NUMERIC){
        // punch a hole through the numeric loop. create a new one, and fill
        // it in with the existing target.
        struct esctrie* newe;
        if((newe = create_esctrie_node(0)) == NULL){
          return -1;
        }
        for(int i = 0 ; i < 0x80 ; ++i){
          newe->trie[i] = eptr->trie[c]->trie[i];
        }
        eptr->trie[c] = newe;
      }
      eptr = eptr->trie[c];
    }
  }
  if(inescape){
    logerror("illegal escape at end of line\n");
    return -1;
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
  if(e->ntype == NODE_STRING){
    if(candidate == 0x1b){
      a->state = e->trie[candidate];
      a->instring = 0;
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
  // initialize any node we've just stepped into
  switch(e->ntype){
    case NODE_NUMERIC:
      break;
    case NODE_STRING:
      a->instring = 1;
      break;
    case NODE_SPECIAL:
      if(e->ni.id){
        memcpy(ni, &e->ni, sizeof(*ni));
        return 1;
      }
      break;
    case NODE_FUNCTION:
      if(e->fxn == NULL){
        return 2;
      }
      return e->fxn(ictx);
      break;
  }
  return 0;
}

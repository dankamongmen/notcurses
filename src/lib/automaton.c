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
  // valid control sequence. links are 1-biased (0 is NULL).
  unsigned* trie;
  enum {
    NODE_SPECIAL,  // an accepting node, or pure transit (if ni.id == 0)
    NODE_NUMERIC,  // accumulates a number
    NODE_STRING,   // accumulates a string
    NODE_FUNCTION, // invokes a function
  } ntype;
  ncinput ni;      // composed key terminating here
  triefunc fxn;    // function to call on match
  unsigned kleene; // idx of kleene match
} esctrie;

// get node corresponding to 1-biased index
static inline esctrie*
esctrie_from_idx(const automaton* a, unsigned idx){
  if(idx == 0){
    return NULL;
  }
  return a->nodepool + (idx - 1);
}

// return 1-biased index of node in pool
static inline unsigned
esctrie_idx(const automaton* a, const esctrie* e){
  return e - a->nodepool + 1;
}

uint32_t esctrie_id(const esctrie* e){
  return e->ni.id;
}

// returns the idx of the new node, or 0 on failure (idx is 1-biased)
static inline unsigned
create_esctrie_node(automaton* a, int special){
  if(a->poolused == a->poolsize){
    unsigned newsize = a->poolsize ? a->poolsize * 2 : 512;
    esctrie* tmp = realloc(a->nodepool, sizeof(*a->nodepool) * newsize);
    if(tmp == NULL){
      return 0;
    }
    a->nodepool = tmp;
    a->poolsize = newsize;
  }
  esctrie* e = &a->nodepool[a->poolused++];
  memset(e, 0, sizeof(*e));
  e->ntype = NODE_SPECIAL;
  if((e->ni.id = special) == 0){
    const size_t tsize = sizeof(*e->trie) * 0x80;
    if((e->trie = malloc(tsize)) == NULL){
      --a->poolused;
      return 0;
    }
    memset(e->trie, 0, tsize);
  }
  return esctrie_idx(a, e);
}

void input_free_esctrie(automaton* a){
  a->escapes = 0;
  a->poolsize = 0;
  for(unsigned i = 0 ; i < a->poolused ; ++i){
    free(a->nodepool[i].trie);
  }
  free(a->nodepool);
  a->poolused = 0;
  a->nodepool = NULL;
}

static int
esctrie_make_numeric(automaton* a, esctrie* e){
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
    e->trie[i] = esctrie_idx(a, e);
  }
  return 0;
}

static int
esctrie_make_kleene(automaton* a, esctrie* e, unsigned follow, esctrie* term){
  if(e->ntype != NODE_SPECIAL){
    logerror("can't make node type %d string\n", e->ntype);
    return -1;
  }
  for(unsigned i = 0 ; i < 0x80 ; ++i){
    if(i == follow){
      e->trie[i] = esctrie_idx(a, term);
    }else if(e->trie[i] == 0){
      e->trie[i] = esctrie_idx(a, e);
    }
  }
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
  return 0;
}

static int
esctrie_make_string(automaton* a, esctrie* e, triefunc fxn){
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
  esctrie* newe = esctrie_from_idx(a, create_esctrie_node(a, 0));
  if(newe == NULL){
    return -1;
  }
  for(int i = 0 ; i < 0x80 ; ++i){
    if(!isprint(i)){
      continue;
    }
    e->trie[i] = esctrie_idx(a, newe);
  }
  e = newe;
  e->ntype = NODE_STRING;
  for(int i = 0 ; i < 0x80 ; ++i){
    if(!isprint(i)){
      continue;
    }
    e->trie[i] = esctrie_idx(a, newe);
  }
  if((e->trie[0x1b] = create_esctrie_node(a, 0)) == 0){
    return -1;
  }
  e = esctrie_from_idx(a, e->trie[0x1b]);
  if((e->trie['\\'] = create_esctrie_node(a, NCKEY_INVALID)) == 0){
    return -1;
  }
  e = esctrie_from_idx(a, e->trie['\\']);
  e->ni.id = 0;
  e->ntype = NODE_SPECIAL;
  if(esctrie_make_function(e, fxn)){
    return -1;
  }
  logdebug("made string: %p\n", e);
  return 0;
}

static esctrie*
link_kleene(automaton* a, esctrie* e, unsigned follow){
  if(e->kleene){
    return a->nodepool + e->kleene;
  }
  esctrie* term = esctrie_from_idx(a, create_esctrie_node(a, 0));
  if(term == NULL){
    return NULL;
  }
  esctrie* targ = NULL;
  if((targ = esctrie_from_idx(a, create_esctrie_node(a, 0))) == NULL){
    return NULL;
  }
  if(esctrie_make_kleene(a, targ, follow, term)){
    return NULL;
  }
  // fill in all NULL numeric links with the new target
  for(unsigned int i = 0 ; i < 0x80 ; ++i){
    if(i == follow){
      if(e->trie[i]){
        logerror("drain terminator already registered\n");
        return NULL;
      }
      e->trie[follow] = esctrie_idx(a, term);
    }else if(e->trie[i] == 0){
      e->trie[i] = esctrie_idx(a, targ);
      // FIXME travel to the ends and link targ there
    }
  }
  targ->kleene = esctrie_idx(a, targ);
  return esctrie_from_idx(a, e->trie[follow]);
}

static void
fill_in_numerics(automaton* a, esctrie* e, esctrie* targ, unsigned follow, esctrie* efollow){
  // fill in all NULL numeric links with the new target
  for(int i = '0' ; i <= '9' ; ++i){
    if(e->trie[i] == 0){
      e->trie[i] = esctrie_idx(a, targ);
    }else if(e->trie[i] != esctrie_idx(a, e)){
      fill_in_numerics(a, esctrie_from_idx(a, e->trie[i]), targ, follow, efollow);
    }
  }
  e->trie[follow] = esctrie_idx(a, efollow);
}

// accept any digit and transition to a numeric node.
static esctrie*
link_numeric(automaton* a, esctrie* e, unsigned follow){
  esctrie* targ = NULL;
  // find a linked NODE_NUMERIC, if one exists. we'll want to reuse it.
  for(int i = '0' ; i <= '9' ; ++i){
    targ = esctrie_from_idx(a, e->trie[i]);
    if(targ && targ->ntype == NODE_NUMERIC){
      break;
    }
    targ = NULL;
  }
  // we either have a numeric target, or will make one now
  if(targ == NULL){
    if((targ = esctrie_from_idx(a, create_esctrie_node(a, 0))) == 0){
      return NULL;
    }
    if(esctrie_make_numeric(a, targ)){
      return NULL;
    }
  }
  // targ is the numeric node we're either creating or coopting
  esctrie* efollow = esctrie_from_idx(a, targ->trie[follow]);
  if(efollow == NULL){
    if((efollow = esctrie_from_idx(a, create_esctrie_node(a, 0))) == NULL){
      return NULL;
    }
  }
  for(int i = '0' ; i <= '9' ; ++i){
    if(e->trie[i] == 0){
      e->trie[i] = esctrie_idx(a, targ);
    }
    fill_in_numerics(a, esctrie_from_idx(a, e->trie[i]), targ, follow, efollow);
  }
  return efollow;
}

// add a cflow path to the automaton
int inputctx_add_cflow(automaton* a, const char* seq, triefunc fxn){
  if(a->escapes == 0){
    if((a->escapes = create_esctrie_node(a, 0)) == 0){
      return -1;
    }
  }
  esctrie* eptr = esctrie_from_idx(a, a->escapes);
  bool inescape = false;
  unsigned char c;
  while( (c = *seq++) ){
    if(c == '\\'){
      if(inescape){
        logerror("illegal escape: \\\n");
        return -1;
      }
      inescape = true;
    }else if(inescape){
      if(c == 'N'){
        // a numeric must be followed by some terminator
        if(!*seq){
          logerror("illegal numeric terminator\n");
          return -1;
        }
        c = *seq++;
        eptr = link_numeric(a, eptr, c);
        if(eptr == NULL){
          return -1;
        }
      }else if(c == 'S'){
        if(esctrie_make_string(a, eptr, fxn)){
          return -1;
        }
        return 0;
      }else if(c == 'D'){ // drain (kleene closure)
        // a kleene must be followed by some terminator
        if(!*seq){
          logerror("illegal kleene terminator\n");
          return -1;
        }
        c = *seq++;
        eptr = link_kleene(a, eptr, c);
        if(eptr == NULL){
          return -1;
        }
      }else{
        logerror("illegal escape: %u\n", c);
        return -1;
      }
      inescape = false;
    }else{
      if(eptr->trie[c] == 0){
        if((eptr->trie[c] = create_esctrie_node(a, 0)) == 0){
          return -1;
        }
      }else if(eptr->trie[c] == eptr->kleene){
        if((eptr->trie[c] = create_esctrie_node(a, 0)) == 0){
          return -1;
        }
      }else if(esctrie_from_idx(a, eptr->trie[c])->ntype == NODE_NUMERIC){
        // punch a hole through the numeric loop. create a new one, and fill
        // it in with the existing target.
        struct esctrie* newe;
        if((newe = esctrie_from_idx(a, create_esctrie_node(a, 0))) == 0){
          return -1;
        }
        for(int i = 0 ; i < 0x80 ; ++i){
          newe->trie[i] = esctrie_from_idx(a, eptr->trie[c])->trie[i];
        }
        eptr->trie[c] = esctrie_idx(a, newe);
      }
      eptr = esctrie_from_idx(a, eptr->trie[c]);
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
  if(a->escapes == 0){
    if((a->escapes = create_esctrie_node(a, 0)) == 0){
      return -1;
    }
  }
  esctrie* cur = esctrie_from_idx(a, a->escapes);
  ++esc; // don't encode initial escape as a transition
  do{
    int valid = *esc;
    if(valid <= 0 || valid >= 0x80 || valid == NCKEY_ESC){
      logerror("invalid character %d in escape\n", valid);
      return -1;
    }
    if(cur->trie[valid] == 0){
      if((cur->trie[valid] = create_esctrie_node(a, 0)) == 0){
        return -1;
      }
    }
    cur = esctrie_from_idx(a, cur->trie[valid]);
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
  esctrie* e = esctrie_from_idx(a, a->state);
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
  if((a->state = e->trie[candidate]) == 0){
    if(isprint(candidate)){
      if(esctrie_idx(a, e) == a->escapes){
        memset(ni, 0, sizeof(*ni));
        ni->id = candidate;
        ni->alt = true;
        return 1;
      }
    }
    loginfo("unexpected transition %u\n", candidate);
    return -1;
  }
  e = esctrie_from_idx(a, a->state);
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

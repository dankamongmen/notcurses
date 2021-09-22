#ifndef NOTCURSES_AUTOMATON
#define NOTCURSES_AUTOMATON

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

struct ncinput;
struct esctrie;

void input_free_esctrie(struct esctrie** eptr);

int inputctx_add_input_escape(struct esctrie** eptr, const char* esc,
                              uint32_t special, unsigned shift,
                              unsigned ctrl, unsigned alt);

uint32_t esctrie_id(const struct esctrie* e);
void esctrie_ni(const struct esctrie* e, struct ncinput* ni);
// returns 128-way array of esctrie pointers
struct esctrie** esctrie_trie(struct esctrie* e);

#ifdef __cplusplus
}
#endif

#endif

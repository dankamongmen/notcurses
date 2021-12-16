#ifndef NOTCURSES_MAN_STRUCTURE
#define NOTCURSES_MAN_STRUCTURE

#ifdef __cplusplus
extern "C" {
#endif

struct ncplane;
struct docstructure;

struct docstructure* docstructure_create(struct ncplane* n);
void docstructure_free(struct docstructure* ds);

void docstructure_toggle(struct ncplane* p, struct ncplane* b, struct docstructure *ds);

// add the specified [sub]section to the document strucure.
int docstructure_add(struct docstructure* ds, const char* title, int y);

// update the docstructure browser based off a move of the page plane from to |newy|.
// |movedown| ought be non-zero iff the move was down.
int docstructure_move(struct docstructure* ds, int newy, unsigned movedown);

int docstructure_prev(struct docstructure* ds);
int docstructure_next(struct docstructure* ds);

#ifdef __cplusplus
}
#endif

#endif

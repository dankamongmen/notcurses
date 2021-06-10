#ifndef NOTCURSES_NCSEQS
#define NOTCURSES_NCSEQS

#ifdef __cplusplus
extern "C" {
#endif

// unicode box-drawing characters
#define NCBOXLIGHT  "┌┐└┘─│"
#define NCBOXHEAVY  "┏┓┗┛━┃"
#define NCBOXROUND  "╭╮╰╯─│"
#define NCBOXDOUBLE "╔╗╚╝═║"
// argh
#define NCBOXLIGHTW  L"┌┐└┘─│"
#define NCBOXHEAVYW  L"┏┓┗┛━┃"
#define NCBOXROUNDW  L"╭╮╰╯─│"
#define NCBOXDOUBLEW L"╔╗╚╝═║"

#ifdef __cplusplus
} // extern "C"
#endif

#endif

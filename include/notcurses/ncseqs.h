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
#define NCBOXASCII  "/\\\\/-|"
// argh
#define NCBOXLIGHTW  L"┌┐└┘─│"
#define NCBOXHEAVYW  L"┏┓┗┛━┃"
#define NCBOXROUNDW  L"╭╮╰╯─│"
#define NCBOXDOUBLEW L"╔╗╚╝═║"
#define NCBOXASCIIW  L"/\\\\/-|"

#ifdef __cplusplus
} // extern "C"
#endif

#endif

#ifndef NOTCURSES_LINUX
#define NOTCURSES_LINUX

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

struct tinfo;

bool is_linux_console(int fd, unsigned no_font_changes, bool* quadrants);

// if is_linux_console returned true, call this to determine whether it is
// a drawable framebuffer console. do not call if not is_linux_console!
bool is_linux_framebuffer(struct tinfo* ti);

#ifdef __cplusplus
}
#endif

#endif

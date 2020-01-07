from cffi import FFI
ffibuild = FFI()

ffibuild.set_source(
    "_notcurses",
    """
    #include <notcurses.h>
    """,
    libraries=["notcurses"],
)

ffibuild.cdef("""
struct notcurses_options;
struct notcurses* notcurses_init(const struct notcurses_options*, FILE*);
int notcurses_stop(struct notcurses*);
int notcurses_render(struct notcurses*);
""")

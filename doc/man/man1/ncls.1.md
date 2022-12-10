% ncls(1)
% nick black <nickblack@linux.com>
% v3.0.9

# NAME

ncls - List paths with rendering of multimedia

# SYNOPSIS

**ncls** [**-h**|**--help**] [**-V**|**--version**] [**-d**] [**-l**] [**-L**] [**-R**] [**-a**|**--align** ***type***] [**-b**|**--blitter** ***blitter***] [**-s**|**--scale** ***scale***] [ paths ]

# DESCRIPTION

**ncls** uses a multimedia-enabled Notcurses to list paths, similarly to the
**ls(1)** command, rendering images and videos to a terminal.

# OPTIONS

**-d**: list directories themselves, not their contents.

**-l**: use a long listing format.

**-L**: dereference symbolic links

**-R**: list subdirectories recursively.

**-V**|**--version**: Print version information, and exit with success.

**-h**|**--help**: Print help information, and exit with success.

**-a**|**--align** ***type***: Align images on **left**, **center**, or **right**.

**-b**|**--blitter** ***blitter***: Blitter, one of **ascii**, **half**,
**quad**, **sex**, **braille**, or **pixel**. The default is **pixel**.
If the chosen blitter is unavailable, **ncls** will degrade (see
**notcurses_visual(3)**).

**-s**|**--scale** ***scalemode***: Scaling mode, one of **none**, **hires**,
**scale**, **scalehi**, or **stretch**.

paths: Run on the specified paths. If none are supplied, run on the current
directory.

If no scaling is specified, **hires** will be used with the **pixel** blitter,
and **scalehi** will be used otherwise.

# NOTES

Optimal display requires a terminal advertising the **rgb** terminfo(5)
capability, or that the environment variable **COLORTERM** is defined to
**24bit** (and that the terminal honors this variable), along with a
fixed-width font with good coverage of the Unicode Block Drawing Characters.

# SEE ALSO

**notcurses(3)**,
**notcurses_visual(3)**,
**terminfo(5)**,
**unicode(7)**

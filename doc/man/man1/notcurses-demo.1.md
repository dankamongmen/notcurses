% notcurses-demo(1)
% nick black <nickblack@linux.com>
% v1.1.0

# NAME

notcurses-demo - Show off some notcurses features

# SYNOPSIS

**notcurses-demo** [**-h|--help**] [**-p path**] [**-d delaymult**] [**-l loglevel**] [**-kHVc**] demospec

# DESCRIPTION

**notcurses-demo** demonstrates the capabilities of the notcurses library. It
can be run in any terminal emulator or console with a correct terminfo(5)
database, but is at is best in a "DirectColor" 24bpp RGB environment. If
**notcurses-demo** seems to generate garbage, something is likely configured in
a way that is going to prevent notcurses from working.

The demonstrations include:

* (i)ntro—a setting of tone
* (b)oxes—pulsating boxes with a transparent center
* (c)hunli—the strongest woman in the world
* (e)agle—they took some time off my life, back in the day
* (f)allin—the screen falls apart under heavy blows
* (g)rid—a gradient of color lain atop a great grid
* (j)ungle—low-bandwidth color cycling reveals ancient ruins
* (l)uigi-a dashing Apennine plumber in a world of fire
* (p)anelreels—demonstration of the panelreel high-level widget
* (s)liders—a missing-piece puzzle made up of colorful blocks
* (t)rans—an exploration of various transparencies
* (u)niblocks—a series of blocks detailing Unicode pages
* (v)iew—images and a video are rendered as text
* (w)itherworm—a great Nothing slowly robs the world of color
* (x)ray—stimulate a logo with energy
* (o)utro—a message of hope from the library's author

At any time, press 'q' to quit. The demo is best run in at least a 80x45 terminal.

# OPTIONS

**-p path**: Look in the specified **path** for data files.

**-d delaymult**: Apply a rational multiplier to the standard delay of 1s.

**-l loglevel**: Log everything (high log level) or nothing (log level 0) to stderr.

**-H**: Launch a HUD with running timers for each demo. This HUD can be moved or closed with the mouse.

**-k**: Inhibit use of the alternate screen. Necessary if you want the output left on your terminal after the program exits.

**-c**: Do not attempt to seed the PRNG. This is useful when benchmarking.

**-h**: Print a usage message, and exit with success.

**-V**: Print the program name and version, and exit with success.

demospec: Select which demos to run, and what order to run them in. The default is **ixetcgpwubvlfsjo**. See above for a list of demos.

# NOTES
Proper display requires:

* A terminal advertising the **rgb** terminfo(5) capability, or that the environment variable **COLORTERM** is defined to **24bit** (and that the terminal honors this variable),
* A monospaced font, and
* Good Unicode support in your libc, font, and terminal emulator.

# BUGS

If notcurses is built without FFmpeg, the following demos will fail immedaitely when specified: **xray**,
**eagle**, **chunli**, **fallin**, **view**, **outro**.

# AUTHORS

* All code and design copyright Nick Black.
* Images from Street Fighter II and Mega Man 2 copyright Capcom of America.
* Images from Super Mario Bros. copyright Nintendo of America.
* Images from Ninja Gaiden copyright Koei Tecmo America.
* "Jungle with Rain" and "Ruins with Rain" copyright Mark Ferrari.

# SEE ALSO
notcurses(3notcurses), ncurses(3ncurses), terminfo(5)

% notcurses-demo(1)
% nick black <nickblack@linux.com>
% v1.2.0

# NAME

notcurses-demo - Show off some notcurses features

# SYNOPSIS

**notcurses-demo** [**-h|--help**] [**-p path**] [**-d delaymult**]
 [**-l loglevel**] [**-f renderfile**] [**-ikVc**] demospec

# DESCRIPTION

**notcurses-demo** demonstrates the capabilities of the notcurses library. It
can be run in any terminal emulator or console with a correct terminfo(5)
database, but is at is best in a "DirectColor" 24bpp RGB environment. If
**notcurses-demo** seems to generate garbage, something is likely configured in
a way that is going to prevent notcurses from working.

The demonstrations include (see NOTES below):

* (i)ntro—a setting of tone
* (b)oxes—pulsating boxes with a transparent center
* (c)hunli—the strongest woman in the world
* (e)agle—they took some time off my life, back in the day
* (f)allin'—the screen falls apart under heavy blows
* (g)rid—a gradient of color lain atop a great grid
* (h)ighcon—high contrast text atop various colors
* (j)ungle—low-bandwidth color cycling reveals ancient ruins
* (l)uigi-a dashing Apennine plumber in a world of fire
* (r)eel—demonstration of the ncreel high-level widget
* (s)liders—a missing-piece puzzle made up of colorful blocks
* (t)rans—an exploration of various transparencies
* (u)niblocks—a series of blocks detailing Unicode pages
* (v)iew—images and a video are rendered as text
* (w)hiteout—a great Nothing slowly robs the world of color
* (x)ray—stimulate a logo with energy
* (o)utro—a message of hope from the library's author

At any time, press 'q' to quit. The demo is best run in at least an 80x45 terminal.

# OPTIONS

**-p path**: Look in the specified **path** for data files.

**-d delaymult**: Apply a non-negative rational multiplier to the standard delay of 1s.

**-l loglevel**: Log everything (high log level) or nothing (log level 0) to stderr.

**-f renderfile**: Render each frame to **renderfile** in addition to the screen.

**-k**: Inhibit use of the alternate screen. Necessary if you want the output left on your terminal after the program exits.

**-c**: Do not attempt to seed the PRNG. This is useful when benchmarking.

**-i**: Continue after a failing demo.

**-h**: Print a usage message, and exit with success.

**-V**: Print the program name and version, and exit with success.

demospec: Select which demos to run, and what order to run them in. The default is **ixethbcgrwuvlfsjo**. See above for a list of demos.

# NOTES

Proper display requires:

* A terminal advertising the **rgb** terminfo(5) capability, or that the environment variable **COLORTERM** is defined to **24bit** (and that the terminal honors this variable),
* A monospaced font, and
* Good Unicode support in your libc, font, and terminal emulator.

The Debian version of notcurses leaves out certain multimedia considered
non-free under the Debian Free Software Guidelines. As a result, the
**chunli**, **eagle**, **fallin'**, **jungle**, **luigi**, and **view** demos
are unavailable through the Debian package.

If notcurses is built without FFmpeg, the **chunli**, **eagle**, **fallin'**,
**outro**, **view**, and **xray** demos will be unavailable.

The following keypresses are recognized (and are also available from the menu):

* **Ctrl-U**: Toggle the help screen.
* **H**: Toggle the HUD. The HUD shows the most recent and current demos'
         runtime and number of rendered frames. It can be grabbed and moved
         with the mouse.
* **Ctrl-R**: Restart the demo.
* **q**: Quit.

# BUGS

# AUTHORS

* All code and design copyright Nick Black.
* Images from Street Fighter II and Mega Man 2 copyright Capcom of America.
* Images from Super Mario Bros. copyright Nintendo of America.
* Images from Ninja Gaiden copyright Koei Tecmo America.
* Images from Final Fantasy copyright Square Enix Co Ltd.
* "Jungle with Rain" and "Ruins with Rain" copyright Mark Ferrari/Living Worlds.

# SEE ALSO

**notcurses(3)**,
**ncurses(3NCURSES)**,
**terminfo(5)**

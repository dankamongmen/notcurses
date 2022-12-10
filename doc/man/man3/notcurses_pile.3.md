% notcurses_pile(3)
% nick black <nickblack@linux.com>
% v3.0.9

# NAME

notcurses_pile - operations on Notcurses piles

# SYNOPSIS

**struct ncplane* notcurses_top(struct notcurses* ***n***);**

**struct ncplane* notcurses_bottom(struct notcurses* ***n***);**

**struct ncplane* ncpile_top(struct ncplane* ***n***);**

**struct ncplane* ncpile_bottom(struct ncplane* ***n***);**

# DESCRIPTION

Notcurses does not export the **ncpile** type, nor any functionality that
uses it directly. Piles are nonetheless an important concept in Notcurses.
Functions which operate on piles (e.g. **ncpile_render(3)** are invoked
with any **ncplane** within that pile.

Piles are collections of **ncplane**s, independent from one another for
purposes of rendering and also thread-safety. While only one pile can be
rasterized (written to the display) at a time, arbitrary concurrent actions
can be safely performed on distinct piles. Piles do not compose: rasterizing
a pile destroys any overlapping material.

A pile is created in one of three ways:

* **ncpile_create(3)** is called,
* **ncvisual_blit(3)** is called with a **NULL** target plane ***n***, or
* **ncplane_reparent(3)** is called with ***n*** equal to ***newparent***,
   and ***n*** is not already a root plane. 

A pile is destroyed whenever its last **ncplane** is destroyed, or
reparented into some other pile.

The planes of a pile are totally ordered along the z-axis. **ncpile_top** and
**ncpile_bottom** return the topmost and bottommost planes, respectively, of
the pile containing their argument. **notcurses_top** and **notcurses_bottom**
do the same for the standard pile.

# NOTES

# BUGS

# SEE ALSO

**notcurses(3)**,
**notcurses_plane(3)**,
**notcurses_render(3)**,
**notcurses_visual(3)**

# SPDX-License-Identifier: Apache-2.0

# Copyright 2020, 2021 igo95862

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

# http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
from __future__ import annotations

from typing import Optional, Tuple

# Stub file for typing and docs

# region misc


def notcurses_version() -> str:
    """Get human readable string of running Notcurses version."""
    raise NotImplementedError('Stub')


def notcurses_version_components() -> Tuple[int, int, int, int]:
    """Get a tuple of major, minor, patch, tweak integer
    of the running Notcurses version"""
    raise NotImplementedError('Stub')


def ncstrwidth(text: str, /) -> int:
    """Returns the number of columns occupied by a string,
    or -1 if a non-printable/illegal character is encountered."""
    raise NotImplementedError('Stub')


# endregion misc

# region ncchannel


def ncchannels_rgb_initializer(
        fr: int, fg: int, fb: int,
        br: int, bg: int, bb: int,
        /) -> int:
    """Initialize a 64-bit ncchannel pair with specified RGB fg/bg."""
    raise NotImplementedError('Stub')


def ncchannel_rgb_initializer(
        r: int, g: int, b: int,
        /) -> int:
    """Initialize a 32-bit single ncchannel with specified RGB."""
    raise NotImplementedError('Stub')


def ncchannel_r(channel: int, /) -> int:
    """Extract the 8-bit red component from a 32-bit ncchannel."""
    raise NotImplementedError('Stub')


def ncchannel_g(channel: int, /) -> int:
    """Extract the 8-bit green component from a 32-bit ncchannel."""
    raise NotImplementedError('Stub')


def ncchannel_b(channel: int, /) -> int:
    """Extract the 8-bit blue component from a 32-bit ncchannel."""
    raise NotImplementedError('Stub')


def ncchannel_rgb8(channel: int, /) -> Tuple[int, int, int]:
    """Extract the three 8-bit R/G/B components from a 32-bit ncchannel."""
    raise NotImplementedError('Stub')


def ncchannel_set_rgb8(channel: int, r: int, g: int, b: int, /) -> int:
    """Set the three 8-bit components of a 32-bit ncchannel.

    Mark it as not using the default color.
    Retain the other bits unchanged.
    """
    raise NotImplementedError('Stub')


def ncchannel_set_rgb8_clipped(channel: int, r: int, g: int, b: int, /) -> int:
    """Set the three 8-bit components of a 32-bit ncchannel.

    Mark it as not using the default color.
    Retain the other bits unchanged. r, g, and b will be clipped to
    the range [0..255].
    """
    raise NotImplementedError('Stub')


def ncchannel_set(channel: int, rdb: int, /) -> int:
    """Set the three 8-bit components of a 32-bit ncchannel
    from a provide an assembled, packed 24 bits of rgb.
    """
    raise NotImplementedError('Stub')


def ncchannel_alpha(channel: int, /) -> int:
    """Extract 2 bits of foreground alpha from channel"""
    raise NotImplementedError('Stub')


def ncchannel_palindex(channel: int, /) -> int:
    """Extract 2 bits of palindex from channel"""
    raise NotImplementedError('Stub')


def ncchannel_set_alpha(channel: int, alpha: int, /) -> int:
    """Set the 2-bit alpha component of the 32-bit channel."""
    raise NotImplementedError('Stub')


def ncchannel_set_palindex(channel: int, palindex: int, /) ->int:
    """Set the 2-bit palindex of the 32-bit channel."""
    raise NotImplementedError('Stub')


def ncchannel_default_p(channel: int, /) -> bool:
    """Is this ncchannel using the \"default color\" rather
    than RGB/palette-indexed?
    """
    raise NotImplementedError('Stub')


def ncchannel_palindex_p(channel: int, /) -> bool:
    """Is this ncchannel using palette-indexed color rather than RGB?"""
    raise NotImplementedError('Stub')


def ncchannel_set_default(channel: int, /) ->int:
    """Mark the ncchannel as using its default color, which also
    marks it opaque."""
    raise NotImplementedError('Stub')


def ncchannels_bchannel(channel: int, /) -> int:
    """Extract the 32-bit background ncchannel from a channel pair."""
    raise NotImplementedError('Stub')


def ncchannels_fchannel(channel: int, /) -> int:
    """Extract the 32-bit foreground ncchannel from a channel pair."""
    raise NotImplementedError('Stub')


def ncchannels_set_bchannel(channels: int, channel: int, /) -> int:
    """Set the 32-bit background ncchannel of a channels pair."""
    raise NotImplementedError('Stub')


def ncchannels_set_fchannel(channels: int, channel: int, /) -> int:
    """Set the 32-bit foreground ncchannel of a channels pair."""
    raise NotImplementedError('Stub')


def ncchannels_combine(fchan: int, bchan: int, /) ->int:
    """Combine foreground and background channels in to channels pair."""
    raise NotImplementedError('Stub')


def ncchannels_fg_palindex(channels: int, /) -> int:
    """Extract the 2-bit palindex from foreground channel of channels pair."""
    raise NotImplementedError('Stub')


def ncchannels_bg_palindex(channels: int, /) -> int:
    """Extract the 2-bit palindex from background channel of channels pair."""
    raise NotImplementedError('Stub')


def ncchannels_fg_rgb(channels: int, /) -> int:
    """Extract 24 bits of foreground RGB from channels pair."""
    raise NotImplementedError('Stub')


def ncchannels_bg_rgb(channels: int, /) -> int:
    """Extract 24 bits of background RGB from channels pair."""
    raise NotImplementedError('Stub')


def ncchannels_fg_alpha(channels: int, /) -> int:
    """Extract the 2-bit alpha from foreground channel of channels pair."""
    raise NotImplementedError('Stub')


def ncchannels_bg_alpha(channels: int, /) -> int:
    """Extract the 2-bit alpha from background channel of channels pair."""
    raise NotImplementedError('Stub')


def ncchannels_fg_rgb8(channels: int, /) -> Tuple[int, int, int]:
    """Extract 24 bits of foreground RGB from channels pair, split into
    subncchannels."""
    raise NotImplementedError('Stub')


def ncchannels_bg_rgb8(channels: int, /) -> Tuple[int, int, int]:
    """Extract 24 bits of background RGB from channels pair, split into
    subncchannels."""
    raise NotImplementedError('Stub')


def ncchannels_set_fg_rgb8(channels: int, r: int, g: int, b: int, /) -> int:
    """Set the red, green and blue of foreground channel of channels pair."""
    raise NotImplementedError('Stub')


def ncchannels_set_fg_rgb8_clipped(channels: int, r: int, g: int, b: int,
                                   /) -> int:
    """Set the red, green and blue of foreground channel of channels pair.

    Clipped to 0..255
    """
    raise NotImplementedError('Stub')


def ncchannels_set_fg_alpha(channels: int, alpha: int, /) -> int:
    """Set the 2-bit alpha of the foreground channel of channels pair."""
    raise NotImplementedError('Stub')


def ncchannels_set_fg_palindex(channels: int, alpha: int, /) -> int:
    """Set the 2-bit paliindex of the foreground channel of channels pair."""
    raise NotImplementedError('Stub')


def ncchannels_set_fg_rgb(channels: int, rgb: int, /) -> int:
    """Set the RGB assembled 24 bit channel of the foreground channel of
    channels pair."""
    raise NotImplementedError('Stub')


def ncchannels_set_bg_rgb8(channels: int, r: int, g: int, b: int, /) -> int:
    """Set the red, green and blue of background channel of channels pair."""
    raise NotImplementedError('Stub')


def ncchannels_set_bg_rgb8_clipped(channels: int, r: int, g: int, b: int,
                                   /) -> int:
    """Set the red, green and blue of background channel of channels pair.

    Clipped to 0..255
    """
    raise NotImplementedError('Stub')


def ncchannels_set_bg_alpha(channels: int, alpha: int, /) -> int:
    """Set the 2-bit alpha of the background channel of channels pair."""
    raise NotImplementedError('Stub')


def ncchannels_set_bg_palindex(channels: int, alpha: int, /) -> int:
    """Set the 2-bit paliindex of the background channel of channels pair."""
    raise NotImplementedError('Stub')


def ncchannels_set_bg_rgb(channels: int, rgb: int, /) -> int:
    """Set the RGB assembled 24 bit channel of the background channel of
    channels pair."""
    raise NotImplementedError('Stub')


def ncchannels_fg_default_p(channels: int, /) ->bool:
    """Is the foreground using the default color?"""
    raise NotImplementedError('Stub')


def ncchannels_fg_palindex_p(channels: int, /) ->bool:
    """Is the foreground using indexed palette color?"""
    raise NotImplementedError('Stub')


def ncchannels_bg_default_p(channels: int, /) ->bool:
    """Is the foreground using the default color?"""
    raise NotImplementedError('Stub')


def ncchannels_bg_palindex_p(channels: int, /) ->bool:
    """Is the foreground using indexed palette color?"""
    raise NotImplementedError('Stub')


def ncchannels_set_fg_default(channels: int, /) -> int:
    """Mark the foreground ncchannel as using its default color."""
    raise NotImplementedError('Stub')


def ncchannels_set_bg_default(channels: int, /) -> int:
    """Mark the background ncchannel as using its default color."""
    raise NotImplementedError('Stub')

# endregion ncchannel


class NcInput:
    id: int
    y: int
    x: int
    utf8: str
    evtype: int
    modifiers: int
    xpx: int
    ypx: int


class Notcurses:
    """Notcurses context."""

    def __init__(
            self,
            tty_fd: int = 0,
            term_type: str = "",
            renderfd: int = 0,
            loglevel: int = 0,
            margins_str: str = "",
            margin_t: int = 0, margin_r: int = 0,
            margin_b: int = 0, margin_l: int = 0,
            flags: int = 0):
        """Initialize new Notcruses context."""
        raise NotImplementedError('Stub')

    def drop_planes(self) -> None:
        """Destroy all ncplanes other than the stdplane."""
        raise NotImplementedError('Stub')

    def render(self) -> None:
        """Renders and rasterizes the standard pile in one shot.

        Blocking call.
        """
        raise NotImplementedError('Stub')

    def top(self) -> NcPlane:
        """Return the topmost ncplane of the standard pile."""
        raise NotImplementedError('Stub')

    def bottom(self) -> NcPlane:
        """Return the bottommost ncplane of the standard pile."""
        raise NotImplementedError('Stub')

    def inputready_fd(self) -> int:
        """Get a file descriptor suitable for input event poll."""
        raise NotImplementedError('Stub')

    def get(self, deadline: Optional[float]) -> NcInput:
        """Reads an input event. If no event is ready, returns None."""
        raise NotImplementedError('Stub')

    def get_nblock(self) -> NcInput:
        """Get input event without blocking. If no event is ready,
        returns None."""
        raise NotImplementedError('Stub')

    def get_blocking(self) -> NcInput:
        """Get input event completely blocking until and event or signal
        received."""
        raise NotImplementedError('Stub')

    def mouse_enable(self) -> None:
        """Enable the mouse in \"button-event tracking\" mode with
        focus detection and UTF8-style extended coordinates."""
        raise NotImplementedError('Stub')

    def mouse_disable(self) -> None:
        """Disable mouse events.

        Any events in the input queue can still be delivered.
        """
        raise NotImplementedError('Stub')

    def linesigs_disable(self) -> None:
        """Disable signals originating from the terminal's line discipline.

        They are enabled by default.
        """
        raise NotImplementedError('Stub')

    def linesigs_enable(self) -> None:
        """Restore signals originating from the terminal's line discipline."""
        raise NotImplementedError('Stub')

    def refresh(self) -> Tuple[int, int]:
        """Refresh the physical screen to match what was last rendered.

        Return the new dimensions.
        """
        raise NotImplementedError('Stub')

    def stdplane(self) -> NcPlane:
        """Get a reference to the standard plane.

        The standard plane always exists, and its origin is always at
        the uppermost, leftmost cell of the terminal.
        """
        raise NotImplementedError('Stub')

    def stddim_yx(self) -> Tuple[NcPlane, int, int]:
        """Get standard plane plus dimensions."""
        raise NotImplementedError('Stub')

    def term_dim_yx(self) -> Tuple[int, int]:
        """Return our current idea of the terminal dimensions
        in rows and cols."""
        raise NotImplementedError('Stub')

    def at_yx(self, yoff: int, xoff: int, /) -> Tuple[int, int]:
        """Retrieve the contents of the specified cells stylemask and channels
        as last rendered"""
        raise NotImplementedError('Stub')

    def pile_create(self, y_pos: int = 0, x_pos: int = 0,
                    rows: int = 0, cols: int = 0,
                    name: str = "",
                    flags: int = 0,
                    margin_b: int = 0, margin_r: int = 0) -> NcPlane:
        """Same as ncplane_create(), but creates a new pile.

        The returned plane will be the top, bottom, and root of this new pile.
        """
        raise NotImplementedError('Stub')

    def supported_styles(self) -> int:
        """Returns a 16-bit bitmask of supported curses-style attributes."""
        raise NotImplementedError('Stub')

    def palette_size(self) -> int:
        """Returns the number of simultaneous colors claimed
        to be supported."""
        raise NotImplementedError('Stub')

    def cantruecolor(self) -> bool:
        """Can we directly specify RGB values per cell,
        or only use palettes?"""
        raise NotImplementedError('Stub')

    def canfade(self) -> bool:
        """Can we fade?"""
        raise NotImplementedError('Stub')

    def canchangecolor(self) -> bool:
        """Can we set the \"hardware\" palette? """
        raise NotImplementedError('Stub')

    def canopen_images(self) -> bool:
        """Can we load images?

        This requires being built against FFmpeg/OIIO.
        """
        raise NotImplementedError('Stub')

    def canopen_videos(self) -> bool:
        """Can we load videos?

        This requires being built against FFmpeg.
        """
        raise NotImplementedError('Stub')

    def canutf8(self) -> bool:
        """Is our encoding UTF-8?

        Requires LANG being set to a UTF8 locale.
        """
        raise NotImplementedError('Stub')

    def cansextant(self) -> bool:
        """Can we reliably use Unicode 13 sextants?"""
        raise NotImplementedError('Stub')

    def canbraille(self) -> bool:
        """Can we reliably use Unicode Braille?"""
        raise NotImplementedError('Stub')

    def check_pixel_support(self) -> bool:
        """This function must successfully return before NCBLIT_PIXEL
        is available.
        """
        raise NotImplementedError('Stub')

    def stats(self) -> None:
        """Acquire an atomic snapshot of the Notcurses stats."""
        raise NotImplementedError('Stub')

    def stats_reset(self) -> None:
        """Reset all cumulative stats (immediate ones, such as fbbytes,
        are not reset) and returning a copy before reset."""
        raise NotImplementedError('Stub')

    def cursor_enable(self) -> None:
        """Enable the terminal's cursor, if supported"""
        raise NotImplementedError('Stub')

    def cursor_disable(self) -> None:
        """Disable the terminal's cursor."""
        raise NotImplementedError('Stub')


class NcPlane:
    """Notcurses Plane"""

    def create(self, rows: int, cols: int,
               y_pos: int = 0, x_pos: int = 0,
               name: str = "",
               flags: int = 0,
               margin_b: int = 0, margin_r: int = 0,) -> None:
        """Create a new ncplane bound to this plane."""
        raise NotImplementedError('Stub')

    def destroy(self) -> None:
        """Destroy the plane.

        Do NOT try to use the plane after its been
        destoryed.
        """
        raise NotImplementedError('Stub')

    def dim_yx(self) -> Tuple[int, int]:
        """Return the dimensions of this NcPlane."""
        raise NotImplementedError('Stub')

    def dim_x(self) -> int:
        """Return X dimension of this NcPlane."""
        raise NotImplementedError('Stub')

    def dim_y(self) -> int:
        """Return Y dimension of this NcPlane."""
        raise NotImplementedError('Stub')

    def pixel_geom(self) -> Tuple[int, int, int, int, int, int]:
        """Retrieve pixel geometry for the display region, each cell
        and the maximum displayable bitmap."""
        raise NotImplementedError('Stub')

    def set_resizecb(self) -> None:
        """Replace the ncplane's existing resize callback

        TODO
        """
        raise NotImplementedError('Stub')

    def reparent(self, new_parent: NcPlane) -> None:
        """Plane will be unbound from its parent plane
        and will be made a bound child of passed plane."""
        raise NotImplementedError('Stub')

    def reparent_family(self, new_parent: NcPlane) -> None:
        """The same as reparent(), except any planes bound
        to this plane come along with it to its new destination."""
        raise NotImplementedError('Stub')

    def dup(self) -> NcPlane:
        """Duplicate an existing ncplane."""
        raise NotImplementedError('Stub')

    def translate(self,
                  another_plane: NcPlane,
                  /) -> Tuple[int, int]:
        """Return coordinates relative to another plane."""
        raise NotImplementedError('Stub')

    def translate_abs(self) -> bool:
        """Check if coordinates are in the plane."""
        raise NotImplementedError('Stub')

    def set_scrolling(self, state: bool, /) -> None:
        """All planes are created with scrolling disabled."""
        raise NotImplementedError('Stub')

    def resize(self, keepy: int, keepx: int,
               keepleny: int, keeplenx: int,
               yoff: int, xoff: int,
               ylen: int, xlen: int,) -> None:
        """Resize the ncplane."""
        raise NotImplementedError('Stub')

    def resize_simple(self, ylen: int, xlen: int, /) -> None:
        """Resize the plane, retaining what data we can.

        Keep the origin where it is.
        """
        raise NotImplementedError('Stub')

    def set_base_cell(self) -> None:
        """Set the plane's base NcCell.

        TODO
        """
        raise NotImplementedError('Stub')

    def set_base(self) -> None:
        """Set the plane's base NcCell.

        TODO
        """
        raise NotImplementedError('Stub')

    def base(self) -> None:
        """Extract the plane's base NcCell.

        TODO
        """
        raise NotImplementedError('Stub')

    def move_yx(self, y: int, x: int, /) -> None:
        """Move this plane relative to the standard plane,
        or the plane to which it is bound."""
        raise NotImplementedError('Stub')

    def yx(self) -> Tuple[int, int]:
        """Get the origin of the plane relative to its
        bound plane, or pile."""
        raise NotImplementedError('Stub')

    def y(self) -> int:
        """Get the Y origin of the plane relative to its
        bound plane, or pile."""
        raise NotImplementedError('Stub')

    def x(self) -> int:
        """Get the X origin of the plane relative to its
        bound plane, or pile."""
        raise NotImplementedError('Stub')

    def abs_yx(self) -> Tuple[int, int]:
        """Get the origin of plane relative to its pile."""
        raise NotImplementedError('Stub')

    def abs_y(self) -> int:
        """Get the Y origin of plane relative to its pile."""
        raise NotImplementedError('Stub')

    def abs_x(self) -> int:
        """Get the X origin of plane relative to its pile."""
        raise NotImplementedError('Stub')

    def parent(self) -> Optional[NcPlane]:
        """Get the plane to which the plane is bound or None
        if plane does not have parent.
        """
        raise NotImplementedError('Stub')

    def descendant_p(self, ancestor: NcPlane, /) -> None:
        """Return True if plane is a proper descendent of passed
        'ancestor' plane.
        """
        raise NotImplementedError('Stub')

    def move_top(self) -> None:
        """Splice ncplane out of the z-buffer, and reinsert it at the top."""
        raise NotImplementedError('Stub')

    def move_bottom(self) -> None:
        """Splice ncplane out of the z-buffer, and reinsert it at
        the bottom.
        """
        raise NotImplementedError('Stub')

    def move_above(self, plane: NcPlane, /) -> None:
        """Splice ncplane out of the z-buffer, and reinsert it above passed
        plane."""
        raise NotImplementedError('Stub')

    def move_below(self, plane: NcPlane, /) -> None:
        """Splice ncplane out of the z-buffer, and reinsert it bellow
        passed plane.
        """
        raise NotImplementedError('Stub')

    def below(self) -> Optional[NcPlane]:
        """Return the plane below this one, or None if this is
        at the bottom.
        """
        raise NotImplementedError('Stub')

    def above(self) -> Optional[NcPlane]:
        """Return the plane above this one, or None if this is at the top."""
        raise NotImplementedError('Stub')

    def rotate_cw(self) -> None:
        """Rotate the plane π/2 radians clockwise."""
        raise NotImplementedError('Stub')

    def rotate_ccw(self) -> None:
        """Rotate the plane π/2 radians counterclockwise."""
        raise NotImplementedError('Stub')

    def at_cursor(self) -> Tuple[str, int, int]:
        """Retrieve the current contents of the cell under the cursor."""
        raise NotImplementedError('Stub')

    def at_cursor_cell(self) -> None:
        """Retrieve the current contents of the cell under the cursor.

        TODO
        """
        raise NotImplementedError('Stub')

    def at_yx(self, y: int, x: int, /) -> Tuple[str, int, int]:
        """Retrieve the current contents of the specified cell."""
        raise NotImplementedError('Stub')

    def at_yx_cell(self) -> None:
        """"Retrieve the current contents of the specified cell"

        TODO
        """
        raise NotImplementedError('Stub')

    def contents(self, begy: int, begx: int,
                 leny: int = -1, lenx: int = -1) -> str:
        """Create a flat string from the EGCs of the selected region
        of the ncplane.
        """
        raise NotImplementedError('Stub')

    def center_abs(self) -> Tuple[int, int]:
        """Return the plane center absolute coordiantes."""
        raise NotImplementedError('Stub')

    def halign(self, align: int, collumn: int, /) -> int:
        """Return the column at which cols ought start in
        order to be aligned.
        """
        raise NotImplementedError('Stub')

    def valign(self, align: int, collumn: int, /) -> int:
        """Return the row at which rows ought start in
        order to be aligned.
        """
        raise NotImplementedError('Stub')

    def cursor_move_yx(self, y: int, x: int, /) -> None:
        """Move the cursor to the specified position
        (the cursor needn't be visible).
        """
        raise NotImplementedError('Stub')

    def home(self) -> None:
        """Move the cursor to 0, 0."""
        raise NotImplementedError('Stub')

    def cursor_yx(self) -> Tuple[int, int]:
        """Get the current position of the cursor within plane."""
        raise NotImplementedError('Stub')

    def channels(self) -> int:
        """Get the current channels or attribute word."""
        raise NotImplementedError('Stub')

    def styles(self) -> int:
        """Return the current styling for this ncplane."""
        raise NotImplementedError('Stub')

    def putc_yx(self) -> None:
        """Replace the cell at the specified coordinates with
        the provided cell.

        TODO
        """
        raise NotImplementedError('Stub')

    def putc(self) -> None:
        """Replace cell at the current cursor location.

        TODO
        """
        raise NotImplementedError('Stub')

    def putchar_yx(self, y: int, x: int, char: str, /) -> None:
        """Replace the cell at the specified coordinates
        with the provided 7-bit char."""
        raise NotImplementedError('Stub')

    def putchar(self, char: str) -> None:
        """Replace the cell at the current cursor location."""
        raise NotImplementedError('Stub')

    def putchar_stained(self, char: str, /) -> None:
        """Replace the EGC underneath us, but retain the styling."""
        raise NotImplementedError('Stub')

    def putegc_yx(self, y: int, x: int, egc: str, /) -> int:
        """Replace the cell at the specified coordinates
        with the provided EGC.

        Returns number of collumns the cursor has advanced.
        """
        raise NotImplementedError('Stub')

    def putegc(self, egc: str, /) -> int:
        """Replace the cell at the current cursor location
        with the provided EGC

        Returns number of collumns the cursor has advanced.
        """
        raise NotImplementedError('Stub')

    def putegc_stained(self, egc: str, /) -> int:
        """Replace the EGC underneath us, but retain the styling.

        Returns number of collumns the cursor has advanced.
        """
        raise NotImplementedError('Stub')

    def putstr_yx(self, y: int, x: int, egc: str, /) -> None:
        """Write a series of EGCs to the location,
        using the current style.
        """
        raise NotImplementedError('Stub')

    def putstr(self, egc: str, /) -> None:
        """Write a series of EGCs to the current location,
        using the current style.
        """
        raise NotImplementedError('Stub')

    def putstr_aligned(self, y: int, align: int, egc: str, /) -> None:
        """Write a series of EGCs to the current location,
        using the alignment.
        """
        raise NotImplementedError('Stub')

    def putstr_stained(self, egc: str, /) -> None:
        """Replace a string's worth of glyphs at the current
        cursor location, but retain the styling."""
        raise NotImplementedError('Stub')

    def putnstr_yx(self, y: int, x: int, size: int, egc: str, /) -> None:
        """"Write a series of EGCs to the location, using the current
        style.
        """
        raise NotImplementedError('Stub')

    def putnstr(self, size: int, egc: str, /) -> None:
        """Write a series of EGCs to the current location,
        using the current style.
        """
        raise NotImplementedError('Stub')

    def putnstr_aligned(self, y: int, align: int,
                        size: int, egc: str, /) -> None:
        """Write a series of EGCs to the current location,
        using the alignment.
        """
        raise NotImplementedError('Stub')

    def puttext(self, y: int, align: int, /) -> int:
        """Write the specified text to the plane, breaking lines sensibly,
        beginning at the specified line.

        Returns the number of columns written.
        """
        raise NotImplementedError('Stub')

    def box(self) -> None:
        """Draw a box with its upper-left corner
        at the current cursor position.

        TODO
        """
        raise NotImplementedError('Stub')

    def box_sized(self) -> None:
        """Draw a box with its upper-left corner at
        the current cursor position, having dimensions.

        TODO
        """
        raise NotImplementedError('Stub')

    def perimeter(self) -> None:
        """Draw a perimeter with its upper-left corner
        at the current cursor position

        TODO
        """
        raise NotImplementedError('Stub')

    def polyfill_yx(self) -> None:
        """Starting at the specified coordinate, if its glyph
        is different from that of is copied into it, and the
        original glyph is considered the fill target.

        TODO
        """
        raise NotImplementedError('Stub')

    def gradient(self, egc: str, stylemask: int,
                 ul: int, ur: int, ll: int, lr: int,
                 ystop: int, xstop: int,) -> int:
        """Draw a gradient with its upper-left corner
        at the current cursor position.

        Returns cells filled.
        """
        raise NotImplementedError('Stub')

    def highgradient(self,
                     ul: int, ur: int, ll: int, lr: int,
                     ystop: int, xstop: int) -> int:
        """Do a high-resolution gradient using upper blocks
        and synced backgrounds.

        Returns cells filled.
        """
        raise NotImplementedError('Stub')

    def gradient_sized(self, egc: str, stylemask: int,
                       ul: int, ur: int, ll: int, lr: int,
                       ylen: int, xlen: int,) -> int:
        """Draw a gradient with its upper-left corner
        at the current cursor position.

        Returns cells filled.
        """
        raise NotImplementedError('Stub')

    def highgradient_sized(self,
                           ul: int, ur: int, ll: int, lr: int,
                           ylen: int, xlen: int,) -> int:
        """NcPlane.gradent_sized() meets NcPlane.highgradient().

        Returns cells filled.
        """
        raise NotImplementedError('Stub')

    def format(self, ystop: int, xstop: int,
               stylemark: int, /) -> int:
        """Set the given style throughout the specified region,
        keeping content and attributes unchanged.

        Returns the number of cells set.
        """
        raise NotImplementedError('Stub')

    def stain(self, ystop: int, xstop: int,
              ul: int, ur: int, ll: int, lr: int) -> int:
        """Set the given style throughout the specified region,
        keeping content and attributes unchanged.

        Returns the number of cells set.
        """
        raise NotImplementedError('Stub')

    def mergedown_simple(self, dst: NcPlane, /) -> None:
        """Merge the ncplane down onto the passed ncplane."""
        raise NotImplementedError('Stub')

    def mergedown(self, dst: NcPlane,
                  begsrcy: int = 0, begsrcx: int = 0,
                  leny: int = 0, lenx: int = 0,
                  dsty: int = 0, dstx: int = 0) -> None:
        """Merge with parameters the ncplane down onto the passed ncplane."""
        raise NotImplementedError('Stub')

    def erase(self) -> None:
        """Erase every cell in the ncplane."""
        raise NotImplementedError('Stub')

    def bchannel(self) -> int:
        """Extract the 32-bit working background channel from an ncplane."""
        raise NotImplementedError('Stub')

    def fchannel(self) -> int:
        """Extract the 32-bit working foreground channel from an ncplane."""
        raise NotImplementedError('Stub')

    def set_channels(self, channels: int, /) -> None:
        """Set both foreground and background channels of the plane."""
        raise NotImplementedError('Stub')

    def set_styles(self, stylebits: int, /) -> None:
        """Set the specified style bits for the plane,
        whether they're actively supported or not.
        """
        raise NotImplementedError('Stub')

    def on_styles(self, stylebits: int, /) -> None:
        """Add the specified styles to the ncplane's existing spec."""
        raise NotImplementedError('Stub')

    def off_styles(self, stylebits: int, /) -> None:
        """Remove the specified styles from the ncplane's existing spec."""
        raise NotImplementedError('Stub')

    def fg_rgb(self) -> int:
        """Extract 24 bits of working foreground RGB from the plane,
        shifted to LSBs.
        """
        raise NotImplementedError('Stub')

    def bg_rgb(self) -> int:
        """Extract 24 bits of working background RGB from the plane,
        shifted to LSBs."""
        raise NotImplementedError('Stub')

    def fg_alpha(self) -> int:
        """Extract 2 bits of foreground alpha from plane,
        shifted to LSBs.
        """
        raise NotImplementedError('Stub')

    def fg_default_p(self) -> bool:
        """Is the plane's foreground using the \"default foreground color\"?"""
        raise NotImplementedError('Stub')

    def bg_alpha(self) -> int:
        """Extract 2 bits of background alpha from the plane,
        shifted to LSBs.
        """
        raise NotImplementedError('Stub')

    def bg_default_p(self) -> bool:
        """Is the plane's background using the \"default background color\"?"""
        raise NotImplementedError('Stub')

    def fg_rgb8(self) -> Tuple[int, int, int]:
        """Extract 24 bits of foreground RGB from the plane,
        split into components.
        """
        raise NotImplementedError('Stub')

    def bg_rgb8(self) -> Tuple[int, int, int]:
        """Is the plane's background using the \"default background color\"?"""
        raise NotImplementedError('Stub')

    def set_fchannel(self, channel: int, /) -> int:
        """Set an entire foreground channel of the plane,
        return new channels.
        """
        raise NotImplementedError('Stub')

    def set_bchannel(self, channel: int, /) -> int:
        """Set an entire background channel of the plane,
        return new channels.
        """
        raise NotImplementedError('Stub')

    def set_fg_rgb8(self, red: int, green: int, blue: int, /) -> None:
        """Set the current foreground color using RGB specifications."""
        raise NotImplementedError('Stub')

    def set_bg_rgb8(self, red: int, green: int, blue: int, /) -> None:
        """Set the current background color using RGB specifications."""
        raise NotImplementedError('Stub')

    def set_bg_rgb8_clipped(self, red: int, green: int, blue: int, /) -> None:
        """Set the current foreground color using RGB specifications
        but clipped to [0..255]."""
        raise NotImplementedError('Stub')

    def set_fg_rgb8_clipped(self, red: int, green: int, blue: int, /) -> None:
        """Set the current background color using RGB specifications
        but clipped to [0..255]."""
        raise NotImplementedError('Stub')

    def set_fg_rgb(self, channel: int, /) -> None:
        """Set the current foreground color using channel."""
        raise NotImplementedError('Stub')

    def set_bg_rgb(self, channel: int, /) -> None:
        """Set the current background color using channel."""
        raise NotImplementedError('Stub')

    def set_fg_default(self) -> None:
        """Use the default color for the foreground."""
        raise NotImplementedError('Stub')

    def set_bg_default(self) -> None:
        """Use the default color for the background."""
        raise NotImplementedError('Stub')

    def set_fg_palindex(self, idx: int, /) -> None:
        """Set the ncplane's foreground palette index."""
        raise NotImplementedError('Stub')

    def set_bg_palindex(self, idx: int, /) -> None:
        """Set the ncplane's background palette index."""
        raise NotImplementedError('Stub')

    def set_fg_alpha(self, aplha: int, /) -> None:
        """Set the foreground alpha parameters for the plane."""
        raise NotImplementedError('Stub')

    def set_bg_alpha(self, aplha: int, /) -> None:
        """Set the current background color using channel."""
        raise NotImplementedError('Stub')

    def fadeout(self) -> None:
        """Fade the ncplane out over the provided time,
        calling 'fader' at each iteration.

        TODO
        """
        raise NotImplementedError('Stub')

    def fadein(self) -> None:
        """Fade the ncplane in over the specified time.

        TODO
        """
        raise NotImplementedError('Stub')

    def fade_setup(self) -> None:
        """Create NcFadeCtx.

        TODO
        """
        raise NotImplementedError('Stub')

    def fadeout_iteration(self) -> None:
        """TODO"""
        raise NotImplementedError('Stub')

    def fadein_iteration(self) -> None:
        """TODO"""
        raise NotImplementedError('Stub')

    def pulse(self) -> None:
        """Pulse the plane in and out until the callback returns non-zero.

        TODO
        """
        raise NotImplementedError('Stub')

    def cells_load_box(self) -> None:
        """Load up six cells with the EGCs necessary to draw a box.

        TODO
        """
        raise NotImplementedError('Stub')

    def cells_rounded_box(self) -> None:
        """Load up six cells with the EGCs necessary to draw a round box.

        TODO
        """
        raise NotImplementedError('Stub')

    def perimeter_rounded(
            self,
            stylemask: int, channels: int,
            ctlword: int,) -> None:
        """Draw a perimeter around plane."""
        raise NotImplementedError('Stub')

    def rounded_box_sized(self,
                          styles: int, channels: int,
                          ylen: int, xlen: int,
                          ctlword: int) -> None:
        """Draw a round box around plane."""
        raise NotImplementedError('Stub')

    def cells_double_box(self) -> None:
        """Draw a double box with cells.

        TODO
        """
        raise NotImplementedError('Stub')

    def double_box(self,
                   styles: int, channels: int,
                   ylen: int, xlen: int,
                   ctlword: int) -> None:
        """Draw a double box."""
        raise NotImplementedError('Stub')

    def perimeter_double(self,
                         styles: int, channels: int,
                         ctlword: int) -> None:
        """Draw a double perimeter."""
        raise NotImplementedError('Stub')

    def double_box_sized(self,
                         styles: int, channels: int,
                         ylen: int, xlen: int,
                         ctlword: int) -> None:
        """Draw a double box sized."""
        raise NotImplementedError('Stub')

    def ncvisual_from_plane(self) -> None:
        """Promote an plane to an NcVisual.

        TODO
        """
        raise NotImplementedError('Stub')

    def as_rgba(self) -> None:
        """Create an RGBA flat array from the selected
        region of the plane.

        TODO
        """
        raise NotImplementedError('Stub')

    def reel_create(self) -> None:
        """Take over the plane and use it to draw a reel.

        TODO
        """
        raise NotImplementedError('Stub')

    def greyscale(self) -> None:
        """Convert the plane's content to greyscale."""
        raise NotImplementedError('Stub')

    def selector_create(self) -> None:
        """Create NcSelector.

        TODO
        """
        raise NotImplementedError('Stub')

    def multiselector_create(self) -> None:
        """Create MultiSelector.

        TODO
        """
        raise NotImplementedError('Stub')

    def tree_create(self) -> None:
        """Create NcTree.

        TODO
        """
        raise NotImplementedError('Stub')

    def menu_create(self) -> None:
        """Create NcMenu.

        TODO
        """
        raise NotImplementedError('Stub')

    def progbar_create(self) -> None:
        """Create NcProgbar.

        TODO
        """
        raise NotImplementedError('Stub')

    def tabbed_create(self) -> None:
        """Create NcTabbed.

        TODO
        """
        raise NotImplementedError('Stub')

    def uplot_create(self) -> None:
        """Create NcUplot.

        TODO
        """
        raise NotImplementedError('Stub')

    def dplot_create(self) -> None:
        """Create NcDplot.

        TODO
        """
        raise NotImplementedError('Stub')

    def fdplane_create(self) -> None:
        """Create NcFdPlane.

        TODO
        """
        raise NotImplementedError('Stub')

    def subproc_createv(self) -> None:
        """Create subprocess plane.

        TODO
        """
        raise NotImplementedError('Stub')

    def subproc_createvp(self) -> None:
        """Create subprocess plane.

        TODO
        """
        raise NotImplementedError('Stub')

    def subproc_createvpe(self) -> None:
        """Create subprocess plane.

        TODO
        """
        raise NotImplementedError('Stub')

    def qrcode(self, data: bytes, /) -> Tuple[int, int]:
        """Create QR code, return y and x size."""
        raise NotImplementedError('Stub')

    def reader_create(self) -> None:
        """Create NcReader.

        TODO
        """
        raise NotImplementedError('Stub')

    def pile_top(self) -> NcPlane:
        """Return the topmost plane of the pile."""
        raise NotImplementedError('Stub')

    def pile_bottom(self) -> NcPlane:
        """Return the bottommost plane of the pile."""
        raise NotImplementedError('Stub')

    def pile_render(self) -> None:
        """Renders the pile of which plane is a part."""
        raise NotImplementedError('Stub')

    def pile_rasterize(self) -> None:
        """Make the physical screen match the last
        rendered frame from the pile."""
        raise NotImplementedError('Stub')

    def pile_render_to_buffer(self) -> bytes:
        """Perform the rendering and rasterization portion of render()
        and write it to bytes object instead of terminal."""
        raise NotImplementedError('Stub')

    def pile_render_to_file(self, fd: int, /) -> None:
        """Write the last rendered frame, in its entirety, to file descriptor.

        If render() has not yet been called, nothing will be written.
        """
        raise NotImplementedError('Stub')

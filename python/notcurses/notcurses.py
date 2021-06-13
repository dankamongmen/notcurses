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

from typing import Tuple

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

    def render_to_buffer(self) -> bytes:
        """Perform the rendering and rasterization portion of render()
        and write it to bytes object instead of terminal."""
        raise NotImplementedError('Stub')

    def render_to_file(self, fd: int, /) -> None:
        """Write the last rendered frame, in its entirety, to file descriptor.

        If render() has not yet been called, nothing will be written.
        """
        raise NotImplementedError('Stub')

    def top(self) -> NcPlane:
        """Return the topmost ncplane of the standard pile."""
        raise NotImplementedError('Stub')

    def bottom(self) -> NcPlane:
        """Return the bottommost ncplane of the standard pile."""
        raise NotImplementedError('Stub')

    def getc(self) -> str:
        """Get a single unicode codepoint of input."""
        raise NotImplementedError('Stub')

    def inputready_fd(self) -> int:
        """Get a file descriptor suitable for input event poll."""
        raise NotImplementedError('Stub')

    def getc_nblock(self) -> str:
        """Get input event without blocking. If no event is ready,
        returns None."""
        raise NotImplementedError('Stub')

    def getc_blocking(self) -> str:
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

    def pixelgeom(self) -> Tuple[int, int, int, int, int, int]:
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

    def abs_yx(self) -> None:
        """Get the origin of plane 'n' relative to its pile."""
        raise NotImplementedError('Stub')

    def abs_y(self) -> None:
        raise NotImplementedError('Stub')

    def abs_x(self) -> None:
        raise NotImplementedError('Stub')

    def parent(self) -> None:
        raise NotImplementedError('Stub')

    def descendant_p(self) -> None:
        raise NotImplementedError('Stub')

    def move_top(self) -> None:
        raise NotImplementedError('Stub')

    def move_bottom(self) -> None:
        raise NotImplementedError('Stub')

    def move_above(self) -> None:
        raise NotImplementedError('Stub')

    def move_below(self) -> None:
        raise NotImplementedError('Stub')

    def below(self) -> None:
        raise NotImplementedError('Stub')

    def above(self) -> None:
        raise NotImplementedError('Stub')

    def rotate_cw(self) -> None:
        raise NotImplementedError('Stub')

    def rotate_ccw(self) -> None:
        raise NotImplementedError('Stub')

    def at_cursor(self) -> None:
        raise NotImplementedError('Stub')

    def at_cursor_cell(self) -> None:
        raise NotImplementedError('Stub')

    def at_yx(self) -> None:
        raise NotImplementedError('Stub')

    def at_yx_cell(self) -> None:
        raise NotImplementedError('Stub')

    def contents(self) -> None:
        raise NotImplementedError('Stub')

    def center_abs(self) -> None:
        raise NotImplementedError('Stub')

    def halign(self) -> None:
        raise NotImplementedError('Stub')

    def valign(self) -> None:
        raise NotImplementedError('Stub')

    def cursor_move_yx(self) -> None:
        raise NotImplementedError('Stub')

    def home(self) -> None:
        raise NotImplementedError('Stub')

    def cursor_yx(self) -> None:
        raise NotImplementedError('Stub')

    def channels(self) -> None:
        raise NotImplementedError('Stub')

    def styles(self) -> None:
        raise NotImplementedError('Stub')

    def putc_yx(self) -> None:
        raise NotImplementedError('Stub')

    def putc(self) -> None:
        raise NotImplementedError('Stub')

    def putchar_yx(self) -> None:
        raise NotImplementedError('Stub')

    def putchar(self) -> None:
        raise NotImplementedError('Stub')

    def putchar_stained(self) -> None:
        raise NotImplementedError('Stub')

    def putegc_yx(self) -> None:
        raise NotImplementedError('Stub')

    def putegc(self) -> None:
        raise NotImplementedError('Stub')

    def putegc_stained(self) -> None:
        raise NotImplementedError('Stub')

    def putstr_yx(self) -> None:
        raise NotImplementedError('Stub')

    def putstr(self) -> None:
        raise NotImplementedError('Stub')

    def putstr_aligned(self) -> None:
        raise NotImplementedError('Stub')

    def putstr_stained(self) -> None:
        raise NotImplementedError('Stub')

    def putnstr_yx(self) -> None:
        raise NotImplementedError('Stub')

    def putnstr(self) -> None:
        raise NotImplementedError('Stub')

    def putnstr_aligned(self) -> None:
        raise NotImplementedError('Stub')

    def puttext(self) -> None:
        raise NotImplementedError('Stub')

    def box(self) -> None:
        raise NotImplementedError('Stub')

    def box_sized(self) -> None:
        raise NotImplementedError('Stub')

    def perimeter(self) -> None:
        raise NotImplementedError('Stub')

    def polyfill_yx(self) -> None:
        raise NotImplementedError('Stub')

    def gradient(self) -> None:
        raise NotImplementedError('Stub')

    def highgradient(self) -> None:
        raise NotImplementedError('Stub')

    def gradient_sized(self) -> None:
        raise NotImplementedError('Stub')

    def highgradient_sized(self) -> None:
        raise NotImplementedError('Stub')

    def format(self) -> None:
        raise NotImplementedError('Stub')

    def stain(self) -> None:
        raise NotImplementedError('Stub')

    def mergedown_simple(self) -> None:
        raise NotImplementedError('Stub')

    def mergedown(self) -> None:
        raise NotImplementedError('Stub')

    def erase(self) -> None:
        raise NotImplementedError('Stub')

    def bchannel(self) -> None:
        raise NotImplementedError('Stub')

    def fchannel(self) -> None:
        raise NotImplementedError('Stub')

    def set_channels(self) -> None:
        raise NotImplementedError('Stub')

    def set_styles(self) -> None:
        raise NotImplementedError('Stub')

    def on_styles(self) -> None:
        raise NotImplementedError('Stub')

    def off_styles(self) -> None:
        raise NotImplementedError('Stub')

    def fg_rgb(self) -> None:
        raise NotImplementedError('Stub')

    def bg_rgb(self) -> None:
        raise NotImplementedError('Stub')

    def fg_alpha(self) -> None:
        raise NotImplementedError('Stub')

    def fg_default_p(self) -> None:
        raise NotImplementedError('Stub')

    def bg_alpha(self) -> None:
        raise NotImplementedError('Stub')

    def bg_default_p(self) -> None:
        raise NotImplementedError('Stub')

    def fg_rgb8(self) -> None:
        raise NotImplementedError('Stub')

    def bg_rgb8(self) -> None:
        raise NotImplementedError('Stub')

    def set_fchannel(self) -> None:
        raise NotImplementedError('Stub')

    def set_bchannel(self) -> None:
        raise NotImplementedError('Stub')

    def set_fg_rgb8(self) -> None:
        raise NotImplementedError('Stub')

    def set_bg_rgb8(self) -> None:
        raise NotImplementedError('Stub')

    def set_bg_rgb8_clipped(self) -> None:
        raise NotImplementedError('Stub')

    def set_fg_rgb8_clipped(self) -> None:
        raise NotImplementedError('Stub')

    def set_fg_rgb(self) -> None:
        raise NotImplementedError('Stub')

    def set_bg_rgb(self) -> None:
        raise NotImplementedError('Stub')

    def set_fg_default(self) -> None:
        raise NotImplementedError('Stub')

    def set_bg_default(self) -> None:
        raise NotImplementedError('Stub')

    def set_fg_palindex(self) -> None:
        raise NotImplementedError('Stub')

    def set_bg_palindex(self) -> None:
        raise NotImplementedError('Stub')

    def set_fg_alpha(self) -> None:
        raise NotImplementedError('Stub')

    def set_bg_alpha(self) -> None:
        raise NotImplementedError('Stub')

    def fadeout(self) -> None:
        raise NotImplementedError('Stub')

    def fadein(self) -> None:
        raise NotImplementedError('Stub')

    def fade_setup(self) -> None:
        raise NotImplementedError('Stub')

    def fadeout_iteration(self) -> None:
        raise NotImplementedError('Stub')

    def fadein_iteration(self) -> None:
        raise NotImplementedError('Stub')

    def pulse(self) -> None:
        raise NotImplementedError('Stub')

    def cells_load_box(self) -> None:
        raise NotImplementedError('Stub')

    def cells_rounded_box(self) -> None:
        raise NotImplementedError('Stub')

    def perimeter_rounded(self) -> None:
        raise NotImplementedError('Stub')

    def rounded_box_sized(self) -> None:
        raise NotImplementedError('Stub')

    def cells_double_box(self) -> None:
        raise NotImplementedError('Stub')

    def double_box(self) -> None:
        raise NotImplementedError('Stub')

    def perimeter_double(self) -> None:
        raise NotImplementedError('Stub')

    def double_box_sized(self) -> None:
        raise NotImplementedError('Stub')

    def ncvisual_from_plane(self) -> None:
        raise NotImplementedError('Stub')

    def as_rgba(self) -> None:
        raise NotImplementedError('Stub')

    def reel_create(self) -> None:
        raise NotImplementedError('Stub')

    def greyscale(self) -> None:
        raise NotImplementedError('Stub')

    def selector_create(self) -> None:
        raise NotImplementedError('Stub')

    def multiselector_create(self) -> None:
        raise NotImplementedError('Stub')

    def tree_create(self) -> None:
        raise NotImplementedError('Stub')

    def menu_create(self) -> None:
        raise NotImplementedError('Stub')

    def progbar_create(self) -> None:
        raise NotImplementedError('Stub')

    def tabbed_create(self) -> None:
        raise NotImplementedError('Stub')

    def uplot_create(self) -> None:
        raise NotImplementedError('Stub')

    def dplot_create(self) -> None:
        raise NotImplementedError('Stub')

    def fdplane_create(self) -> None:
        raise NotImplementedError('Stub')

    def subproc_createv(self) -> None:
        raise NotImplementedError('Stub')

    def subproc_createvp(self) -> None:
        raise NotImplementedError('Stub')

    def subproc_createvpe(self) -> None:
        raise NotImplementedError('Stub')

    def qrcode(self) -> None:
        raise NotImplementedError('Stub')

    def reader_create(self) -> None:
        raise NotImplementedError('Stub')

    def pile_top(self) -> None:
        raise NotImplementedError('Stub')

    def pile_bottom(self) -> None:
        raise NotImplementedError('Stub')

    def pile_render(self) -> None:
        raise NotImplementedError('Stub')

    def pile_rasterize(self) -> None:
        raise NotImplementedError('Stub')

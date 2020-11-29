# SPDX-License-Identifier: Apache-2.0

# Copyright 2020 igo95862

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

# http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""
Notcurses python module
"""
from __future__ import annotations

from enum import IntEnum
from typing import Dict, Iterable, Optional, Tuple, Union

from . import _notcurses
from ._notcurses import (_nc_channels_set_background_rgb,
                         _nc_channels_set_foreground_rgb,
                         _nc_direct_disable_cursor, _nc_direct_enable_cursor,
                         _nc_direct_get_dim_x, _nc_direct_get_dim_y,
                         _nc_direct_init, _nc_direct_putstr, _nc_direct_stop,
                         _nc_plane_create, _nc_plane_dimensions_yx,
                         _nc_plane_erase, _nc_plane_putstr,
                         _nc_plane_putstr_aligned,
                         _nc_plane_set_background_rgb,
                         _nc_plane_set_foreground_rgb, _NcChannels, _NcDirect,
                         _NcInput, _NcPlane, _notcurses_context_cursor_disable,
                         _notcurses_context_cursor_enable,
                         _notcurses_context_get_input_blocking,
                         _notcurses_context_get_std_plane,
                         _notcurses_context_init,
                         _notcurses_context_mouse_disable,
                         _notcurses_context_mouse_enable,
                         _notcurses_context_render, _notcurses_context_stop,
                         _NotcursesContext)


class NcAlign(IntEnum):
    """
    Enum containing alignment types

    :cvar UNALIGNED: No alignment
    :cvar LEFT: Left alignment
    :cvar CENTER: Center alignment
    :cvar RIGHT: Right alignment
    """
    UNALIGNED = _notcurses.NCALIGN_UNALIGNED
    LEFT = _notcurses.NCALIGN_LEFT
    CENTER = _notcurses.NCALIGN_CENTER
    RIGHT = _notcurses.NCALIGN_RIGHT


class NotcursesContext:
    """
    Notcurses Context

    This class controls the attached terminal and should only be
    initialized once per terminal.

    Using :py:func:`get_std_plane` is recommended in most cases instead
    of directly initializing the context.
    """

    def __init__(self,
                 start_immediately: bool = True):
        """
        Create the context

        :param bool start_immediately: Whether or not to acquire the terminal
        """
        self._nc_context = _NotcursesContext()
        self._has_started = False
        if start_immediately:
            self.start()

    def render(self) -> None:
        """
        Updates the terminal window with the actual content
        This should be called after the you have filled the
        plane with such function as :py:meth:`NcPlane.put_lines`

        .. warning::
            This method is not thread safe.
        """
        _notcurses_context_render(self._nc_context)

    def start(self) -> None:
        """Notcurses acquires the terminal."""
        _notcurses_context_init(self._nc_context)
        self._has_started = True

    def stop(self) -> None:
        """
        Notcurses releases the terminal.

        This will be automatically called with the context object
        gets garbage collected.
        """
        _notcurses_context_stop(self._nc_context)
        self._has_started = False

    def get_input_blocking(self) -> NcInput:
        """
        Waits synchronously for an :py:class:`NcInput` event.
        """
        return NcInput(
            _notcurses_context_get_input_blocking(self._nc_context)
        )

    def enable_mouse(self) -> None:
        """Enables mouse on the terminal"""
        _notcurses_context_mouse_enable(self._nc_context)

    def disable_mouse(self) -> None:
        """Disables mouse on the terminal"""
        _notcurses_context_mouse_disable(self._nc_context)

    def enable_cursor(self) -> None:
        """Enables cursor on the terminal"""
        _notcurses_context_cursor_enable(self._nc_context, 0, 0)

    def disable_cursor(self) -> None:
        """Disables cursor on the terminal"""
        _notcurses_context_cursor_disable(self._nc_context)

    def __del__(self) -> None:
        if self._has_started:
            self.stop()


class NcInput:
    """Represents an input event"""

    def __init__(self, nc_input: _NcInput):
        self._nc_input = nc_input

    @property
    def code(self) -> Union[str, NcInputCodes]:
        """
        Either a single character or an enum of :py:class:`NcInputCodes`

        For example, `q` represents a button Q on keyboard.
        `NcInputCodes.MOUSE_LEFT_BUTTON` represents left mouse button click.

        The keys references can be found in :py:class:`NcInputCodes`

        :rtype: Union[str, NcInputCodes]
        """
        try:
            return NC_INPUT_CODES[self._nc_input.codepoint]
        except KeyError:
            return chr(self._nc_input.codepoint)

    @property
    def y_pos(self) -> int:
        """
        Y position of event

        :rtype: int
        """
        return self._nc_input.y_pos

    @property
    def x_pos(self) -> int:
        """
        X position of event

        :rtype: int
        """
        return self._nc_input.x_pos

    @property
    def is_alt(self) -> bool:
        """
        Was Alt key pressed during event?

        :rtype: bool
        """
        return self._nc_input.is_alt

    @property
    def is_shift(self) -> bool:
        """
        Was Shift key pressed during event?

        :rtype: bool
        """
        return self._nc_input.is_shift

    @property
    def is_ctrl(self) -> bool:
        """
        Was Ctrl key pressed during event?

        :rtype: bool
        """
        return self._nc_input.is_ctrl

    @property
    def seqnum(self) -> int:
        """
        Sequence number

        :rtype: int
        """
        return self._nc_input.seqnum


class NcPlane:
    """Class representing a drawing surface"""

    def __init__(self, plane: _NcPlane, context: NotcursesContext) -> None:
        """
        NcPlane should not be initialized directly by user.
        Use :py:meth:`NcPlane.create_sub_plane` to create sub planes from the
        standard plane
        """
        self._nc_plane = plane
        self.context = context

    @property
    def dimensions_yx(self) -> Tuple[int, int]:
        """
        Returns Y and X dimensions of the plane

        :rtype: Tuple[int, int]
        """
        return _nc_plane_dimensions_yx(self._nc_plane)

    def putstr(
            self,
            string: str,
            y_pos: int = -1, x_pos: int = -1) -> int:
        """
        Puts a string on the plane

        :param str string: String to put
        :param int y_pos: Y position to put string.
            By default is the cursor position.
        :param int x_pos: X position to put string.
            By default is the cursor position.
        :returns: Number of characters written.
            Negative if some characters could not be written.
        :rtype: int
        """
        return _nc_plane_putstr(
            self._nc_plane,
            string,
            y_pos,
            x_pos,
        )

    def putstr_aligned(self,
                       string: str,
                       y_pos: int = -1,
                       align: NcAlign = NcAlign.UNALIGNED) -> int:
        """
        Puts a string on the plane with specified alignment
        instead of X coordinate

        :param str string: String to put
        :param int y_pos: Y position to put string.
            By default is the cursor position.
        :param NcAlign align: Use specific alignment.
        :returns: Number of characters written.
            Negative if some characters could not be written.
        :rtype: int
        """
        return _nc_plane_putstr_aligned(
            self._nc_plane,
            string,
            y_pos,
            align,
        )

    def put_lines(
        self, lines_iter: Iterable[str], wrap_lines: bool = False
    ) -> None:
        """
        Puts string from the iterator on the plane.
        Each string is put on a new line.

        :param iter[str] lines_iter: Iterator of lines to put on the plane
        :param bool wrap_lines: If line is longer that the surface
            should it be continued on the next line? Default false.
        """
        y_pos = 0

        for line in lines_iter:
            # TODO: needs to stop if we are outside the plane
            chars_put = self.putstr(line, y_pos, 0)
            y_pos += 1

            if not wrap_lines:
                continue

            while chars_put < 0:
                line = line[abs(chars_put):]
                chars_put = self.putstr(line, y_pos, 0)
                y_pos += 1

    def erase(self) -> None:
        """Remove all symbols from plane"""
        return _nc_plane_erase(self._nc_plane)

    def set_background_rgb(
            self, red: int, green: int, blue: int) -> None:
        """
        Sets the background color

        :param int red: Red color component given as integer from 0 to 255
        :param int green: Green color component given as integer from 0 to 255
        :param int blue: Blue color component given as integer from 0 to 255
        """
        _nc_plane_set_background_rgb(self._nc_plane, red, green, blue)

    def set_foreground_rgb(
            self, red: int, green: int, blue: int) -> None:
        """
        Sets the foreground color

        :param int red: Red color component given as integer from 0 to 255
        :param int green: Green color component given as integer from 0 to 255
        :param int blue: Blue color component given as integer from 0 to 255
        """
        _nc_plane_set_foreground_rgb(self._nc_plane, red, green, blue)

    def create_sub_plane(
        self,
        y_pos: int = 0,
        x_pos: int = 0,
        rows_num: Optional[int] = None,
        cols_num: Optional[int] = None
    ) -> NcPlane:
        """
        Creates a new plane within this plane

        :param int y_pos: top left corner Y coordinate
            relative to top left corner of parent

        :param int x_pos: top left corner X coordinate
            relative to top left corner of parent

        :param int rows_num: Number of rows (i.e. Y size)
        :param int cols_num: Number of columns (i.e. X size)
        :returns: New plane
        :rtype: NcPlane
        """

        if cols_num is None:
            y_dim, _ = self.dimensions_yx
            cols_num = y_dim // 2

        if rows_num is None:
            _, x_dim = self.dimensions_yx
            rows_num = x_dim

        new_plane = _nc_plane_create(
            self._nc_plane,
            y_pos, x_pos, rows_num, cols_num
        )

        return NcPlane(new_plane, self.context)


_default_context: Optional[NotcursesContext] = None


def get_std_plane() -> NcPlane:
    """
    Initializes context and returns the standard plane.

    .. warning::
        The terminal will be acquired by notcurses and uncontrollable until
        the standard plane will be dereferenced.

    :return: Standard plane of the terminal
    :rtype: NcPlane
    """
    global _default_context
    if _default_context is None:
        _default_context = NotcursesContext()

    std_plane_ref = _notcurses_context_get_std_plane(
        _default_context._nc_context)
    return NcPlane(std_plane_ref, _default_context)


class NcChannels:
    """
    Class that hold the colors and transparency values

    Can be used in some functions instead of directly specifying colors.
    """

    def __init__(self) -> None:
        self._nc_channels = _NcChannels()

    def set_background_rgb(self, red: int, green: int, blue: int) -> None:
        """
        Sets the background color

        :param int red: Red color component given as integer from 0 to 255
        :param int green: Green color component given as integer from 0 to 255
        :param int blue: Blue color component given as integer from 0 to 255
        """
        _nc_channels_set_background_rgb(
            self._nc_channels,
            red, green, blue,
        )

    def set_foreground_rgb(self, red: int, green: int, blue: int) -> None:
        """
        Sets the foreground color

        :param int red: Red color component given as integer from 0 to 255
        :param int green: Green color component given as integer from 0 to 255
        :param int blue: Blue color component given as integer from 0 to 255
        """
        _nc_channels_set_foreground_rgb(
            self._nc_channels,
            red, green, blue,
        )


class NcDirect:
    """
    NcDirect is a subset of Notcurses.
    It does not clear entire terminal but instead draws on to normal
    terminal surface. That means the output is preserved after the application
    has exited and can be scrolled back.

    NcDirect has only one main plane.
    """

    def __init__(self,
                 start_immediately: bool = True):
        """
        Create the main direct plane.

        :param bool start_immediately: Whether or not to start NcDirect on
            initialization.
        """
        self._nc_direct = _NcDirect()
        self._is_cursor_enabled: Optional[bool] = None
        self._has_started = False
        if start_immediately:
            self.start()

    def __del__(self) -> None:
        if self._has_started:
            self.stop()

    def start(self) -> None:
        """
        Start NcDirect.
        """
        _nc_direct_init(self._nc_direct)
        self._has_started = True

    def stop(self) -> None:
        """
        Stop NcDirect

        Will be automatically called if NcDirect object gets garbage collected
        """
        _nc_direct_stop(self._nc_direct)

    def putstr(
            self, string: str,
            nc_channels: Optional[NcChannels] = None) -> int:
        """
        Puts a string on the plane.
        This will immediately take effect. There is not `render` function for
        NcDirect.

        :param Optional[NcChannels] nc_channels: The colors string will use
        :returns: Number of characters written.
            Negative if some characters could not be written.
        :rtype: int
        """

        return _nc_direct_putstr(
            self._nc_direct,
            string,
            nc_channels._nc_channels
            if nc_channels is not None else nc_channels,
        )

    @property
    def dimensions_yx(self) -> Tuple[int, int]:
        """
        Returns Y and X dimensions of the plane

        :rtype: Tuple[int, int]
        """
        return (_nc_direct_get_dim_y(self._nc_direct),
                _nc_direct_get_dim_x(self._nc_direct))

    @property
    def cursor_enabled(self) -> Optional[bool]:
        """
        Is the cursor enabled?

        Assign boolean to enable or disable cursor.

        :type: bool
        :rtype: bool
        """
        return self._is_cursor_enabled

    @cursor_enabled.setter
    def cursor_enabled(self, set_to_what: Optional[bool]) -> None:
        self._is_cursor_enabled = set_to_what
        if set_to_what:
            _nc_direct_enable_cursor(self._nc_direct)
        else:
            _nc_direct_disable_cursor(self._nc_direct)


class NcInputCodes(IntEnum):
    """
    Enum containing special keys mapping

    :cvar INVALID:
    :cvar UP:
    :cvar RESIZE:
    :cvar RIGHT:
    :cvar DOWN:
    :cvar LEFT:
    :cvar INSERT:
    :cvar DELETE:
    :cvar BACKSPACE:
    :cvar PAGE_DOWN:
    :cvar PAGE_UP:
    :cvar HOME:
    :cvar EBD:
    :cvar F0:
    :cvar F1:
    :cvar F2:
    :cvar F3:
    :cvar F4:
    :cvar F5:
    :cvar F6:
    :cvar F7:
    :cvar F8:
    :cvar F9:
    :cvar F10:
    :cvar F11:
    :cvar F12:
    :cvar ENTER:
    :cvar CAPS_LOCL:
    :cvar DOWN_LEFT:
    :cvar DOWN_RIGHT:
    :cvar UP_LEFT:
    :cvar UP_RIGHT:
    :cvar CENTER:
    :cvar BEGIN:
    :cvar CANCEL:
    :cvar CLOSE:
    :cvar COMMAND:
    :cvar COPY:
    :cvar EXIT:
    :cvar PRINT:
    :cvar REFRESH:
    :cvar MOUSE_LEFT_BUTTON:
    :cvar MOUSE_MIDDLE_BUTTON:
    :cvar MOUSE_RIGHT_BUTTON:
    :cvar MOUSE_SCROLL_UP:
    :cvar MOUSE_SCROLL_DOWN:
    :cvar MOUSE_6:
    :cvar MOUSE_RELEASE:
    """
    INVALID = _notcurses.NCKEY_INVALID
    UP = _notcurses.NCKEY_UP
    RESIZE = _notcurses.NCKEY_RESIZE
    RIGHT = _notcurses.NCKEY_RIGHT
    DOWN = _notcurses.NCKEY_DOWN
    LEFT = _notcurses.NCKEY_LEFT
    INSERT = _notcurses.NCKEY_INS
    DELETE = _notcurses.NCKEY_DEL
    BACKSPACE = _notcurses.NCKEY_BACKSPACE
    PAGE_DOWN = _notcurses.NCKEY_PGDOWN
    PAGE_UP = _notcurses.NCKEY_PGUP
    HOME = _notcurses.NCKEY_HOME
    EBD = _notcurses.NCKEY_END
    F0 = _notcurses.NCKEY_F00
    F1 = _notcurses.NCKEY_F01
    F2 = _notcurses.NCKEY_F02
    F3 = _notcurses.NCKEY_F03
    F4 = _notcurses.NCKEY_F04
    F5 = _notcurses.NCKEY_F05
    F6 = _notcurses.NCKEY_F06
    F7 = _notcurses.NCKEY_F07
    F8 = _notcurses.NCKEY_F08
    F9 = _notcurses.NCKEY_F09
    F10 = _notcurses.NCKEY_F10
    F11 = _notcurses.NCKEY_F11
    F12 = _notcurses.NCKEY_F12
    ENTER = _notcurses.NCKEY_ENTER
    CAPS_LOCL = _notcurses.NCKEY_CLS
    DOWN_LEFT = _notcurses.NCKEY_DLEFT
    DOWN_RIGHT = _notcurses.NCKEY_DRIGHT
    UP_LEFT = _notcurses.NCKEY_ULEFT
    UP_RIGHT = _notcurses.NCKEY_URIGHT
    CENTER = _notcurses.NCKEY_CENTER
    BEGIN = _notcurses.NCKEY_BEGIN
    CANCEL = _notcurses.NCKEY_CANCEL
    CLOSE = _notcurses.NCKEY_CLOSE
    COMMAND = _notcurses.NCKEY_COMMAND
    COPY = _notcurses.NCKEY_COPY
    EXIT = _notcurses.NCKEY_EXIT
    PRINT = _notcurses.NCKEY_PRINT
    REFRESH = _notcurses.NCKEY_REFRESH
    MOUSE_LEFT_BUTTON = _notcurses.NCKEY_BUTTON1
    MOUSE_MIDDLE_BUTTON = _notcurses.NCKEY_BUTTON2
    MOUSE_RIGHT_BUTTON = _notcurses.NCKEY_BUTTON3
    MOUSE_SCROLL_UP = _notcurses.NCKEY_SCROLL_UP
    MOUSE_SCROLL_DOWN = _notcurses.NCKEY_SCROLL_DOWN
    MOUSE_6 = _notcurses.NCKEY_BUTTON6
    MOUSE_RELEASE = _notcurses.NCKEY_RELEASE


NC_INPUT_CODES: Dict[int, NcInputCodes] = {
    element.value: element for element in NcInputCodes
}

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
from __future__ import annotations

from typing import Optional, Tuple


class _NcChannels:
    ...


class _NcPlane:
    ...


class _NotcursesContext:
    ...


class _NcDirect:
    ...


class _NcInput:
    @property
    def codepoint(self) -> int:
        ...

    @property
    def y_pos(self) -> int:
        ...

    @property
    def x_pos(self) -> int:
        ...

    @property
    def is_alt(self) -> bool:
        ...

    @property
    def is_shift(self) -> bool:
        ...

    @property
    def is_ctrl(self) -> bool:
        ...

    @property
    def seqnum(self) -> int:
        ...


def _nc_direct_init(ncdirect: _NcDirect, /) -> None:
    ...


def _nc_direct_stop(ncdirect: _NcDirect, /) -> None:
    ...


def _nc_direct_putstr(nc_direct: _NcDirect,
                      string: str,
                      nc_channels: Optional[_NcChannels], /) -> int:
    ...


def _nc_direct_get_dim_x(nc_direct: _NcDirect, /) -> int:
    ...


def _nc_direct_get_dim_y(nc_direct: _NcDirect, /) -> int:
    ...


def _nc_direct_disable_cursor(nc_direct: _NcDirect, /) -> None:
    ...


def _nc_direct_enable_cursor(nc_direct: _NcDirect, /) -> None:
    ...


def _nc_channels_set_background_rgb(
        nc_channels: _NcChannels,
        red: int, green: int, blue: int, /) -> None:
    ...


def _nc_channels_set_foreground_rgb(
        nc_channels: _NcChannels,
        red: int, green: int, blue: int, /) -> None:
    ...


def _notcurses_context_init(nc_context: _NotcursesContext, /) -> None:
    ...


def _notcurses_context_stop(nc_context: _NotcursesContext, /) -> None:
    ...


def _notcurses_context_render(nc_context: _NotcursesContext, /) -> None:
    ...


def _notcurses_context_mouse_disable(nc_context: _NotcursesContext, /) -> None:
    ...


def _notcurses_context_mouse_enable(nc_context: _NotcursesContext, /) -> None:
    ...


def _notcurses_context_cursor_disable(
        nc_context: _NotcursesContext, /) -> None:
    ...


def _notcurses_context_cursor_enable(
        nc_context: _NotcursesContext,
        y_pos: int, x_pos: int, /) -> None:
    ...


def _notcurses_context_get_std_plane(
        nc_context: _NotcursesContext, /) -> _NcPlane:
    ...


def _notcurses_context_get_input_blocking(
        nc_context: _NotcursesContext, /) -> _NcInput:
    ...


def _nc_plane_set_background_rgb(
        nc_plane: _NcPlane,
        red: int, green: int, blue: int, /) -> None:
    ...


def _nc_plane_set_foreground_rgb(
        nc_plane: _NcPlane,
        red: int, green: int, blue: int, /) -> None:
    ...


def _nc_plane_putstr(
        nc_plane: _NcPlane, string: str,
        y_pos: int, x_pos: int, /) -> int:
    ...


def _nc_plane_putstr_aligned(
        nc_plane: _NcPlane, string: str,
        y_pos: int, align: int, /) -> int:
    ...


def _nc_plane_dimensions_yx(nc_plane: _NcPlane, /) -> Tuple[int, int]:
    ...


def _nc_plane_polyfill_yx(
        nc_plane: _NcPlane,
        y_pos: int, x_pos: int, cell_str: str, /) -> int:
    ...


def _nc_plane_erase(nc_plane: _NcPlane, /) -> None:
    ...


def _nc_plane_create(
        nc_plane: _NcPlane,
        y_pos: int, x_pos: int,
        rows_num: int, cols_num: int, /) -> _NcPlane:
    ...


def get_notcurses_version() -> str:
    """Returns notcurses version from library"""
    ...


# Assign 0 to make this stub file importable
NCKEY_INVALID: int = 0
NCKEY_UP: int = 0
NCKEY_RESIZE: int = 0
NCKEY_RIGHT: int = 0
NCKEY_DOWN: int = 0
NCKEY_LEFT: int = 0
NCKEY_INS: int = 0
NCKEY_DEL: int = 0
NCKEY_BACKSPACE: int = 0
NCKEY_PGDOWN: int = 0
NCKEY_PGUP: int = 0
NCKEY_HOME: int = 0
NCKEY_END: int = 0
NCKEY_F00: int = 0
NCKEY_F01: int = 0
NCKEY_F02: int = 0
NCKEY_F03: int = 0
NCKEY_F04: int = 0
NCKEY_F05: int = 0
NCKEY_F06: int = 0
NCKEY_F07: int = 0
NCKEY_F08: int = 0
NCKEY_F09: int = 0
NCKEY_F10: int = 0
NCKEY_F11: int = 0
NCKEY_F12: int = 0
NCKEY_ENTER: int = 0
NCKEY_CLS: int = 0
NCKEY_DLEFT: int = 0
NCKEY_DRIGHT: int = 0
NCKEY_ULEFT: int = 0
NCKEY_URIGHT: int = 0
NCKEY_CENTER: int = 0
NCKEY_BEGIN: int = 0
NCKEY_CANCEL: int = 0
NCKEY_CLOSE: int = 0
NCKEY_COMMAND: int = 0
NCKEY_COPY: int = 0
NCKEY_EXIT: int = 0
NCKEY_PRINT: int = 0
NCKEY_REFRESH: int = 0
NCKEY_BUTTON1: int = 0
NCKEY_BUTTON2: int = 0
NCKEY_BUTTON3: int = 0
NCKEY_SCROLL_UP: int = 0
NCKEY_SCROLL_DOWN: int = 0
NCKEY_BUTTON6: int = 0
NCKEY_RELEASE: int = 0
NCALIGN_UNALIGNED: int = 0
NCALIGN_LEFT: int = 0
NCALIGN_CENTER: int = 0
NCALIGN_RIGHT: int = 0
NCSCALE_NONE: int = 0
NCSCALE_SCALE: int = 0
NCSCALE_STRETCH: int = 0

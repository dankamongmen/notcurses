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


from notcurses import get_std_plane

std_plane = get_std_plane()
std_plane.putstr("Enter string!")

std_plane.context.render()
std_plane.context.enable_cursor()
std_plane.context.enable_mouse()

while True:
    # Get an input event and print its properties
    p = std_plane.context.get_input_blocking()
    std_plane.erase()
    std_plane.putstr(f"Code point: {repr(p.code)}",
                     y_pos=0, x_pos=0)
    std_plane.putstr(f"Y pos: {p.y_pos}", y_pos=1, x_pos=0)
    std_plane.putstr(f"X pos: {p.x_pos}", y_pos=2, x_pos=0)
    std_plane.putstr(f"Is alt: {p.is_alt}", y_pos=3, x_pos=0)
    std_plane.putstr(f"Is shift: {p.is_shift}", y_pos=4, x_pos=0)
    std_plane.putstr(f"Is ctrl: {p.is_ctrl}", y_pos=5, x_pos=0)
    std_plane.putstr(f"Seqnum: {p.seqnum}", y_pos=6, x_pos=0)
    std_plane.putstr("Press CTRL+C to exit.", y_pos=7, x_pos=0)

    std_plane.context.render()

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

from notcurses import Notcurses

nc = Notcurses()
std_plane = nc.stdplane()
std_plane.putstr("Enter string!")

nc.render()
nc.mice_enable()

while True:
    # Get an input event and print its properties
    inp = nc.get_blocking()
    std_plane.erase()
    std_plane.putstr_yx(1, 4, f"Code point: {hex(inp.id)}")
    std_plane.putstr_yx(2, 4, f"Y pos: {inp.y}")
    std_plane.putstr_yx(3, 4, f"X pos: {inp.x}")
    std_plane.putstr_yx(4, 4, f"UTF-8: {inp.utf8!r}")
    std_plane.putstr_yx(5, 4, f"Event type: {inp.evtype}")
    std_plane.putstr_yx(6, 4, f"Modifiers: {bin(inp.modifiers)}")
    std_plane.putstr_yx(7, 4, f"Y pixel offset: {inp.ypx}")
    std_plane.putstr_yx(8, 4, f"X pixel offset: {inp.xpx}")
    std_plane.putstr_yx(10, 4, "Press CTRL+C to exit.")

    nc.render()

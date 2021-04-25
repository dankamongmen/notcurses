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


from time import sleep

from notcurses import Notcurses

nc = Notcurses()

stdplane = nc.stdplane()

sub_plane_left = stdplane.create(
    rows=5,
    cols=5,
)

sub_plane_right = stdplane.create(
    x_pos=(stdplane.dim_x() // 2),
    rows=5,
    cols=5,
)

sub_plane_left.set_fg_rgb8(0, 255, 0)
sub_plane_left.putstr("Left")

sub_plane_right.set_fg_rgb8(255, 0, 0)
sub_plane_right.putstr("Right")

nc.render()

sleep(3)

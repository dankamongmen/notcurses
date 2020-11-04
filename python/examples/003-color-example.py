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

from notcurses import get_std_plane

std_plane = get_std_plane()
std_plane.set_background_rgb(0, 0, 255)
std_plane.set_foreground_rgb(255, 0, 0)
std_plane.putstr("Red on blue", y_pos=0)

std_plane.set_background_rgb(0, 255, 0)
std_plane.set_foreground_rgb(255, 255, 255)
std_plane.putstr("White on green", y_pos=1, x_pos=0)

std_plane.set_background_rgb(0, 0, 0)
std_plane.set_foreground_rgb(255, 0, 255)
std_plane.putstr("Purple on black", y_pos=2, x_pos=0)

std_plane.context.render()

sleep(5)

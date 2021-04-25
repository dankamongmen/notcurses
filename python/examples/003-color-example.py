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
stdplane.set_bg_rgb8_clipped(0, 0, 255)
stdplane.set_fg_rgb8_clipped(255, 0, 0)
stdplane.putstr_yx(0, 0, "Red on blue")

stdplane.set_bg_rgb8(0, 255, 0)
stdplane.set_fg_rgb8(255, 255, 255)
stdplane.putstr_yx(1, 0, "White on green")

stdplane.set_bg_rgb8(0, 0, 0)
stdplane.set_fg_rgb8(255, 0, 255)
stdplane.putstr_yx(2, 0, "Purple on black")

nc.render()

sleep(5)

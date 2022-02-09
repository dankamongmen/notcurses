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
plane = nc.stdplane()

y_dimension, x_dimension = plane.dim_yx()
for y in range(y_dimension):
    for x in range(x_dimension):
        y_frac = round(y / y_dimension * 256)
        x_frac = round(x / x_dimension * 256)
        plane.set_fg_rgb8(128, y_frac, x_frac)
        plane.set_bg_rgb8(x_frac, 128, y_frac)
        plane.putstr_yx(y, x, 'X')

nc.render()
sleep(5)


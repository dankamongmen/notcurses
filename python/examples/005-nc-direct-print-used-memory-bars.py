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
from typing import Generator

from notcurses import NcChannels, NcDirect

# Acquire the NcDirect plane
nc_direct = NcDirect()
nc_direct.cursor_enabled = False
channels = NcChannels()
channels.set_background_rgb(255, 0, 0)
channels.set_foreground_rgb(0, 0, 0)

# Get x dimensions, ignore y
_, x_dimension = nc_direct.dimensions_yx

# Function to generate the green to red line


def red_line_gen(used_space: int = 0, percent: float = 1.0
                 ) -> Generator[int, None, None]:
    line_size = x_dimension - used_space  # How much space in the line we have
    blocks_to_put = round(percent * line_size)  # How many blocks to fill
    for x in range(blocks_to_put):
        # Yeilds the integer on what to reduce green and increase red by
        yield round(x * 255.0 / line_size)


# Open the meminfo file
with open('/proc/meminfo') as f:
    meminfo_lines = f.read().splitlines()


mem_total_line = meminfo_lines[0]  # Line 0 is the total memory
mem_avalible_line = meminfo_lines[2]

# The lines in meminfo file are like this
# MemTotal:        3801578 kB
# so we need to split by whitespace and get second item
mem_total = int(mem_total_line.split()[1])  # Get total memory
mem_avalible = int(mem_avalible_line.split()[1])  # Get avalible memory

mem_percent_used = 1.0 - mem_avalible / mem_total  # Calculate percent used

mem_sting = f"Memory used: {round(100.0 * mem_percent_used)}% "

nc_direct.putstr(mem_sting)  # Put the used memory

for red_shift in red_line_gen(len(mem_sting), mem_percent_used):
    # Get the red shift from the function and use it in red channel
    # and subtract it from green
    channels.set_background_rgb(red_shift, 255-red_shift, 0)
    channels.set_foreground_rgb(red_shift, 255-red_shift, 0)
    nc_direct.putstr('X', channels)

nc_direct.putstr('\n')  # Finish line

swap_total = int(meminfo_lines[14].split()[1])  # Get swap total
swap_avalible = int(meminfo_lines[15].split()[1])  # Get swap used

swap_percent_used = 1.0 - swap_avalible / swap_total

swap_string = f"Swap used:   {round(100.0 * swap_percent_used)}% "

# Add space in case swap is a single digit
if len(swap_string) < len(mem_sting):
    swap_string += ' '

nc_direct.putstr(swap_string)

for red_shift in red_line_gen(len(swap_string), swap_percent_used):
    channels.set_background_rgb(red_shift, 255-red_shift, 0)
    channels.set_foreground_rgb(red_shift, 255-red_shift, 0)
    nc_direct.putstr('X', channels)

nc_direct.putstr('\n')

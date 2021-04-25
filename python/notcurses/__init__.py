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

from .notcurses import (
    NcPlane, Notcurses, channel_alpha, channel_b, channel_default_p,
    channel_g, channel_palindex, channel_palindex_p, channel_r,
    channel_rgb8, channel_rgb_initializer, channel_set,
    channel_set_alpha, channel_set_default, channel_set_palindex,
    channel_set_rgb8, channel_set_rgb8_clipped, channels_bchannel,
    channels_bg_alpha, channels_bg_default_p, channels_bg_palindex,
    channels_bg_palindex_p, channels_bg_rgb, channels_bg_rgb8,
    channels_combine, channels_fchannel, channels_fg_alpha,
    channels_fg_default_p, channels_fg_palindex,
    channels_fg_palindex_p, channels_fg_rgb, channels_fg_rgb8,
    channels_rgb_initializer, channels_set_bchannel,
    channels_set_bg_alpha, channels_set_bg_default,
    channels_set_bg_palindex, channels_set_bg_rgb,
    channels_set_bg_rgb8, channels_set_bg_rgb8_clipped,
    channels_set_fchannel, channels_set_fg_alpha,
    channels_set_fg_default, channels_set_fg_palindex,
    channels_set_fg_rgb, channels_set_fg_rgb8,
    channels_set_fg_rgb8_clipped, ncstrwidth, notcurses_version,
    notcurses_version_components,
)

__all__ = (
    'NcPlane', 'Notcurses',

    'channel_alpha', 'channel_b', 'channel_default_p', 'channel_g',
    'channel_palindex', 'channel_palindex_p', 'channel_r', 'channel_rgb8',
    'channel_rgb_initializer', 'channel_set', 'channel_set_alpha',
    'channel_set_default', 'channel_set_palindex', 'channel_set_rgb8',
    'channel_set_rgb8_clipped', 'channels_bchannel', 'channels_bg_alpha',
    'channels_bg_default_p', 'channels_bg_palindex',
    'channels_bg_palindex_p', 'channels_bg_rgb', 'channels_bg_rgb8',
    'channels_combine', 'channels_fchannel', 'channels_fg_alpha',
    'channels_fg_default_p', 'channels_fg_palindex',
    'channels_fg_palindex_p', 'channels_fg_rgb', 'channels_fg_rgb8',
    'channels_rgb_initializer', 'channels_set_bchannel',
    'channels_set_bg_alpha', 'channels_set_bg_default',
    'channels_set_bg_palindex', 'channels_set_bg_rgb',
    'channels_set_bg_rgb8', 'channels_set_bg_rgb8_clipped',
    'channels_set_fchannel', 'channels_set_fg_alpha',
    'channels_set_fg_default', 'channels_set_fg_palindex',
    'channels_set_fg_rgb', 'channels_set_fg_rgb8',
    'channels_set_fg_rgb8_clipped',

    'ncstrwidth', 'notcurses_version', 'notcurses_version_components',
)

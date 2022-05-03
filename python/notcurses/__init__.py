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
    NcPlane, Notcurses, NcInput,
    ncchannel_alpha, ncchannel_b, ncchannel_default_p,
    ncchannel_g, ncchannel_palindex, ncchannel_palindex_p, ncchannel_r,
    ncchannel_rgb8, ncchannel_rgb_initializer, ncchannel_set,
    ncchannel_set_alpha, ncchannel_set_default, ncchannel_set_palindex,
    ncchannel_set_rgb8, ncchannel_set_rgb8_clipped, ncchannels_bchannel,
    ncchannels_bg_alpha, ncchannels_bg_default_p, ncchannels_bg_palindex,
    ncchannels_bg_palindex_p, ncchannels_bg_rgb, ncchannels_bg_rgb8,
    ncchannels_combine, ncchannels_fchannel, ncchannels_fg_alpha,
    ncchannels_fg_default_p, ncchannels_fg_palindex,
    ncchannels_fg_palindex_p, ncchannels_fg_rgb, ncchannels_fg_rgb8,
    ncchannels_rgb_initializer, ncchannels_set_bchannel,
    ncchannels_set_bg_alpha, ncchannels_set_bg_default,
    ncchannels_set_bg_palindex, ncchannels_set_bg_rgb,
    ncchannels_set_bg_rgb8, ncchannels_set_bg_rgb8_clipped,
    ncchannels_set_fchannel, ncchannels_set_fg_alpha,
    ncchannels_set_fg_default, ncchannels_set_fg_palindex,
    ncchannels_set_fg_rgb, ncchannels_set_fg_rgb8,
    ncchannels_set_fg_rgb8_clipped, ncstrwidth, notcurses_version,
    notcurses_version_components,
    NCBOXASCII, NCBOXDOUBLE, NCBOXHEAVY, NCBOXLIGHT, NCBOXOUTER, NCBOXROUND,
    box, rgb,
)

__all__ = (
    'NcPlane', 'Notcurses',

    'ncchannel_alpha', 'ncchannel_b', 'ncchannel_default_p', 'ncchannel_g',
    'ncchannel_palindex', 'ncchannel_palindex_p', 'ncchannel_r',
    'ncchannel_rgb8',
    'ncchannel_rgb_initializer', 'ncchannel_set', 'ncchannel_set_alpha',
    'ncchannel_set_default', 'ncchannel_set_palindex', 'ncchannel_set_rgb8',
    'ncchannel_set_rgb8_clipped', 'ncchannels_bchannel', 'ncchannels_bg_alpha',
    'ncchannels_bg_default_p', 'ncchannels_bg_palindex',
    'ncchannels_bg_palindex_p', 'ncchannels_bg_rgb', 'ncchannels_bg_rgb8',
    'ncchannels_combine', 'ncchannels_fchannel', 'ncchannels_fg_alpha',
    'ncchannels_fg_default_p', 'ncchannels_fg_palindex',
    'ncchannels_fg_palindex_p', 'ncchannels_fg_rgb', 'ncchannels_fg_rgb8',
    'ncchannels_rgb_initializer', 'ncchannels_set_bchannel',
    'ncchannels_set_bg_alpha', 'ncchannels_set_bg_default',
    'ncchannels_set_bg_palindex', 'ncchannels_set_bg_rgb',
    'ncchannels_set_bg_rgb8', 'ncchannels_set_bg_rgb8_clipped',
    'ncchannels_set_fchannel', 'ncchannels_set_fg_alpha',
    'ncchannels_set_fg_default', 'ncchannels_set_fg_palindex',
    'ncchannels_set_fg_rgb', 'ncchannels_set_fg_rgb8',
    'ncchannels_set_fg_rgb8_clipped',

    'ncstrwidth', 'notcurses_version', 'notcurses_version_components',

    'box', 'rgb',
)

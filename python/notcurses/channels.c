// SPDX-License-Identifier: Apache-2.0
/*
Copyright 2020, 2021 igo95862

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "notcurses-python.h"

static PyObject *
python_channels_rgb_initializer(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long fr, fg, fb, br, bg, bb = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "KKKKKK", &fr, &fg, &fb, &br, &bg, &bb));

    unsigned long long channels = CHANNELS_RGB_INITIALIZER(fr, fg, fb, br, bg, bb);

    return Py_BuildValue("K", channels);
}

static PyObject *
python_channel_rgb_initializer(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long r, g, b = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "kkk", &r, &g, &b));

    unsigned long channel = CHANNEL_RGB_INITIALIZER(r, g, b);

    return Py_BuildValue("k", channel);
}

static PyObject *
python_channel_r(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long channel = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "k", &channel));

    unsigned long r = channel_r((uint32_t)channel);

    return Py_BuildValue("k", r);
}

static PyObject *
python_channel_g(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long channel = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "k", &channel));

    unsigned long g = channel_r((uint32_t)channel);

    return Py_BuildValue("k", g);
}

static PyObject *
python_channel_b(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long channel = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "k", &channel));

    unsigned long b = channel_b((uint32_t)channel);

    return Py_BuildValue("k", b);
}

static PyObject *
python_channel_rgb8(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long channel = {0};
    unsigned int r, g, b = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "k", &channel));

    channel_rgb8((uint32_t)channel, &r, &g, &b);

    return Py_BuildValue("III", r, g, b);
}

static PyObject *
python_channel_set_rgb8(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long channel = {0};
    int r, g, b = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "kiii", &channel, &r, &g, &b));

    uint32_t channel_fixed_size = (uint32_t)channel;

    CHECK_NOTCURSES(channel_set_rgb8(&channel_fixed_size, r, g, b));

    return Py_BuildValue("k", (unsigned long)channel_fixed_size);
}

static PyObject *
python_channel_set_rgb8_clipped(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned int channel = {0};
    int r, g, b = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "Iiii", &channel, &r, &g, &b));

    channel_set_rgb8_clipped(&channel, r, g, b);

    return Py_BuildValue("k", channel);
}

static PyObject *
python_channel_set(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned int channel, rgb = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "II", &channel, &rgb));

    CHECK_NOTCURSES(channel_set(&channel, rgb));

    return Py_BuildValue("I", channel);
}

static PyObject *
python_channel_alpha(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned int channel = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "I", &channel));

    return Py_BuildValue("I", channel_alpha(channel));
}

static PyObject *
python_channel_palindex(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long channel = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "k", &channel));

    return Py_BuildValue("I", channel_palindex((uint32_t)channel));
}

static PyObject *
python_channel_set_alpha(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned int channel, alpha = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "II", &channel, &alpha));

    CHECK_NOTCURSES(channel_set_alpha(&channel, alpha));

    return Py_BuildValue("I", channel);
}

static PyObject *
python_channel_set_palindex(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long channel = {0};
    int idx = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "ki", &channel, &idx));

    uint32_t channel_fixed_size = (uint32_t)channel;

    CHECK_NOTCURSES(channel_set_palindex(&channel_fixed_size, idx));

    return Py_BuildValue("k", (unsigned long)channel_fixed_size);
}

static PyObject *
python_channel_default_p(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned int channel = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "I", &channel));

    return PyBool_FromLong((long)channel_default_p(channel));
}

static PyObject *
python_channel_palindex_p(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned int channel = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "I", &channel));

    return PyBool_FromLong((long)channel_palindex_p(channel));
}

static PyObject *
python_channel_set_default(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned int channel = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "I", &channel));

    return Py_BuildValue("I", channel_set_default(&channel));
}

static PyObject *
python_channels_bchannel(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long channels = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "K", &channels));

    return Py_BuildValue("k", (unsigned long)channels_bchannel((uint64_t)channels));
}

static PyObject *
python_channels_fchannel(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long channels = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "K", &channels));

    return Py_BuildValue("k", (unsigned long)channels_fchannel((uint64_t)channels));
}

static PyObject *
python_channels_set_bchannel(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long channels = {0};
    unsigned long channel = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "Kk", &channels, &channel));

    uint64_t channels_fixed_size = (uint64_t)channels;

    return Py_BuildValue("K", (unsigned long long)channels_set_bchannel(&channels_fixed_size, (uint32_t)channel));
}

static PyObject *
python_channels_set_fchannel(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long channels = {0};
    unsigned long channel = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "Kk", &channels, &channel));

    uint64_t channels_fixed_size = (uint64_t)channels;

    return Py_BuildValue("K", (unsigned long long)channels_set_fchannel(&channels_fixed_size, (uint32_t)channel));
}

static PyObject *
python_channels_combine(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long fchan, bchan = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "kk", &fchan, &bchan));

    return Py_BuildValue("K", (unsigned long long)channels_combine((uint32_t)fchan, (uint32_t)bchan));
}

static PyObject *
python_channels_fg_palindex(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long channels = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "K", &channels));

    return Py_BuildValue("k", (unsigned long)channels_fg_palindex((uint64_t)channels));
}

static PyObject *
python_channels_bg_palindex(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long channels = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "K", &channels));

    return Py_BuildValue("k", (unsigned long)channels_bg_palindex((uint64_t)channels));
}

static PyObject *
python_channels_fg_rgb(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long channels = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "K", &channels));

    return Py_BuildValue("I", channels_fg_rgb(channels));
}

static PyObject *
python_channels_bg_rgb(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long channels = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "K", &channels));

    return Py_BuildValue("I", channels_bg_rgb(channels));
}

static PyObject *
python_channels_fg_alpha(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long channels = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "K", &channels));

    return Py_BuildValue("I", channels_fg_alpha(channels));
}

static PyObject *
python_channels_bg_alpha(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long channels = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "K", &channels));

    return Py_BuildValue("I", channels_bg_alpha(channels));
}

static PyObject *
python_channels_fg_rgb8(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long channels = {0};
    unsigned int r, g, b = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "K", &channels));

    channels_fg_rgb8((uint64_t)channels, &r, &g, &b);

    return Py_BuildValue("III", r, g, b);
}

static PyObject *
python_channels_bg_rgb8(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long channels = {0};
    unsigned int r, g, b = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "K", &channels));

    channels_bg_rgb8((uint64_t)channels, &r, &g, &b);

    return Py_BuildValue("III", r, g, b);
}

static PyObject *
python_channels_set_fg_rgb8(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long channels = {0};
    int r, g, b = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "Kiii", &channels, &r, &g, &b));

    uint64_t channels_fixed_size = (uint64_t)channels;

    CHECK_NOTCURSES(channels_set_fg_rgb8(&channels_fixed_size, r, g, b));

    return Py_BuildValue("K", (unsigned long long)channels_fixed_size);
}

static PyObject *
python_channels_set_fg_rgb8_clipped(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long channels = {0};
    int r, g, b = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "Kiii", &channels, &r, &g, &b));

    uint64_t channels_fixed_size = (uint64_t)channels;

    channels_set_fg_rgb8_clipped(&channels_fixed_size, r, g, b);

    return Py_BuildValue("K", (unsigned long long)channels_fixed_size);
}

static PyObject *
python_channels_set_fg_alpha(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long channels = {0};
    unsigned int alpha = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "KI", &channels, &alpha));

    uint64_t channels_fixed_size = (uint64_t)channels;

    CHECK_NOTCURSES(channels_set_fg_alpha(&channels_fixed_size, alpha));

    return Py_BuildValue("K", (unsigned long long)channels_fixed_size);
}

static PyObject *
python_channels_set_fg_palindex(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long channels = {0};
    int idx = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "Ki", &channels, &idx));

    uint64_t channels_fixed_size = (uint64_t)channels;

    CHECK_NOTCURSES(channels_set_fg_palindex(&channels_fixed_size, idx));

    return Py_BuildValue("K", (unsigned long long)channels_fixed_size);
}

static PyObject *
python_channels_set_fg_rgb(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long channels = {0};
    unsigned int rgb = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "KI", &channels, &rgb));

    uint64_t channels_fixed_size = (uint64_t)channels;

    CHECK_NOTCURSES(channels_set_fg_rgb(&channels_fixed_size, rgb));

    return Py_BuildValue("K", (unsigned long long)channels_fixed_size);
}

static PyObject *
python_channels_set_bg_rgb8(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long channels = {0};
    int r, g, b = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "Kiii", &channels, &r, &g, &b));

    uint64_t channels_fixed_size = (uint64_t)channels;

    CHECK_NOTCURSES(channels_set_bg_rgb8(&channels_fixed_size, r, g, b));

    return Py_BuildValue("K", (unsigned long long)channels_fixed_size);
}

static PyObject *
python_channels_set_bg_rgb8_clipped(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long channels = {0};
    int r, g, b = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "Kiii", &channels, &r, &g, &b));

    uint64_t channels_fixed_size = (uint64_t)channels;

    channels_set_bg_rgb8_clipped(&channels_fixed_size, r, g, b);

    return Py_BuildValue("K", (unsigned long long)channels_fixed_size);
}

static PyObject *
python_channels_set_bg_alpha(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long channels = {0};
    unsigned int alpha = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "KI", &channels, &alpha));

    uint64_t channels_fixed_size = (uint64_t)channels;

    CHECK_NOTCURSES(channels_set_bg_alpha(&channels_fixed_size, alpha));

    return Py_BuildValue("K", (unsigned long long)channels_fixed_size);
}

static PyObject *
python_channels_set_bg_palindex(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long channels = {0};
    int idx = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "Ki", &channels, &idx));

    uint64_t channels_fixed_size = (uint64_t)channels;

    CHECK_NOTCURSES(channels_set_bg_palindex(&channels_fixed_size, idx));

    return Py_BuildValue("K", (unsigned long long)channels_fixed_size);
}

static PyObject *
python_channels_set_bg_rgb(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long channels = {0};
    unsigned int rgb = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "KI", &channels, &rgb));

    uint64_t channels_fixed_size = (uint64_t)channels;

    CHECK_NOTCURSES(channels_set_bg_rgb(&channels_fixed_size, rgb));

    return Py_BuildValue("K", (unsigned long long)channels_fixed_size);
}

static PyObject *
python_channels_fg_default_p(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long channels = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "K", &channels));

    return PyBool_FromLong((long)channels_fg_default_p((uint64_t)channels));
}

static PyObject *
python_channels_fg_palindex_p(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long channels = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "K", &channels));

    return PyBool_FromLong((long)channels_fg_palindex_p((uint64_t)channels));
}

static PyObject *
python_channels_bg_default_p(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long channels = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "K", &channels));

    return PyBool_FromLong((long)channels_bg_default_p((uint64_t)channels));
}

static PyObject *
python_channels_bg_palindex_p(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long channels = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "K", &channels));

    return PyBool_FromLong((long)channels_bg_palindex_p((uint64_t)channels));
}

static PyObject *
python_channels_set_fg_default(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long channels = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "K", &channels));

    uint64_t channels_fixed_size = (uint64_t)channels;

    return Py_BuildValue("K", (unsigned long long)channels_set_fg_default(&channels_fixed_size));
}

static PyObject *
python_channels_set_bg_default(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long channels = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "K", &channels));

    uint64_t channels_fixed_size = (uint64_t)channels;

    return Py_BuildValue("K", (unsigned long long)channels_set_bg_default(&channels_fixed_size));
}

PyMethodDef ChannelsFunctions[] = {
    {"channels_rgb_initializer", (PyCFunction)python_channels_rgb_initializer, METH_VARARGS, "Initialize a 64-bit channel pair with specified RGB fg/bg."},
    {"channel_rgb_initializer", (PyCFunction)python_channel_rgb_initializer, METH_VARARGS, "Initialize a 32-bit single channel with specified RGB."},
    {"channel_r", (PyCFunction)python_channel_r, METH_VARARGS, "Extract the 8-bit red component from a 32-bit channel."},
    {"channel_g", (PyCFunction)python_channel_g, METH_VARARGS, "Extract the 8-bit green component from a 32-bit channel."},
    {"channel_b", (PyCFunction)python_channel_b, METH_VARARGS, "Extract the 8-bit blue component from a 32-bit channel."},
    {"channel_rgb8", (PyCFunction)python_channel_rgb8, METH_VARARGS, "Extract the three 8-bit R/G/B components from a 32-bit channel."},
    {"channel_set_rgb8", (PyCFunction)python_channel_set_rgb8, METH_VARARGS, "Set the three 8-bit components of a 32-bit channel, and mark it as not using the default color. Retain the other bits unchanged."},
    {"channel_set_rgb8_clipped", (PyCFunction)python_channel_set_rgb8_clipped, METH_VARARGS, "Set the three 8-bit components of a 32-bit channel, and mark it as not using the default color. Retain the other bits unchanged. r, g, and b will be clipped to the range [0..255]."},
    {"channel_set", (PyCFunction)python_channel_set, METH_VARARGS, "Set the three 8-bit components of a 32-bit channel from a provide an assembled, packed 24 bits of rgb."},
    {"channel_alpha", (PyCFunction)python_channel_alpha, METH_VARARGS, "Extract 2 bits of foreground alpha from 'channels', shifted to LSBs."},
    {"channel_palindex", (PyCFunction)python_channel_palindex, METH_VARARGS, NULL},
    {"channel_set_alpha", (PyCFunction)python_channel_set_alpha, METH_VARARGS, "Set the 2-bit alpha component of the 32-bit channel."},
    {"channel_set_palindex", (PyCFunction)python_channel_set_palindex, METH_VARARGS, NULL},
    {"channel_default_p", (PyCFunction)python_channel_default_p, METH_VARARGS, "Is this channel using the \"default color\" rather than RGB/palette-indexed?"},
    {"channel_palindex_p", (PyCFunction)python_channel_palindex_p, METH_VARARGS, "Is this channel using palette-indexed color rather than RGB?"},
    {"channel_set_palindex", (PyCFunction)python_channel_set_palindex, METH_VARARGS, "Is this channel using palette-indexed color rather than RGB?"},
    {"channel_set_default", (PyCFunction)python_channel_set_default, METH_VARARGS, "Mark the channel as using its default color, which also marks it opaque."},
    {"channels_bchannel", (PyCFunction)python_channels_bchannel, METH_VARARGS, "Extract the 32-bit background channel from a channel pair."},
    {"channels_fchannel", (PyCFunction)python_channels_fchannel, METH_VARARGS, "Extract the 32-bit foreground channel from a channel pair."},
    {"channels_set_bchannel", (PyCFunction)python_channels_set_bchannel, METH_VARARGS, "Set the 32-bit background channel of a channel pair."},
    {"channels_set_fchannel", (PyCFunction)python_channels_set_fchannel, METH_VARARGS, "Set the 32-bit foreground channel of a channel pair."},
    {"channels_combine", (PyCFunction)python_channels_combine, METH_VARARGS, NULL},
    {"channels_fg_palindex", (PyCFunction)python_channels_fg_palindex, METH_VARARGS, NULL},
    {"channels_bg_palindex", (PyCFunction)python_channels_bg_palindex, METH_VARARGS, NULL},
    {"channels_fg_rgb", (PyCFunction)python_channels_fg_rgb, METH_VARARGS, "Extract 24 bits of foreground RGB from 'channels', shifted to LSBs."},
    {"channels_bg_rgb", (PyCFunction)python_channels_bg_rgb, METH_VARARGS, "Extract 24 bits of background RGB from 'channels', shifted to LSBs."},
    {"channels_fg_alpha", (PyCFunction)python_channels_fg_alpha, METH_VARARGS, "Extract 2 bits of foreground alpha from 'channels', shifted to LSBs."},
    {"channels_bg_alpha", (PyCFunction)python_channels_bg_alpha, METH_VARARGS, "Extract 2 bits of background alpha from 'channels', shifted to LSBs."},
    {"channels_fg_rgb8", (PyCFunction)python_channels_fg_rgb8, METH_VARARGS, "Extract 24 bits of foreground RGB from 'channels', split into subchannels."},
    {"channels_bg_rgb8", (PyCFunction)python_channels_bg_rgb8, METH_VARARGS, "Extract 24 bits of background RGB from 'channels', split into subchannels."},
    {"channels_set_fg_rgb8", (PyCFunction)python_channels_set_fg_rgb8, METH_VARARGS, "Set the r, g, and b channels for the foreground component of this 64-bit 'channels' variable, and mark it as not using the default color."},
    {"channels_set_fg_rgb8_clipped", (PyCFunction)python_channels_set_fg_rgb8_clipped, METH_VARARGS, "Set the r, g, and b channels for the foreground component of this 64-bit 'channels' variable but clips to [0..255]."},
    {"channels_set_fg_alpha", (PyCFunction)python_channels_set_fg_alpha, METH_VARARGS, "Set the 2-bit alpha component of the foreground channel."},
    {"channels_set_fg_palindex", (PyCFunction)python_channels_set_fg_palindex, METH_VARARGS, NULL},
    {"channels_set_fg_rgb", (PyCFunction)python_channels_set_fg_rgb, METH_VARARGS, "Set the r, g, and b channels for the foreground component of this 64-bit 'channels' variable but set an assembled 24 bit channel at once."},
    {"channels_set_bg_rgb8", (PyCFunction)python_channels_set_bg_rgb8, METH_VARARGS, "Set the r, g, and b channels for the background component of this 64-bit 'channels' variable, and mark it as not using the default color."},
    {"channels_set_bg_rgb8_clipped", (PyCFunction)python_channels_set_bg_rgb8_clipped, METH_VARARGS, "Set the r, g, and b channels for the background component of this 64-bit 'channels' variable but clips to [0..255]."},
    {"channels_set_bg_alpha", (PyCFunction)python_channels_set_bg_alpha, METH_VARARGS, "Set the 2-bit alpha component of the background channel."},
    {"channels_set_bg_palindex", (PyCFunction)python_channels_set_bg_palindex, METH_VARARGS, NULL},
    {"channels_set_bg_rgb", (PyCFunction)python_channels_set_bg_rgb, METH_VARARGS, "Set the r, g, and b channels for the background component of this 64-bit 'channels' variable but set an assembled 24 bit channel at once."},
    {"channels_fg_default_p", (PyCFunction)python_channels_fg_default_p, METH_VARARGS, "Is the foreground using the \"default foreground color\"?"},
    {"channels_fg_palindex_p", (PyCFunction)python_channels_fg_palindex_p, METH_VARARGS, "Is the foreground using indexed palette color?"},
    {"channels_bg_default_p", (PyCFunction)python_channels_bg_default_p, METH_VARARGS, "Is the background using the \"default background color\"? The \"defaultbackground color\" must generally be used to take advantage of terminal-effected transparency."},
    {"channels_bg_palindex_p", (PyCFunction)python_channels_bg_palindex_p, METH_VARARGS, "Is the background using indexed palette color?"},
    {"channels_set_fg_default", (PyCFunction)python_channels_set_fg_default, METH_VARARGS, "Mark the foreground channel as using its default color."},
    {"channels_set_bg_default", (PyCFunction)python_channels_set_bg_default, METH_VARARGS, "Mark the background channel as using its default color."},
    {NULL, NULL, 0, NULL},
};
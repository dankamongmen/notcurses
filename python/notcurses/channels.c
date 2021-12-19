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
python_ncchannels_rgb_initializer(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long fr, fg, fb, br, bg, bb = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "KKKKKK", &fr, &fg, &fb, &br, &bg, &bb));

    unsigned long long ncchannels = NCCHANNELS_INITIALIZER(fr, fg, fb, br, bg, bb);

    return Py_BuildValue("K", ncchannels);
}

static PyObject *
python_ncchannel_rgb_initializer(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long r, g, b = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "kkk", &r, &g, &b));

    unsigned long ncchannel = NCCHANNEL_INITIALIZER(r, g, b);

    return Py_BuildValue("k", ncchannel);
}

static PyObject *
python_ncchannel_r(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long ncchannel = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "k", &ncchannel));

    unsigned long r = ncchannel_r((uint32_t)ncchannel);

    return Py_BuildValue("k", r);
}

static PyObject *
python_ncchannel_g(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long ncchannel = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "k", &ncchannel));

    unsigned long g = ncchannel_r((uint32_t)ncchannel);

    return Py_BuildValue("k", g);
}

static PyObject *
python_ncchannel_b(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long ncchannel = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "k", &ncchannel));

    unsigned long b = ncchannel_b((uint32_t)ncchannel);

    return Py_BuildValue("k", b);
}

static PyObject *
python_ncchannel_rgb8(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long ncchannel = {0};
    unsigned int r, g, b = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "k", &ncchannel));

    ncchannel_rgb8((uint32_t)ncchannel, &r, &g, &b);

    return Py_BuildValue("III", r, g, b);
}

static PyObject *
python_ncchannel_set_rgb8(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long ncchannel = {0};
    unsigned r, g, b = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "kkkk", &ncchannel, &r, &g, &b));

    uint32_t ncchannel_fixed_size = (uint32_t)ncchannel;

    CHECK_NOTCURSES(ncchannel_set_rgb8(&ncchannel_fixed_size, r, g, b));

    return Py_BuildValue("k", (unsigned long)ncchannel_fixed_size);
}

static PyObject *
python_ncchannel_set_rgb8_clipped(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned int ncchannel = {0};
    int r, g, b = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "Iiii", &ncchannel, &r, &g, &b));

    ncchannel_set_rgb8_clipped(&ncchannel, r, g, b);

    return Py_BuildValue("k", ncchannel);
}

static PyObject *
python_ncchannel_set(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned int ncchannel, rgb = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "II", &ncchannel, &rgb));

    CHECK_NOTCURSES(ncchannel_set(&ncchannel, rgb));

    return Py_BuildValue("I", ncchannel);
}

static PyObject *
python_ncchannel_alpha(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned int ncchannel = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "I", &ncchannel));

    return Py_BuildValue("I", ncchannel_alpha(ncchannel));
}

static PyObject *
python_ncchannel_palindex(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long ncchannel = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "k", &ncchannel));

    return Py_BuildValue("I", ncchannel_palindex((uint32_t)ncchannel));
}

static PyObject *
python_ncchannel_set_alpha(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned int ncchannel, alpha = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "II", &ncchannel, &alpha));

    CHECK_NOTCURSES(ncchannel_set_alpha(&ncchannel, alpha));

    return Py_BuildValue("I", ncchannel);
}

static PyObject *
python_ncchannel_set_palindex(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long ncchannel = {0};
    unsigned int idx = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "kI", &ncchannel, &idx));

    uint32_t ncchannel_fixed_size = (uint32_t)ncchannel;

    CHECK_NOTCURSES(ncchannel_set_palindex(&ncchannel_fixed_size, idx));

    return Py_BuildValue("k", (unsigned long)ncchannel_fixed_size);
}

static PyObject *
python_ncchannel_default_p(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned int ncchannel = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "I", &ncchannel));

    return PyBool_FromLong((long)ncchannel_default_p(ncchannel));
}

static PyObject *
python_ncchannel_palindex_p(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned int ncchannel = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "I", &ncchannel));

    return PyBool_FromLong((long)ncchannel_palindex_p(ncchannel));
}

static PyObject *
python_ncchannel_set_default(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned int ncchannel = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "I", &ncchannel));

    return Py_BuildValue("I", ncchannel_set_default(&ncchannel));
}

static PyObject *
python_ncchannels_bchannel(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long ncchannels = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "K", &ncchannels));

    return Py_BuildValue("k", (unsigned long)ncchannels_bchannel((uint64_t)ncchannels));
}

static PyObject *
python_ncchannels_fchannel(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long ncchannels = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "K", &ncchannels));

    return Py_BuildValue("k", (unsigned long)ncchannels_fchannel((uint64_t)ncchannels));
}

static PyObject *
python_ncchannels_set_bchannel(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long ncchannels = {0};
    unsigned long ncchannel = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "Kk", &ncchannels, &ncchannel));

    uint64_t ncchannels_fixed_size = (uint64_t)ncchannels;

    return Py_BuildValue("K", (unsigned long long)ncchannels_set_bchannel(&ncchannels_fixed_size, (uint32_t)ncchannel));
}

static PyObject *
python_ncchannels_set_fchannel(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long ncchannels = {0};
    unsigned long ncchannel = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "Kk", &ncchannels, &ncchannel));

    uint64_t ncchannels_fixed_size = (uint64_t)ncchannels;

    return Py_BuildValue("K", (unsigned long long)ncchannels_set_fchannel(&ncchannels_fixed_size, (uint32_t)ncchannel));
}

static PyObject *
python_ncchannels_combine(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long fchan, bchan = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "kk", &fchan, &bchan));

    return Py_BuildValue("K", (unsigned long long)ncchannels_combine((uint32_t)fchan, (uint32_t)bchan));
}

static PyObject *
python_ncchannels_fg_palindex(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long ncchannels = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "K", &ncchannels));

    return Py_BuildValue("k", (unsigned long)ncchannels_fg_palindex((uint64_t)ncchannels));
}

static PyObject *
python_ncchannels_bg_palindex(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long ncchannels = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "K", &ncchannels));

    return Py_BuildValue("k", (unsigned long)ncchannels_bg_palindex((uint64_t)ncchannels));
}

static PyObject *
python_ncchannels_fg_rgb(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long ncchannels = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "K", &ncchannels));

    return Py_BuildValue("I", ncchannels_fg_rgb(ncchannels));
}

static PyObject *
python_ncchannels_bg_rgb(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long ncchannels = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "K", &ncchannels));

    return Py_BuildValue("I", ncchannels_bg_rgb(ncchannels));
}

static PyObject *
python_ncchannels_fg_alpha(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long ncchannels = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "K", &ncchannels));

    return Py_BuildValue("I", ncchannels_fg_alpha(ncchannels));
}

static PyObject *
python_ncchannels_bg_alpha(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long ncchannels = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "K", &ncchannels));

    return Py_BuildValue("I", ncchannels_bg_alpha(ncchannels));
}

static PyObject *
python_ncchannels_fg_rgb8(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long ncchannels = {0};
    unsigned int r, g, b = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "K", &ncchannels));

    ncchannels_fg_rgb8((uint64_t)ncchannels, &r, &g, &b);

    return Py_BuildValue("III", r, g, b);
}

static PyObject *
python_ncchannels_bg_rgb8(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long ncchannels = {0};
    unsigned int r, g, b = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "K", &ncchannels));

    ncchannels_bg_rgb8((uint64_t)ncchannels, &r, &g, &b);

    return Py_BuildValue("III", r, g, b);
}

static PyObject *
python_ncchannels_set_fg_rgb8(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long ncchannels = {0};
    unsigned r, g, b = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "Kkkk", &ncchannels, &r, &g, &b));

    uint64_t ncchannels_fixed_size = (uint64_t)ncchannels;

    CHECK_NOTCURSES(ncchannels_set_fg_rgb8(&ncchannels_fixed_size, r, g, b));

    return Py_BuildValue("K", (unsigned long long)ncchannels_fixed_size);
}

static PyObject *
python_ncchannels_set_fg_rgb8_clipped(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long ncchannels = {0};
    int r, g, b = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "Kiii", &ncchannels, &r, &g, &b));

    uint64_t ncchannels_fixed_size = (uint64_t)ncchannels;

    ncchannels_set_fg_rgb8_clipped(&ncchannels_fixed_size, r, g, b);

    return Py_BuildValue("K", (unsigned long long)ncchannels_fixed_size);
}

static PyObject *
python_ncchannels_set_fg_alpha(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long ncchannels = {0};
    unsigned int alpha = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "KI", &ncchannels, &alpha));

    uint64_t ncchannels_fixed_size = (uint64_t)ncchannels;

    CHECK_NOTCURSES(ncchannels_set_fg_alpha(&ncchannels_fixed_size, alpha));

    return Py_BuildValue("K", (unsigned long long)ncchannels_fixed_size);
}

static PyObject *
python_ncchannels_set_fg_palindex(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long ncchannels = {0};
    unsigned int idx = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "KI", &ncchannels, &idx));

    uint64_t ncchannels_fixed_size = (uint64_t)ncchannels;

    CHECK_NOTCURSES(ncchannels_set_fg_palindex(&ncchannels_fixed_size, idx));

    return Py_BuildValue("K", (unsigned long long)ncchannels_fixed_size);
}

static PyObject *
python_ncchannels_set_fg_rgb(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long ncchannels = {0};
    unsigned int rgb = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "KI", &ncchannels, &rgb));

    uint64_t ncchannels_fixed_size = (uint64_t)ncchannels;

    CHECK_NOTCURSES(ncchannels_set_fg_rgb(&ncchannels_fixed_size, rgb));

    return Py_BuildValue("K", (unsigned long long)ncchannels_fixed_size);
}

static PyObject *
python_ncchannels_set_bg_rgb8(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long ncchannels = {0};
    unsigned r, g, b = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "Kkkk", &ncchannels, &r, &g, &b));

    uint64_t ncchannels_fixed_size = (uint64_t)ncchannels;

    CHECK_NOTCURSES(ncchannels_set_bg_rgb8(&ncchannels_fixed_size, r, g, b));

    return Py_BuildValue("K", (unsigned long long)ncchannels_fixed_size);
}

static PyObject *
python_ncchannels_set_bg_rgb8_clipped(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long ncchannels = {0};
    int r, g, b = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "Kiii", &ncchannels, &r, &g, &b));

    uint64_t ncchannels_fixed_size = (uint64_t)ncchannels;

    ncchannels_set_bg_rgb8_clipped(&ncchannels_fixed_size, r, g, b);

    return Py_BuildValue("K", (unsigned long long)ncchannels_fixed_size);
}

static PyObject *
python_ncchannels_set_bg_alpha(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long ncchannels = {0};
    unsigned int alpha = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "KI", &ncchannels, &alpha));

    uint64_t ncchannels_fixed_size = (uint64_t)ncchannels;

    CHECK_NOTCURSES(ncchannels_set_bg_alpha(&ncchannels_fixed_size, alpha));

    return Py_BuildValue("K", (unsigned long long)ncchannels_fixed_size);
}

static PyObject *
python_ncchannels_set_bg_palindex(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long ncchannels = {0};
    unsigned int idx = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "KI", &ncchannels, &idx));

    uint64_t ncchannels_fixed_size = (uint64_t)ncchannels;

    CHECK_NOTCURSES(ncchannels_set_bg_palindex(&ncchannels_fixed_size, idx));

    return Py_BuildValue("K", (unsigned long long)ncchannels_fixed_size);
}

static PyObject *
python_ncchannels_set_bg_rgb(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long ncchannels = {0};
    unsigned int rgb = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "KI", &ncchannels, &rgb));

    uint64_t ncchannels_fixed_size = (uint64_t)ncchannels;

    CHECK_NOTCURSES(ncchannels_set_bg_rgb(&ncchannels_fixed_size, rgb));

    return Py_BuildValue("K", (unsigned long long)ncchannels_fixed_size);
}

static PyObject *
python_ncchannels_fg_default_p(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long ncchannels = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "K", &ncchannels));

    return PyBool_FromLong((long)ncchannels_fg_default_p((uint64_t)ncchannels));
}

static PyObject *
python_ncchannels_fg_palindex_p(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long ncchannels = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "K", &ncchannels));

    return PyBool_FromLong((long)ncchannels_fg_palindex_p((uint64_t)ncchannels));
}

static PyObject *
python_ncchannels_bg_default_p(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long ncchannels = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "K", &ncchannels));

    return PyBool_FromLong((long)ncchannels_bg_default_p((uint64_t)ncchannels));
}

static PyObject *
python_ncchannels_bg_palindex_p(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long ncchannels = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "K", &ncchannels));

    return PyBool_FromLong((long)ncchannels_bg_palindex_p((uint64_t)ncchannels));
}

static PyObject *
python_ncchannels_set_fg_default(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long ncchannels = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "K", &ncchannels));

    uint64_t ncchannels_fixed_size = (uint64_t)ncchannels;

    return Py_BuildValue("K", (unsigned long long)ncchannels_set_fg_default(&ncchannels_fixed_size));
}

static PyObject *
python_ncchannels_set_bg_default(PyObject *Py_UNUSED(self), PyObject *args)
{
    unsigned long long ncchannels = {0};

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "K", &ncchannels));

    uint64_t ncchannels_fixed_size = (uint64_t)ncchannels;

    return Py_BuildValue("K", (unsigned long long)ncchannels_set_bg_default(&ncchannels_fixed_size));
}

PyMethodDef ChannelsFunctions[] = {
    {"ncchannels_rgb_initializer", (PyCFunction)python_ncchannels_rgb_initializer, METH_VARARGS, "Initialize a 64-bit ncchannel pair with specified RGB fg/bg."},
    {"ncchannel_rgb_initializer", (PyCFunction)python_ncchannel_rgb_initializer, METH_VARARGS, "Initialize a 32-bit single ncchannel with specified RGB."},
    {"ncchannel_r", (PyCFunction)python_ncchannel_r, METH_VARARGS, "Extract the 8-bit red component from a 32-bit ncchannel."},
    {"ncchannel_g", (PyCFunction)python_ncchannel_g, METH_VARARGS, "Extract the 8-bit green component from a 32-bit ncchannel."},
    {"ncchannel_b", (PyCFunction)python_ncchannel_b, METH_VARARGS, "Extract the 8-bit blue component from a 32-bit ncchannel."},
    {"ncchannel_rgb8", (PyCFunction)python_ncchannel_rgb8, METH_VARARGS, "Extract the three 8-bit R/G/B components from a 32-bit ncchannel."},
    {"ncchannel_set_rgb8", (PyCFunction)python_ncchannel_set_rgb8, METH_VARARGS, "Set the three 8-bit components of a 32-bit ncchannel, and mark it as not using the default color. Retain the other bits unchanged."},
    {"ncchannel_set_rgb8_clipped", (PyCFunction)python_ncchannel_set_rgb8_clipped, METH_VARARGS, "Set the three 8-bit components of a 32-bit ncchannel, and mark it as not using the default color. Retain the other bits unchanged. r, g, and b will be clipped to the range [0..255]."},
    {"ncchannel_set", (PyCFunction)python_ncchannel_set, METH_VARARGS, "Set the three 8-bit components of a 32-bit ncchannel from a provide an assembled, packed 24 bits of rgb."},
    {"ncchannel_alpha", (PyCFunction)python_ncchannel_alpha, METH_VARARGS, "Extract 2 bits of foreground alpha from 'ncchannels', shifted to LSBs."},
    {"ncchannel_palindex", (PyCFunction)python_ncchannel_palindex, METH_VARARGS, NULL},
    {"ncchannel_set_alpha", (PyCFunction)python_ncchannel_set_alpha, METH_VARARGS, "Set the 2-bit alpha component of the 32-bit ncchannel."},
    {"ncchannel_set_palindex", (PyCFunction)python_ncchannel_set_palindex, METH_VARARGS, NULL},
    {"ncchannel_default_p", (PyCFunction)python_ncchannel_default_p, METH_VARARGS, "Is this ncchannel using the \"default color\" rather than RGB/palette-indexed?"},
    {"ncchannel_palindex_p", (PyCFunction)python_ncchannel_palindex_p, METH_VARARGS, "Is this ncchannel using palette-indexed color rather than RGB?"},
    {"ncchannel_set_palindex", (PyCFunction)python_ncchannel_set_palindex, METH_VARARGS, "Is this ncchannel using palette-indexed color rather than RGB?"},
    {"ncchannel_set_default", (PyCFunction)python_ncchannel_set_default, METH_VARARGS, "Mark the ncchannel as using its default color, which also marks it opaque."},
    {"ncchannels_bchannel", (PyCFunction)python_ncchannels_bchannel, METH_VARARGS, "Extract the 32-bit background ncchannel from a ncchannel pair."},
    {"ncchannels_fchannel", (PyCFunction)python_ncchannels_fchannel, METH_VARARGS, "Extract the 32-bit foreground ncchannel from a ncchannel pair."},
    {"ncchannels_set_bchannel", (PyCFunction)python_ncchannels_set_bchannel, METH_VARARGS, "Set the 32-bit background ncchannel of a ncchannel pair."},
    {"ncchannels_set_fchannel", (PyCFunction)python_ncchannels_set_fchannel, METH_VARARGS, "Set the 32-bit foreground ncchannel of a ncchannel pair."},
    {"ncchannels_combine", (PyCFunction)python_ncchannels_combine, METH_VARARGS, NULL},
    {"ncchannels_fg_palindex", (PyCFunction)python_ncchannels_fg_palindex, METH_VARARGS, NULL},
    {"ncchannels_bg_palindex", (PyCFunction)python_ncchannels_bg_palindex, METH_VARARGS, NULL},
    {"ncchannels_fg_rgb", (PyCFunction)python_ncchannels_fg_rgb, METH_VARARGS, "Extract 24 bits of foreground RGB from 'ncchannels', shifted to LSBs."},
    {"ncchannels_bg_rgb", (PyCFunction)python_ncchannels_bg_rgb, METH_VARARGS, "Extract 24 bits of background RGB from 'ncchannels', shifted to LSBs."},
    {"ncchannels_fg_alpha", (PyCFunction)python_ncchannels_fg_alpha, METH_VARARGS, "Extract 2 bits of foreground alpha from 'ncchannels', shifted to LSBs."},
    {"ncchannels_bg_alpha", (PyCFunction)python_ncchannels_bg_alpha, METH_VARARGS, "Extract 2 bits of background alpha from 'ncchannels', shifted to LSBs."},
    {"ncchannels_fg_rgb8", (PyCFunction)python_ncchannels_fg_rgb8, METH_VARARGS, "Extract 24 bits of foreground RGB from 'ncchannels', split into subncchannels."},
    {"ncchannels_bg_rgb8", (PyCFunction)python_ncchannels_bg_rgb8, METH_VARARGS, "Extract 24 bits of background RGB from 'ncchannels', split into subncchannels."},
    {"ncchannels_set_fg_rgb8", (PyCFunction)python_ncchannels_set_fg_rgb8, METH_VARARGS, "Set the r, g, and b ncchannels for the foreground component of this 64-bit 'ncchannels' variable, and mark it as not using the default color."},
    {"ncchannels_set_fg_rgb8_clipped", (PyCFunction)python_ncchannels_set_fg_rgb8_clipped, METH_VARARGS, "Set the r, g, and b ncchannels for the foreground component of this 64-bit 'ncchannels' variable but clips to [0..255]."},
    {"ncchannels_set_fg_alpha", (PyCFunction)python_ncchannels_set_fg_alpha, METH_VARARGS, "Set the 2-bit alpha component of the foreground ncchannel."},
    {"ncchannels_set_fg_palindex", (PyCFunction)python_ncchannels_set_fg_palindex, METH_VARARGS, NULL},
    {"ncchannels_set_fg_rgb", (PyCFunction)python_ncchannels_set_fg_rgb, METH_VARARGS, "Set the r, g, and b ncchannels for the foreground component of this 64-bit 'ncchannels' variable but set an assembled 24 bit ncchannel at once."},
    {"ncchannels_set_bg_rgb8", (PyCFunction)python_ncchannels_set_bg_rgb8, METH_VARARGS, "Set the r, g, and b ncchannels for the background component of this 64-bit 'ncchannels' variable, and mark it as not using the default color."},
    {"ncchannels_set_bg_rgb8_clipped", (PyCFunction)python_ncchannels_set_bg_rgb8_clipped, METH_VARARGS, "Set the r, g, and b ncchannels for the background component of this 64-bit 'ncchannels' variable but clips to [0..255]."},
    {"ncchannels_set_bg_alpha", (PyCFunction)python_ncchannels_set_bg_alpha, METH_VARARGS, "Set the 2-bit alpha component of the background ncchannel."},
    {"ncchannels_set_bg_palindex", (PyCFunction)python_ncchannels_set_bg_palindex, METH_VARARGS, NULL},
    {"ncchannels_set_bg_rgb", (PyCFunction)python_ncchannels_set_bg_rgb, METH_VARARGS, "Set the r, g, and b ncchannels for the background component of this 64-bit 'ncchannels' variable but set an assembled 24 bit ncchannel at once."},
    {"ncchannels_fg_default_p", (PyCFunction)python_ncchannels_fg_default_p, METH_VARARGS, "Is the foreground using the \"default foreground color\"?"},
    {"ncchannels_fg_palindex_p", (PyCFunction)python_ncchannels_fg_palindex_p, METH_VARARGS, "Is the foreground using indexed palette color?"},
    {"ncchannels_bg_default_p", (PyCFunction)python_ncchannels_bg_default_p, METH_VARARGS, "Is the background using the \"default background color\"? The \"defaultbackground color\" must generally be used to take advantage of terminal-effected transparency."},
    {"ncchannels_bg_palindex_p", (PyCFunction)python_ncchannels_bg_palindex_p, METH_VARARGS, "Is the background using indexed palette color?"},
    {"ncchannels_set_fg_default", (PyCFunction)python_ncchannels_set_fg_default, METH_VARARGS, "Mark the foreground ncchannel as using its default color."},
    {"ncchannels_set_bg_default", (PyCFunction)python_ncchannels_set_bg_default, METH_VARARGS, "Mark the background ncchannel as using its default color."},
    {NULL, NULL, 0, NULL},
};

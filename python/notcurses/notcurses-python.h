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
#pragma once
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <notcurses/notcurses.h>
#include <notcurses/direct.h>

// Define structs

typedef struct
{
    PyObject_HEAD;
    struct notcurses *notcurses_ptr;
} NotcursesObject;

typedef struct
{
    PyObject_HEAD;
    struct ncplane *ncplane_ptr;
} NcPlaneObject;

typedef struct
{
    PyObject_HEAD;
    struct ncuplot *ncuplot_ptr;
} NcUplotObject;

typedef struct
{
    PyObject_HEAD;
    struct ncdplot *ncdplot_ptr;
} NcDplotObject;

typedef struct
{
    PyObject_HEAD;
    struct ncprogbar *ncprogbar_ptr;
} NcProgBarObject;

typedef struct
{
    PyObject_HEAD;
    struct ncfdplane *ncfdplane_ptr;
} NcFdPlaneObject;

typedef struct
{
    PyObject_HEAD;
    struct ncsubproc *ncsubproc_ptr;
} NcSubProcObject;

typedef struct
{
    PyObject_HEAD;
    struct ncselector *ncselector_ptr;
} NcSelectorObject;

typedef struct
{
    PyObject_HEAD;
    struct ncplane *ncmultiselector_ptr;
} NcMultiSelectorObject;

typedef struct
{
    PyObject_HEAD;
    struct ncreader *ncreader_ptr;
} NcReaderObject;

typedef struct
{
    PyObject_HEAD;
    struct ncfadectx *ncfadectx_ptr;
} NcFadeCtxObject;

typedef struct
{
    PyObject_HEAD;
    struct nctablet *nctablet_ptr;
} NcTabletObject;

typedef struct
{
    PyObject_HEAD;
    struct ncreel *ncreel_ptr;
} NcReelObject;

typedef struct
{
    PyObject_HEAD;
    struct nctab *nctab_ptr;
} NcTabObject;

typedef struct
{
    PyObject_HEAD;
    struct nctabbed *nctabbed_ptr;
} NcTabbedObject;

typedef struct
{
    PyObject_HEAD;
    struct nccell nccell;
} NcCellObject;

typedef struct
{
    PyObject_HEAD;
    struct ncinput ncinput;
} NcInputObject;

typedef struct
{
    PyObject_HEAD;
    struct ncstats ncstats;
} NcStatsObject;

typedef struct
{
    PyObject_HEAD;
    struct palette256 palette256;
} Palette256Object;

// Functions

#define SPLIT __attribute__((used))

// // Channels

PyObject *python_channels_rgb_initializer(PyObject *, PyObject *);
PyObject *python_channel_rgb_initializer(PyObject *, PyObject *);
PyObject *python_channel_r(PyObject *, PyObject *);
PyObject *python_channel_g(PyObject *, PyObject *);
PyObject *python_channel_b(PyObject *, PyObject *);
PyObject *python_channel_rgb8(PyObject *, PyObject *);
PyObject *python_channel_set_rgb8(PyObject *, PyObject *);
PyObject *python_channel_set_rgb8_clipped(PyObject *, PyObject *);
PyObject *python_channel_set(PyObject *, PyObject *);
PyObject *python_channel_alpha(PyObject *, PyObject *);
PyObject *python_channel_palindex(PyObject *, PyObject *);
PyObject *python_channel_set_alpha(PyObject *, PyObject *);
PyObject *python_channel_set_palindex(PyObject *, PyObject *);
PyObject *python_channel_default_p(PyObject *, PyObject *);
PyObject *python_channel_palindex_p(PyObject *, PyObject *);
PyObject *python_channel_set_default(PyObject *, PyObject *);
PyObject *python_channels_bchannel(PyObject *, PyObject *);
PyObject *python_channels_fchannel(PyObject *, PyObject *);
PyObject *python_channels_set_bchannel(PyObject *, PyObject *);
PyObject *python_channels_set_fchannel(PyObject *, PyObject *);
PyObject *python_channels_combine(PyObject *, PyObject *);
PyObject *python_channels_fg_palindex(PyObject *, PyObject *);
PyObject *python_channels_bg_palindex(PyObject *, PyObject *);
PyObject *python_channels_fg_rgb(PyObject *, PyObject *);
PyObject *python_channels_bg_rgb(PyObject *, PyObject *);
PyObject *python_channels_fg_alpha(PyObject *, PyObject *);
PyObject *python_channels_bg_alpha(PyObject *, PyObject *);
PyObject *python_channels_fg_rgb8(PyObject *, PyObject *);
PyObject *python_channels_bg_rgb8(PyObject *, PyObject *);
PyObject *python_channels_set_fg_rgb8(PyObject *, PyObject *);
PyObject *python_channels_set_fg_rgb8_clipped(PyObject *, PyObject *);
PyObject *python_channels_set_fg_alpha(PyObject *, PyObject *);
PyObject *python_channels_set_fg_palindex(PyObject *, PyObject *);
PyObject *python_channels_set_fg_rgb(PyObject *, PyObject *);
PyObject *python_channels_set_bg_rgb8(PyObject *, PyObject *);
PyObject *python_channels_set_bg_rgb8_clipped(PyObject *, PyObject *);
PyObject *python_channels_set_bg_alpha(PyObject *, PyObject *);
PyObject *python_channels_set_bg_palindex(PyObject *, PyObject *);
PyObject *python_channels_set_bg_rgb(PyObject *, PyObject *);
PyObject *python_channels_fg_default_p(PyObject *, PyObject *);
PyObject *python_channels_fg_palindex_p(PyObject *, PyObject *);
PyObject *python_channels_bg_default_p(PyObject *, PyObject *);
PyObject *python_channels_bg_palindex_p(PyObject *, PyObject *);
PyObject *python_channels_set_fg_default(PyObject *, PyObject *);
PyObject *python_channels_set_bg_default(PyObject *, PyObject *);

extern PyMethodDef ChannelsFunctions[];

// // Misc

PyObject *python_notcurses_version(PyObject *, PyObject *);
PyObject *python_notcurses_version_components(PyObject *, PyObject *);
PyObject *python_ncstrwidth(PyObject *, PyObject *);

extern PyMethodDef MiscFunctions[];

// Helpers

static inline void PyObject_cleanup(PyObject **object)
{
    Py_XDECREF(*object);
}

#define CLEANUP_PY_OBJ __attribute__((cleanup(PyObject_cleanup)))

#define GNU_PY_CHECK(py_function)           \
    ({                                      \
        PyObject *new_object = py_function; \
        if (new_object == NULL)             \
        {                                   \
            return NULL;                    \
        }                                   \
        new_object;                         \
    })

#define GNU_PY_CHECK_INT(py_function)   \
    ({                                  \
        int return_value = py_function; \
        if (return_value < 0)           \
        {                               \
            return NULL;                \
        }                               \
        return_value;                   \
    })

#define CHECK_NOTCURSES(notcurses_func)                                                    \
    ({                                                                                     \
        int return_value = notcurses_func;                                                 \
        if (return_value < 0)                                                              \
        {                                                                                  \
            PyErr_Format(PyExc_RuntimeError, "Notcurses returned error %i", return_value); \
            return NULL;                                                                   \
        }                                                                                  \
        return_value;                                                                      \
    })

// SPDX-License-Identifier: Apache-2.0
/*
Copyright 2020 igo95862

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
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <notcurses/notcurses.h>
#include <notcurses/direct.h>
#include "structmember.h"

typedef struct
{
    PyObject_HEAD;
    uint64_t ncchannels_ptr;
} NcChannelsObject;

static PyMethodDef NcChannels_methods[] = {
    {NULL, NULL, 0, NULL},
};

static PyTypeObject NcChannelsType = {
    PyVarObject_HEAD_INIT(NULL, 0)
        .tp_name = "notcurses._notcurses._NcChannels",
    .tp_doc = "Notcurses Channels",
    .tp_basicsize = sizeof(NcChannelsObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
    .tp_init = NULL,
    .tp_methods = NcChannels_methods,
};

typedef struct
{
    PyObject_HEAD;
    struct ncplane *ncplane_ptr;
} NcPlaneObject;

static PyMethodDef NcPlane_methods[] = {
    {NULL, NULL, 0, NULL},
};

static PyTypeObject NcPlaneType = {
    PyVarObject_HEAD_INIT(NULL, 0)
        .tp_name = "notcurses._notcurses._NcPlane",
    .tp_doc = "Notcurses Plane",
    .tp_basicsize = sizeof(NcPlaneObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
    .tp_init = NULL,
    .tp_methods = NcPlane_methods,
};

typedef struct
{
    PyObject_HEAD;
    struct notcurses_options options;
    struct notcurses *notcurses_context_ptr;
} NotcursesContextObject;

static PyMethodDef NotcursesContext_methods[] = {
    {NULL, NULL, 0, NULL},
};

static PyTypeObject NotcursesContextType = {
    PyVarObject_HEAD_INIT(NULL, 0)
        .tp_name = "notcurses._notcurses._NotcursesContext",
    .tp_doc = "Notcurses Context",
    .tp_basicsize = sizeof(NotcursesContextObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
    .tp_methods = NotcursesContext_methods,
};

typedef struct
{
    PyObject_HEAD;
    struct ncdirect *ncdirect_ptr;
} NcDirectObject;

static PyMethodDef NcDirect_methods[] = {
    {NULL, NULL, 0, NULL},
};

static PyTypeObject NcDirectType = {
    PyVarObject_HEAD_INIT(NULL, 0)
        .tp_name = "notcurses._notcurses._NcDirect",
    .tp_doc = "Notcurses Direct",
    .tp_basicsize = sizeof(NcDirectObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
    .tp_methods = NcDirect_methods,
};

typedef struct
{
    PyObject_HEAD;
    long codepoint;
    int y_pos;
    int x_pos;
    bool is_alt;
    bool is_shift;
    bool is_ctrl;
    uint64_t seqnum;
} NcInputObject;

static PyMemberDef NcInput_members[] = {
    {"codepoint", T_LONG, offsetof(NcInputObject, codepoint), READONLY, "Codepoint"},
    {"y_pos", T_INT, offsetof(NcInputObject, y_pos), READONLY, "Y axis positions"},
    {"x_pos", T_INT, offsetof(NcInputObject, x_pos), READONLY, "X axis positions"},
    {"is_alt", T_BOOL, offsetof(NcInputObject, is_alt), READONLY, "Is alt pressed"},
    {"is_shift", T_BOOL, offsetof(NcInputObject, is_shift), READONLY, "Is shift pressed"},
    {"is_ctrl", T_BOOL, offsetof(NcInputObject, is_ctrl), READONLY, "Is ctrl pressed"},
    {"seqnum", T_ULONGLONG, offsetof(NcInputObject, seqnum), READONLY, "Sequence number"},
    {NULL}};

static PyMethodDef NcInput_methods[] = {
    {NULL, NULL, 0, NULL},
};

static PyTypeObject NcInputType = {
    PyVarObject_HEAD_INIT(NULL, 0)
        .tp_name = "notcurses._notcurses._NcInput",
    .tp_doc = "Notcurses Input",
    .tp_basicsize = sizeof(NcInputObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
    .tp_members = NcInput_members,
    .tp_methods = NcInput_methods,
    //.tp_alloc = PyType_GenericAlloc,
};

// Functions

/* Prototype

static PyObject *
_ncdirect_init(PyObject *self, PyObject *args)
{
    NcDirectObject *ncdirect_ref = NULL;
    if (!PyArg_ParseTuple(args, "O!", &NcDirectType, &ncdirect_ref))
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to parse _ncdirect_init arguments");
        return NULL;
    }
    struct ncdirect *ncdirect_ptr = ncdirect_init(NULL, NULL, 0);
    if (ncdirect_ptr != NULL)
    {
        ncdirect_ref->ncdirect_ptr = ncdirect_ptr;
        Py_RETURN_NONE;
    }
    else
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to parse NcDirect_init arguments");
        return NULL;
    }
}

*/
// NcDirect
static PyObject *
_nc_direct_init(PyObject *self, PyObject *args)
{
    NcDirectObject *ncdirect_ref = NULL;
    if (!PyArg_ParseTuple(args, "O!", &NcDirectType, &ncdirect_ref))
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to parse _ncdirect_init arguments");
        return NULL;
    }
    struct ncdirect *ncdirect_ptr = ncdirect_init(NULL, NULL, 0);
    if (ncdirect_ptr != NULL)
    {
        ncdirect_ref->ncdirect_ptr = ncdirect_ptr;
        Py_RETURN_NONE;
    }
    else
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to acquire NcDirectObject");
        return NULL;
    }
}

static PyObject *
_nc_direct_stop(PyObject *self, PyObject *args)
{
    NcDirectObject *ncdirect_ref = NULL;
    if (!PyArg_ParseTuple(args, "O!", &NcDirectType, &ncdirect_ref))
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to parse _ncdirect_init arguments");
        return NULL;
    }
    int return_code = ncdirect_stop(ncdirect_ref->ncdirect_ptr);
    if (return_code == 0)
    {
        Py_RETURN_NONE;
    }
    else
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to stop NcDirectObject");
        return NULL;
    }
}

static PyObject *
_nc_direct_putstr(PyObject *self, PyObject *args)
{
    NcDirectObject *ncdirect_ref = NULL;
    const char *string = NULL;
    const NcChannelsObject *channels_object = NULL;
    if (!PyArg_ParseTuple(args, "O!s|O",
                          &NcDirectType, &ncdirect_ref,
                          &string,
                          &channels_object))
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to parse _ncdirect_putstr arguments");
        return NULL;
    }
    uint64_t channels = 0;
    if (PyObject_IsInstance((PyObject *)channels_object, (PyObject *)&NcChannelsType))
    {
        channels = channels_object->ncchannels_ptr;
    }
    else if ((PyObject *)channels_object == (PyObject *)Py_None)
    {
        channels = 0;
    }
    else
    {
        PyErr_SetString(PyExc_RuntimeError, "Unknown _NcChannels type");
        return NULL;
    }

    int return_code = ncdirect_putstr(ncdirect_ref->ncdirect_ptr, channels, string);
    if (return_code >= 0)
    {
        return PyLong_FromLong(return_code);
    }
    else
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed put string on NcDirect");
        return NULL;
    }
}

static PyObject *
_nc_direct_get_dim_x(PyObject *self, PyObject *args)
{
    NcDirectObject *ncdirect_ref = NULL;
    if (!PyArg_ParseTuple(args, "O!", &NcDirectType, &ncdirect_ref))
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to parse _ncdirect_get_dim_x arguments");
        return NULL;
    }
    if (ncdirect_ref != NULL)
    {
        return PyLong_FromLong(ncdirect_dim_x(ncdirect_ref->ncdirect_ptr));
    }
    else
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to acquire NcDirectObject");
        return NULL;
    }
}

static PyObject *
_nc_direct_get_dim_y(PyObject *self, PyObject *args)
{
    NcDirectObject *ncdirect_ref = NULL;
    if (!PyArg_ParseTuple(args, "O!", &NcDirectType, &ncdirect_ref))
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to parse _ncdirect_get_dim_y arguments");
        return NULL;
    }
    if (ncdirect_ref != NULL)
    {
        return PyLong_FromLong(ncdirect_dim_y(ncdirect_ref->ncdirect_ptr));
    }
    else
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to acquire NcDirectObject");
        return NULL;
    }
}

static PyObject *
_nc_direct_disable_cursor(PyObject *self, PyObject *args)
{
    NcDirectObject *ncdirect_ref = NULL;
    if (!PyArg_ParseTuple(args, "O!", &NcDirectType, &ncdirect_ref))
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to parse _ncdirect_disable_cursor arguments");
        return NULL;
    }
    if (ncdirect_ref != NULL)
    {
        ncdirect_cursor_disable(ncdirect_ref->ncdirect_ptr);
        Py_RETURN_NONE;
    }
    else
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to acquire NcDirectObject");
        return NULL;
    }
}

static PyObject *
_nc_direct_enable_cursor(PyObject *self, PyObject *args)
{
    NcDirectObject *ncdirect_ref = NULL;
    if (!PyArg_ParseTuple(args, "O!", &NcDirectType, &ncdirect_ref))
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to parse _ncdirect_enable_cursor arguments");
        return NULL;
    }
    if (ncdirect_ref != NULL)
    {
        ncdirect_cursor_enable(ncdirect_ref->ncdirect_ptr);
        Py_RETURN_NONE;
    }
    else
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to acquire NcDirectObject");
        return NULL;
    }
}
// NcChannels

static PyObject *
_nc_channels_set_background_rgb(PyObject *self, PyObject *args)
{
    NcChannelsObject *nchannels_ref = NULL;
    int red = 0;
    int green = 0;
    int blue = 0;
    if (!PyArg_ParseTuple(args, "O!iii",
                          &NcChannelsType, &nchannels_ref,
                          &red, &green, &blue))
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to parse _ncchannels_set_background_rgb arguments");
        return NULL;
    }

    if (nchannels_ref == NULL)
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to acquire NcChannelsObject");
        return NULL;
    }

    int return_code = channels_set_bg_rgb8(&(nchannels_ref->ncchannels_ptr), red, green, blue);
    if (return_code != 0)
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to set channel background colors");
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
_nc_channels_set_foreground_rgb(PyObject *self, PyObject *args)
{
    NcChannelsObject *nchannels_ref = NULL;
    int red = 0;
    int green = 0;
    int blue = 0;
    if (!PyArg_ParseTuple(args, "O!iii",
                          &NcChannelsType, &nchannels_ref,
                          &red, &green, &blue))
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to parse _ncchannels_set_foreground_rgb arguments");
        return NULL;
    }

    if (nchannels_ref == NULL)
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to acquire NcChannelsObject");
        return NULL;
    }

    int return_code = channels_set_fg_rgb8(&(nchannels_ref->ncchannels_ptr), red, green, blue);
    if (return_code != 0)
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to set channel foreground colors");
        return NULL;
    }
    Py_RETURN_NONE;
}

// NotcursesContext

static PyObject *
_notcurses_context_init(PyObject *self, PyObject *args)
{
    NotcursesContextObject *notcurses_context_ref = NULL;
    if (!PyArg_ParseTuple(args, "O!", &NotcursesContextType, &notcurses_context_ref))
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to parse _notcurses_context_init arguments");
        return NULL;
    }
    struct notcurses *notcurses_context_ptr = notcurses_init(NULL, NULL);
    if (notcurses_context_ptr != NULL)
    {
        notcurses_context_ref->notcurses_context_ptr = notcurses_context_ptr;
        Py_RETURN_NONE;
    }
    else
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed initialize Notcurses");
        return NULL;
    }
}

static PyObject *
_notcurses_context_stop(PyObject *self, PyObject *args)
{
    NotcursesContextObject *notcurses_context_ref = NULL;
    if (!PyArg_ParseTuple(args, "O!", &NotcursesContextType, &notcurses_context_ref))
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to parse _notcurses_context_stop arguments");
        return NULL;
    }
    int return_code = notcurses_stop(notcurses_context_ref->notcurses_context_ptr);
    if (return_code == 0)
    {
        Py_RETURN_NONE;
    }
    else
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to stop notcurses context");
        return NULL;
    }
}

static PyObject *
_notcurses_context_render(PyObject *self, PyObject *args)
{
    NotcursesContextObject *notcurses_context_ref = NULL;
    if (!PyArg_ParseTuple(args, "O!", &NotcursesContextType, &notcurses_context_ref))
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to parse _notcurses_context_render arguments");
        return NULL;
    }
    int return_code = notcurses_render(notcurses_context_ref->notcurses_context_ptr);
    if (return_code == 0)
    {
        Py_RETURN_NONE;
    }
    else
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to render");
        return NULL;
    }
}

static PyObject *
_notcurses_context_mouse_disable(PyObject *self, PyObject *args)
{
    NotcursesContextObject *notcurses_context_ref = NULL;
    if (!PyArg_ParseTuple(args, "O!", &NotcursesContextType, &notcurses_context_ref))
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to parse _notcurses_context_mouse_disable arguments");
        return NULL;
    }
    int return_code = notcurses_mouse_disable(notcurses_context_ref->notcurses_context_ptr);
    if (return_code == 0)
    {
        Py_RETURN_NONE;
    }
    else
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to disable mouse");
        return NULL;
    }
}

static PyObject *
_notcurses_context_mouse_enable(PyObject *self, PyObject *args)
{
    NotcursesContextObject *notcurses_context_ref = NULL;
    if (!PyArg_ParseTuple(args, "O!", &NotcursesContextType, &notcurses_context_ref))
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to parse _notcurses_context_mouse_enable arguments");
        return NULL;
    }
    int return_code = notcurses_mouse_enable(notcurses_context_ref->notcurses_context_ptr);
    if (return_code == 0)
    {
        Py_RETURN_NONE;
    }
    else
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to enable mouse");
        return NULL;
    }
}

static PyObject *
_notcurses_context_cursor_disable(PyObject *self, PyObject *args)
{
    NotcursesContextObject *notcurses_context_ref = NULL;
    if (!PyArg_ParseTuple(args, "O!", &NotcursesContextType, &notcurses_context_ref))
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to parse _notcurses_context_cursor_disable arguments");
        return NULL;
    }
    int return_code = notcurses_cursor_disable(notcurses_context_ref->notcurses_context_ptr);
    if (return_code == 0)
    {
        Py_RETURN_NONE;
    }
    else
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to disable cursor");
        return NULL;
    }
}

static PyObject *
_notcurses_context_cursor_enable(PyObject *self, PyObject *args)
{
    NotcursesContextObject *notcurses_context_ref = NULL;
    int y = 0;
    int x = 0;
    if (!PyArg_ParseTuple(args, "O!|ii", &NotcursesContextType, &notcurses_context_ref,
                          &y, &x))
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to parse _notcurses_context_cursor_enable arguments");
        return NULL;
    }
    int return_code = notcurses_cursor_enable(notcurses_context_ref->notcurses_context_ptr, y, x);
    if (return_code == 0)
    {
        Py_RETURN_NONE;
    }
    else
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to enable cursor");
        return NULL;
    }
}

static NcPlaneObject *
_notcurses_context_get_std_plane(PyObject *self, PyObject *args)
{
    NotcursesContextObject *notcurses_context_ref = NULL;
    if (!PyArg_ParseTuple(args, "O!", &NotcursesContextType, &notcurses_context_ref))
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to parse _notcurses_context_cursor_enable arguments");
        return NULL;
    }
    struct ncplane *std_plane = notcurses_stdplane(notcurses_context_ref->notcurses_context_ptr);
    NcPlaneObject *ncplane_ref = PyObject_NEW(NcPlaneObject, &NcPlaneType);
    //PyObject_INIT(&ncplane_ref, &NcPlaneType);

    if (ncplane_ref != NULL && std_plane != NULL)
    {
        ncplane_ref->ncplane_ptr = std_plane;
        return ncplane_ref;
    }
    else
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to get std plane");
        return NULL;
    }
}

static NcInputObject *
_notcurses_context_get_input_blocking(PyObject *self, PyObject *args)
{
    NotcursesContextObject *notcurses_context_ref = NULL;
    if (!PyArg_ParseTuple(args, "O!", &NotcursesContextType, &notcurses_context_ref))
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to parse _notcurses_context_get_input_blocking arguments");
        return NULL;
    }
    struct ncinput nc_input_ptr = {};
    char32_t code_point = notcurses_getc_blocking(notcurses_context_ref->notcurses_context_ptr, &nc_input_ptr);
    NcInputObject *nc_input_ref = PyObject_NEW(NcInputObject, &NcInputType);
    PyObject_INIT(nc_input_ref, &NcInputType);
    if (code_point != -1)
    {

        nc_input_ref->codepoint = (long)nc_input_ptr.id;
        nc_input_ref->y_pos = nc_input_ptr.y;
        nc_input_ref->x_pos = nc_input_ptr.x;
        nc_input_ref->is_alt = nc_input_ptr.alt;
        nc_input_ref->is_shift = nc_input_ptr.shift;
        nc_input_ref->is_ctrl = nc_input_ptr.ctrl;
        nc_input_ref->seqnum = nc_input_ptr.seqnum;

        return nc_input_ref;
    }
    else
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to get input");
        return NULL;
    }
}

// NcPlane

static PyObject *
_nc_plane_set_background_rgb(PyObject *self, PyObject *args)
{
    NcPlaneObject *nc_plane_ref = NULL;
    int red = 0;
    int green = 0;
    int blue = 0;
    if (!PyArg_ParseTuple(args, "O!iii",
                          &NcPlaneType, &nc_plane_ref,
                          &red, &green, &blue))
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to parse _nc_plane_set_background_rgb arguments");
        return NULL;
    }

    if (nc_plane_ref == NULL)
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to acquire NcPlaneObject");
        return NULL;
    }

    int return_code = ncplane_set_bg_rgb8(nc_plane_ref->ncplane_ptr, red, green, blue);
    if (return_code != 0)
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to set plane background colors");
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
_nc_plane_set_foreground_rgb(PyObject *self, PyObject *args)
{
    NcPlaneObject *nc_plane_ref = NULL;
    int red = 0;
    int green = 0;
    int blue = 0;
    if (!PyArg_ParseTuple(args, "O!iii",
                          &NcPlaneType, &nc_plane_ref,
                          &red, &green, &blue))
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to parse _nc_plane_set_foreground_rgb arguments");
        return NULL;
    }

    if (nc_plane_ref == NULL)
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to acquire NcPlaneObject");
        return NULL;
    }

    int return_code = ncplane_set_fg_rgb8(nc_plane_ref->ncplane_ptr, red, green, blue);
    if (return_code != 0)
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to set plane foreground colors");
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
_nc_plane_putstr(PyObject *self, PyObject *args)
{
    NcPlaneObject *nc_plane_ref = NULL;
    int y_pos = -1;
    int x_pos = -1;
    const char *string = NULL;
    if (!PyArg_ParseTuple(args, "O!sii",
                          &NcPlaneType, &nc_plane_ref,
                          &string,
                          &y_pos, &x_pos))
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to parse _nc_plane_putstr arguments");
        return NULL;
    }

    if (nc_plane_ref == NULL)
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to acquire NcPlaneObject");
        return NULL;
    }

    int return_code = ncplane_putstr_yx(nc_plane_ref->ncplane_ptr, y_pos, x_pos, string);
    return PyLong_FromLong(return_code);
}

static PyObject *
_nc_plane_putstr_aligned(PyObject *self, PyObject *args)
{
    NcPlaneObject *nc_plane_ref = NULL;
    int y_pos = -1;
    ncalign_e align = NCALIGN_UNALIGNED;
    const char *string = NULL;
    if (!PyArg_ParseTuple(args, "O!sii",
                          &NcPlaneType, &nc_plane_ref,
                          &string,
                          &y_pos, &align))
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to parse _nc_plane_putstr_aligned arguments");
        return NULL;
    }

    int return_code = ncplane_putstr_aligned(nc_plane_ref->ncplane_ptr, y_pos, align, string);
    return PyLong_FromLong(return_code);
}

static PyObject *
_nc_plane_dimensions_yx(PyObject *self, PyObject *args)
{
    NcPlaneObject *nc_plane_ref = NULL;
    int y_dim = 0;
    int x_dim = 0;
    if (!PyArg_ParseTuple(args, "O!",
                          &NcPlaneType, &nc_plane_ref))
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to parse _nc_plane_set_foreground_rgb arguments");
        return NULL;
    }

    if (nc_plane_ref == NULL)
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to acquire NcPlaneObject");
        return NULL;
    }

    ncplane_dim_yx(nc_plane_ref->ncplane_ptr, &y_dim, &x_dim);
    return PyTuple_Pack(2, PyLong_FromLong(y_dim), PyLong_FromLong(x_dim));
}

static PyObject *
_nc_plane_polyfill_yx(PyObject *self, PyObject *args)
{
    NcPlaneObject *nc_plane_ref = NULL;
    int y_dim = -1;
    int x_dim = -1;
    const char *cell_str = NULL;
    if (!PyArg_ParseTuple(args, "O!iis",
                          &NcPlaneType, &nc_plane_ref,
                          &y_dim, &x_dim,
                          &cell_str))
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to parse _nc_plane_polyfill_yx arguments");
        return NULL;
    }
    cell cell_to_fill_with = CELL_CHAR_INITIALIZER(*cell_str);
    int return_code = ncplane_polyfill_yx(nc_plane_ref->ncplane_ptr, y_dim, x_dim, &cell_to_fill_with);
    if (return_code != -1)
    {
        return PyLong_FromLong((long)return_code);
    }
    else
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to polyfill");
        return NULL;
    }
}

static PyObject *
_nc_plane_erase(PyObject *self, PyObject *args)
{
    NcPlaneObject *nc_plane_ref = NULL;
    if (!PyArg_ParseTuple(args, "O!", &NcPlaneType, &nc_plane_ref))
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to parse _nc_plane_erase arguments");
        return NULL;
    }
    ncplane_erase(nc_plane_ref->ncplane_ptr);
    Py_RETURN_NONE;
}

static NcPlaneObject *
_nc_plane_create(PyObject *self, PyObject *args)
{
    NcPlaneObject *nc_plane_parent = NULL;
    int y_pos, x_pos, rows_num, cols_num;
    if (!PyArg_ParseTuple(args, "O!iiii",
                          &NcPlaneType, &nc_plane_parent,
                          &y_pos, &x_pos,
                          &rows_num, &cols_num))
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to parse _nc_plane_create arguments");
        return NULL;
    }
    ncplane_options create_options = {
        .y = y_pos,
        .horiz.x = x_pos,
        .rows = rows_num,
        .cols = cols_num,
    };
    struct ncplane *new_nc_plane = ncplane_create(nc_plane_parent->ncplane_ptr, &create_options);
    if (new_nc_plane != NULL)
    {
        NcPlaneObject *ncplane_ref = PyObject_NEW(NcPlaneObject, &NcPlaneType);

        ncplane_ref->ncplane_ptr = new_nc_plane;
        return ncplane_ref;
    }
    else
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to create NcPlane");
        return NULL;
    }
}

static PyObject *
get_notcurses_version_str(PyObject *self, PyObject *args)
{
    const char *verstion_str = notcurses_version();
    return PyUnicode_FromString(verstion_str);
}

// Copy pasta
// {"_nc_direct_init", (PyCFunction)_ncdirect_init, METH_VARARGS, NULL},
static PyMethodDef NotcursesMethods[] = {
    {"_nc_direct_init", (PyCFunction)_nc_direct_init, METH_VARARGS, NULL},
    {"_nc_direct_stop", (PyCFunction)_nc_direct_stop, METH_VARARGS, NULL},
    {"_nc_direct_putstr", (PyCFunction)_nc_direct_putstr, METH_VARARGS, NULL},
    {"_nc_direct_get_dim_x", (PyCFunction)_nc_direct_get_dim_x, METH_VARARGS, NULL},
    {"_nc_direct_get_dim_y", (PyCFunction)_nc_direct_get_dim_y, METH_VARARGS, NULL},
    {"_nc_direct_disable_cursor", (PyCFunction)_nc_direct_disable_cursor, METH_VARARGS, NULL},
    {"_nc_direct_enable_cursor", (PyCFunction)_nc_direct_enable_cursor, METH_VARARGS, NULL},
    {"_nc_channels_set_background_rgb", (PyCFunction)_nc_channels_set_background_rgb, METH_VARARGS, NULL},
    {"_nc_channels_set_foreground_rgb", (PyCFunction)_nc_channels_set_foreground_rgb, METH_VARARGS, NULL},
    {"_notcurses_context_init", (PyCFunction)_notcurses_context_init, METH_VARARGS, NULL},
    {"_notcurses_context_stop", (PyCFunction)_notcurses_context_stop, METH_VARARGS, NULL},
    {"_notcurses_context_render", (PyCFunction)_notcurses_context_render, METH_VARARGS, NULL},
    {"_notcurses_context_mouse_disable", (PyCFunction)_notcurses_context_mouse_disable, METH_VARARGS, NULL},
    {"_notcurses_context_mouse_enable", (PyCFunction)_notcurses_context_mouse_enable, METH_VARARGS, NULL},
    {"_notcurses_context_cursor_disable", (PyCFunction)_notcurses_context_cursor_disable, METH_VARARGS, NULL},
    {"_notcurses_context_cursor_enable", (PyCFunction)_notcurses_context_cursor_enable, METH_VARARGS, NULL},
    {"_notcurses_context_get_std_plane", (PyCFunction)_notcurses_context_get_std_plane, METH_VARARGS, NULL},
    {"_nc_plane_set_background_rgb", (PyCFunction)_nc_plane_set_background_rgb, METH_VARARGS, NULL},
    {"_nc_plane_set_foreground_rgb", (PyCFunction)_nc_plane_set_foreground_rgb, METH_VARARGS, NULL},
    {"_nc_plane_putstr", (PyCFunction)_nc_plane_putstr, METH_VARARGS, NULL},
    {"_nc_plane_putstr_aligned", (PyCFunction)_nc_plane_putstr_aligned, METH_VARARGS, NULL},
    {"_nc_plane_dimensions_yx", (PyCFunction)_nc_plane_dimensions_yx, METH_VARARGS, NULL},
    {"_nc_plane_polyfill_yx", (PyCFunction)_nc_plane_polyfill_yx, METH_VARARGS, NULL},
    {"_nc_plane_erase", (PyCFunction)_nc_plane_erase, METH_VARARGS, NULL},
    {"_nc_plane_create", (PyCFunction)_nc_plane_create, METH_VARARGS, NULL},
    {"_notcurses_context_get_input_blocking", (PyCFunction)_notcurses_context_get_input_blocking, METH_VARARGS, NULL},
    {"get_notcurses_version", (PyCFunction)get_notcurses_version_str, METH_NOARGS, NULL},
    {NULL, NULL, 0, NULL},
};

static struct PyModuleDef NotcursesModule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "Notcurses", /* name of module */
    .m_doc = "Notcurses.", /* module documentation, may be NULL */
    .m_size = -1,          /* size of per-interpreter state of the module,
                 or -1 if the module keeps state in global variables. */
    NotcursesMethods,
};

PyMODINIT_FUNC
PyInit__notcurses(void)
{
    PyObject *py_module; // create the module
    if (PyType_Ready(&NotcursesContextType) < 0)
        return NULL;

    if (PyType_Ready(&NcPlaneType) < 0)
        return NULL;

    if (PyType_Ready(&NcDirectType) < 0)
        return NULL;

    if (PyType_Ready(&NcChannelsType) < 0)
        return NULL;

    if (PyType_Ready(&NcInputType) < 0)
        return NULL;

    py_module = PyModule_Create(&NotcursesModule);
    if (py_module == NULL)
        return NULL;

    Py_INCREF(&NotcursesContextType);
    if (PyModule_AddObject(py_module, "_NotcursesContext", (PyObject *)&NotcursesContextType) < 0)
    {
        Py_DECREF(&NotcursesContextType);
        Py_DECREF(py_module);
        return NULL;
    }

    Py_INCREF(&NcPlaneType);
    if (PyModule_AddObject(py_module, "_NcPlane", (PyObject *)&NcPlaneType) < 0)
    {
        Py_DECREF(&NcPlaneType);
        Py_DECREF(py_module);
        return NULL;
    }

    Py_INCREF(&NcDirectType);
    if (PyModule_AddObject(py_module, "_NcDirect", (PyObject *)&NcDirectType) < 0)
    {
        Py_DECREF(&NcDirectType);
        Py_DECREF(py_module);
        return NULL;
    }

    Py_INCREF(&NcChannelsType);
    if (PyModule_AddObject(py_module, "_NcChannels", (PyObject *)&NcChannelsType) < 0)
    {
        Py_DECREF(&NcChannelsType);
        Py_DECREF(py_module);
        return NULL;
    }

    Py_INCREF(&NcInputType);
    if (PyModule_AddObject(py_module, "_NcInput", (PyObject *)&NcInputType) < 0)
    {
        Py_DECREF(&NcInputType);
        Py_DECREF(py_module);
        return NULL;
    }

    // Constants PyModule_AddIntMacro(py_module, );
    int constants_control_value = 0;
    // Inputs
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_INVALID);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_RESIZE);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_UP);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_RIGHT);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_DOWN);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_LEFT);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_INS);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_DEL);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_BACKSPACE);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_PGDOWN);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_PGUP);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_HOME);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_END);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_F00);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_F01);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_F02);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_F03);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_F04);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_F05);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_F06);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_F07);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_F08);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_F09);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_F10);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_F11);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_F12);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_ENTER);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_CLS);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_DLEFT);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_DRIGHT);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_ULEFT);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_URIGHT);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_CENTER);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_BEGIN);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_CANCEL);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_CLOSE);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_COMMAND);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_COPY);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_EXIT);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_PRINT);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_REFRESH);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_BUTTON1);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_BUTTON2);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_BUTTON3);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_SCROLL_UP);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_SCROLL_DOWN);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_BUTTON6);
    constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_RELEASE);
    // Nc Align
    constants_control_value |= PyModule_AddIntConstant(py_module, "NCALIGN_UNALIGNED", NCALIGN_UNALIGNED);
    constants_control_value |= PyModule_AddIntConstant(py_module, "NCALIGN_LEFT", NCALIGN_LEFT);
    constants_control_value |= PyModule_AddIntConstant(py_module, "NCALIGN_CENTER", NCALIGN_CENTER);
    constants_control_value |= PyModule_AddIntConstant(py_module, "NCALIGN_RIGHT", NCALIGN_RIGHT);
    // Scale
    constants_control_value |= PyModule_AddIntConstant(py_module, "NCSCALE_NONE", NCSCALE_NONE);
    constants_control_value |= PyModule_AddIntConstant(py_module, "NCSCALE_SCALE", NCSCALE_SCALE);
    constants_control_value |= PyModule_AddIntConstant(py_module, "NCSCALE_STRETCH", NCSCALE_STRETCH);
    if (constants_control_value < 0)
    {
        Py_DECREF(py_module);
        return NULL;
    }

    return py_module;
}
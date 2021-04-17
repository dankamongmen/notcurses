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

static void
NcPlane_dealloc(NcPlaneObject *self)
{
    if (NULL != self->ncplane_ptr && !self->is_stdplane)
    {
        ncplane_destroy(self->ncplane_ptr);
    }

    Py_TYPE(self)->tp_free(self);
}

static PyObject *
NcPlane_new(PyTypeObject *Py_UNUSED(subtype), PyObject *Py_UNUSED(args), PyObject *Py_UNUSED(kwds))
{
    PyErr_SetString(PyExc_ValueError, "NcPlane should NOT be initialied directly.");
    return NULL;
}

static PyObject *
Ncplane_create(NcPlaneObject *self, PyObject *args, PyObject *kwds)
{
    int y = 0, x = 0;
    int rows = 0, cols = 0;
    const char *name = NULL;
    // TODO reseize callback
    unsigned long long flags = 0;
    int margin_b = 0, margin_r = 0;

    char *keywords[] = {"y_pos", "x_pos",
                        "rows", "cols",
                        "name",
                        "flags",
                        "margin_b", "margin_r", NULL};

    GNU_PY_CHECK_INT(PyArg_ParseTupleAndKeywords(args, kwds, "|ii ii s K ii", keywords,
                                                 &y, &x,
                                                 &rows, &cols,
                                                 &name,
                                                 &flags,
                                                 &margin_b, &margin_r));

    ncplane_options options = {
        .y = y,
        .x = x,
        .rows = rows,
        .cols = cols,
        .name = name,
        .flags = (uint64_t)flags,
        .margin_b = margin_b,
        .margin_r = margin_r,
    };

    PyObject *new_object CLEANUP_PY_OBJ = NcPlane_Type.tp_alloc((PyTypeObject *)&NcPlane_Type, 0);
    NcPlaneObject *new_plane = (NcPlaneObject *)new_object;
    new_plane->ncplane_ptr = CHECK_NOTCURSES_PTR(ncplane_create(self->ncplane_ptr, &options));
    new_plane->is_stdplane = false;

    Py_INCREF(new_object);
    return new_object;
}

static PyMethodDef NcPlane_methods[] = {
    {"create", (void *)Ncplane_create, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Create a new ncplane bound to plane 'n', at the offset 'y'x'x' (relative to the origin of 'n') and the specified size. The number of 'rows' and 'cols' must both be positive. This plane is initially at the top of the z-buffer, as if ncplane_move_top() had been called on it. The void* 'userptr' can be retrieved (and reset) later. A 'name' can be set, used in debugging.")},
    {NULL, NULL, 0, NULL},
};

PyTypeObject NcPlane_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
        .tp_name = "notcurses.NcPlane",
    .tp_doc = "Notcurses Plane",
    .tp_basicsize = sizeof(NcPlaneObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = NcPlane_new,
    .tp_dealloc = (destructor)NcPlane_dealloc,
    .tp_methods = NcPlane_methods,
};
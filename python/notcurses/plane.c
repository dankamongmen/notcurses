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

static PyMethodDef NcPlane_methods[] = {

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
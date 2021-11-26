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
python_notcurses_version(PyObject *Py_UNUSED(self), PyObject *Py_UNUSED(args))
{
    const char *verstion_str = notcurses_version();
    return PyUnicode_FromString(verstion_str);
}

static PyObject *
python_notcurses_version_components(PyObject *Py_UNUSED(self), PyObject *Py_UNUSED(args))
{
    int major, minor, patch, tweak = {0};

    notcurses_version_components(&major, &minor, &patch, &tweak);

    return Py_BuildValue("iiii", major, minor, patch, tweak);
}

static PyObject *
python_ncstrwidth(PyObject *Py_UNUSED(self), PyObject *args)
{
    const char *s = NULL;

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "s", &s, NULL, NULL));

    return Py_BuildValue("i", ncstrwidth(s, NULL, NULL));
}

PyMethodDef MiscFunctions[] = {
    {"notcurses_version", (PyCFunction)python_notcurses_version, METH_NOARGS, "Get a human-readable string describing the running Notcurses version."},
    {"notcurses_version_components", (PyCFunction)python_notcurses_version_components, METH_NOARGS, "Get a tuple of major, minor, patch, tweak integer of the running Notcurses version."},
    {"ncstrwidth", (PyCFunction)python_ncstrwidth, METH_VARARGS, "Returns the number of columns occupied by a string, or -1 if a non-printable/illegal character is encountered."},
    {NULL, NULL, 0, NULL},
};

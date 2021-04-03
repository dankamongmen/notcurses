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

static PyMethodDef NotcursesMiscMethods[] = {
    {"notcurses_version", (PyCFunction)python_notcurses_version, METH_NOARGS, "Get a human-readable string describing the running Notcurses version."},
    {NULL, NULL, 0, NULL},
};

static struct PyModuleDef NotcursesMiscModule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "NotcursesMisc",
    .m_doc = "Notcurses miscellaneous functions",
    .m_size = -1,
    .m_methods = NotcursesMiscMethods,
};

PyMODINIT_FUNC
PyInit_misc(void)
{
    PyObject *py_module = NULL;
    py_module = PyModule_Create(&NotcursesMiscModule);

    return py_module;
}
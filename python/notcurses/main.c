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

#include <Python.h>

#include "notcurses-python.h"

PyObject *traceback_format_exception = NULL;
PyObject *new_line_unicode = NULL;

static void
Notcurses_module_free(PyObject *Py_UNUSED(self))
{
    Py_XDECREF(traceback_format_exception);
    Py_XDECREF(new_line_unicode);
}

extern PyMethodDef pync_methods[];

static struct PyModuleDef NotcursesMiscModule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "Notcurses",
    .m_doc = "Notcurses python module",
    .m_size = -1,
    .m_methods = pync_methods,
    .m_free = (freefunc)Notcurses_module_free,
};

static PyStructSequence_Field NcInput_fields[] = {
    {"id", "Unicode codepoint or synthesized NCKEY event"},
    {"y", "y cell coordinate of event, -1 for undefined"},
    {"x", "x cell coordinate of event, -1 for undefined"},
    {"utf8", "utf8 representation, if one exists"},
    // Note: alt, shift, ctrl fields deprecated in C API are omitted.
    {"evtype", NULL},
    {"modifiers", "bitmask over NCKEY_MOD_*"},
    {"ypx", "y pixel offset within cell, -1 for undefined"},
    {"xpx", "x pixel offset within cell, -1 for undefined"},
    {NULL, NULL},
};

static struct PyStructSequence_Desc NcInput_desc = {
    .name = "NcInput",
    .doc = "Notcurses input event",
    .fields = NcInput_fields,
    .n_in_sequence = 8,
};

PyTypeObject *NcInput_Type;

PyMODINIT_FUNC
PyInit_notcurses(void)
{
    PyObject *py_module CLEANUP_PY_OBJ = NULL;
    py_module = PyModule_Create(&NotcursesMiscModule);

    GNU_PY_CHECK_INT(PyModule_AddFunctions(py_module, ChannelsFunctions));
    GNU_PY_CHECK_INT(PyModule_AddFunctions(py_module, MiscFunctions));

    // Type ready?
    GNU_PY_TYPE_READY(&Notcurses_Type);
    GNU_PY_TYPE_READY(&NcPlane_Type);

    // Add objects
    GNU_PY_MODULE_ADD_OBJECT(py_module, (PyObject *)&Notcurses_Type, "Notcurses");
    GNU_PY_MODULE_ADD_OBJECT(py_module, (PyObject *)&NcPlane_Type, "NcPlane");

    NcInput_Type = PyStructSequence_NewType(&NcInput_desc);
    if (NcInput_Type == NULL)
        return NULL;
    GNU_PY_MODULE_ADD_OBJECT(py_module, (PyObject *)NcInput_Type, "NcInput");

    // background cannot be highcontrast, only foreground
    GNU_PY_CHECK_INT(PyModule_AddIntMacro(py_module, NCALPHA_HIGHCONTRAST));
    GNU_PY_CHECK_INT(PyModule_AddIntMacro(py_module, NCALPHA_TRANSPARENT));
    GNU_PY_CHECK_INT(PyModule_AddIntMacro(py_module, NCALPHA_BLEND));
    GNU_PY_CHECK_INT(PyModule_AddIntMacro(py_module, NCALPHA_OPAQUE));

    // FIXME: Better, attributes of an object such as an enum.
    GNU_PY_CHECK_INT(PyModule_AddStringMacro(py_module, NCBOXASCII));
    GNU_PY_CHECK_INT(PyModule_AddStringMacro(py_module, NCBOXDOUBLE));
    GNU_PY_CHECK_INT(PyModule_AddStringMacro(py_module, NCBOXHEAVY));
    GNU_PY_CHECK_INT(PyModule_AddStringMacro(py_module, NCBOXLIGHT));
    GNU_PY_CHECK_INT(PyModule_AddStringMacro(py_module, NCBOXOUTER));
    GNU_PY_CHECK_INT(PyModule_AddStringMacro(py_module, NCBOXROUND));

    // if this bit is set, we are *not* using the default background color
    GNU_PY_CHECK_INT(PyModule_AddIntMacro(py_module, NC_BGDEFAULT_MASK));
    // extract these bits to get the background RGB value
    GNU_PY_CHECK_INT(PyModule_AddIntMacro(py_module, NC_BG_RGB_MASK));
    // if this bit *and* NC_BGDEFAULT_MASK are set, we're using a
    // palette-indexed background color
    GNU_PY_CHECK_INT(PyModule_AddIntMacro(py_module, NC_BG_PALETTE));
    GNU_PY_CHECK_INT(PyModule_AddIntMacro(py_module, NCPALETTESIZE));
    // extract these bits to get the background alpha mask
    GNU_PY_CHECK_INT(PyModule_AddIntMacro(py_module, NC_BG_ALPHA_MASK));

    PyObject *traceback_module CLEANUP_PY_OBJ = GNU_PY_CHECK(PyImport_ImportModule("traceback"));
    traceback_format_exception = GNU_PY_CHECK(PyObject_GetAttrString(traceback_module, "format_exception"));
    new_line_unicode = GNU_PY_CHECK(PyUnicode_FromString("\n"));

    Py_INCREF(py_module);
    return py_module;
}

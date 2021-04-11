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

static struct PyModuleDef NotcursesMiscModule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "Notcurses",
    .m_doc = "Notcurses python module",
    .m_size = -1,
    .m_methods = NULL,
};

PyMODINIT_FUNC
PyInit_notcurses(void)
{
    PyObject *py_module CLEANUP_PY_OBJ = NULL;
    py_module = PyModule_Create(&NotcursesMiscModule);

    GNU_PY_CHECK_INT(PyModule_AddFunctions(py_module, ChannelsFunctions));
    GNU_PY_CHECK_INT(PyModule_AddFunctions(py_module, MiscFunctions));

    // Type ready?
    GNU_PY_TYPE_READY(&Notcurses_Type);

    // Add objects
    GNU_PY_MODULE_ADD_OBJECT(py_module, (PyObject *)&Notcurses_Type, "Notcurses");

    // background cannot be highcontrast, only foreground
    GNU_PY_CHECK_INT(PyModule_AddIntMacro(py_module, CELL_ALPHA_HIGHCONTRAST));
    GNU_PY_CHECK_INT(PyModule_AddIntMacro(py_module, CELL_ALPHA_TRANSPARENT));
    GNU_PY_CHECK_INT(PyModule_AddIntMacro(py_module, CELL_ALPHA_BLEND));
    GNU_PY_CHECK_INT(PyModule_AddIntMacro(py_module, CELL_ALPHA_OPAQUE));

    // if this bit is set, we are *not* using the default background color
    GNU_PY_CHECK_INT(PyModule_AddIntMacro(py_module, CELL_BGDEFAULT_MASK));
    // if this bit is set, we are *not* using the default foreground color
    GNU_PY_CHECK_INT(PyModule_AddIntMacro(py_module, CELL_FGDEFAULT_MASK));
    // extract these bits to get the background RGB value
    GNU_PY_CHECK_INT(PyModule_AddIntMacro(py_module, CELL_BG_RGB_MASK));
    // extract these bits to get the foreground RGB value
    GNU_PY_CHECK_INT(PyModule_AddIntMacro(py_module, CELL_FG_RGB_MASK));
    // if this bit *and* CELL_BGDEFAULT_MASK are set, we're using a
    // palette-indexed background color
    GNU_PY_CHECK_INT(PyModule_AddIntMacro(py_module, CELL_BG_PALETTE));
    GNU_PY_CHECK_INT(PyModule_AddIntMacro(py_module, NCPALETTESIZE));
    // if this bit *and* CELL_FGDEFAULT_MASK are set, we're using a
    // palette-indexed foreground color
    GNU_PY_CHECK_INT(PyModule_AddIntMacro(py_module, CELL_FG_PALETTE));
    // extract these bits to get the background alpha mask
    GNU_PY_CHECK_INT(PyModule_AddIntMacro(py_module, CELL_BG_ALPHA_MASK));
    // extract these bits to get the foreground alpha mask
    GNU_PY_CHECK_INT(PyModule_AddIntMacro(py_module, CELL_FG_ALPHA_MASK));

    Py_INCREF(py_module);
    return py_module;
}
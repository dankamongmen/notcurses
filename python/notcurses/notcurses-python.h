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

// Define classes

typedef struct
{
    PyObject_HEAD;
    struct notcurses *notcurses_ptr;
} NotcursesObject;

extern PyTypeObject Notcurses_Type;

typedef struct
{
    PyObject_HEAD;
    struct ncplane *ncplane_ptr;
} NcPlaneObject;

extern PyTypeObject NcPlane_Type;

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
    struct ncstats ncstats;
} NcStatsObject;

typedef struct
{
    PyObject_HEAD;
    struct ncpalette ncpalette;
} Palette256Object;

extern PyTypeObject *NcInput_Type;

// Imports

extern PyObject *traceback_format_exception;
extern PyObject *new_line_unicode;

// Functions

PyMODINIT_FUNC PyInit_notcurses(void);

// // Channels

extern PyMethodDef ChannelsFunctions[];

// // Misc

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

#define GNU_PY_CHECK_RET_NEG1(py_function)  \
    ({                                      \
        PyObject *new_object = py_function; \
        if (new_object == NULL)             \
        {                                   \
            return -1;                      \
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

#define GNU_PY_CHECK_BOOL(py_function)  \
    ({                                  \
        int return_value = py_function; \
        if (!return_value)              \
        {                               \
            return NULL;                \
        }                               \
        return_value;                   \
    })

#define GNU_PY_CHECK_INT_RET_NEG1(py_function) \
    ({                                         \
        int return_value = py_function;        \
        if (return_value < 0)                  \
        {                                      \
            return -1;                         \
        }                                      \
        return_value;                          \
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

#define CHECK_NOTCURSES_PTR(notcurses_func)                                  \
    ({                                                                       \
        void *return_ptr = notcurses_func;                                   \
        if (NULL == return_ptr)                                              \
        {                                                                    \
            PyErr_Format(PyExc_RuntimeError, "Notcurses returned null ptr"); \
            return NULL;                                                     \
        }                                                                    \
        return_ptr;                                                          \
    })

#define GNU_PY_UINT_CHECK(py_ulong)                          \
    ({                                                       \
        unsigned new_long = (unsigned)PyLong_AsUnsignedLong(py_ulong); \
        if (PyErr_Occurred())                                \
        {                                                    \
            return NULL;                                     \
        }                                                    \
        new_long;                                            \
    })

#define GNU_PY_LONG_CHECK(py_long)              \
    ({                                          \
        long new_long = PyLong_AsLong(py_long); \
        if (PyErr_Occurred())                   \
        {                                       \
            return NULL;                        \
        }                                       \
        new_long;                               \
    })

#define GNU_PY_MODULE_ADD_OBJECT(module, py_object, py_object_name)    \
    ({                                                                 \
        Py_INCREF(py_object);                                          \
        if (PyModule_AddObject(module, py_object_name, py_object) < 0) \
        {                                                              \
            Py_DECREF(py_object);                                      \
            return NULL;                                               \
        }                                                              \
    })

#define GNU_PY_TYPE_READY(py_type)     \
    {                                  \
        if (PyType_Ready(py_type) < 0) \
        {                              \
            return NULL;               \
        }                              \
    }

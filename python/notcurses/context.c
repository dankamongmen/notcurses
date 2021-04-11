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
Notcurses_dealloc(NotcursesObject *self)
{
    if (NULL != self->notcurses_ptr)
    {
        notcurses_stop(self->notcurses_ptr);
    }

    Py_TYPE(self)->tp_free(self);
}

static PyObject *
Notcurses_new(PyTypeObject *subtype, PyObject *args, PyObject *kwds)
{

    PyObject *main_fd_object = NULL;

    const char *term_type = NULL;
    PyObject *render_fd_object = NULL;
    int log_level = {0};
    const char *margins_str = NULL;
    PyObject *margin_top = NULL, *margin_right = NULL, *margin_bottom = NULL, *margin_left = NULL;
    unsigned long long flags = 0;

    char *keywords[] = {"tty_fd",
                        "term_type", "renderfd", "loglevel",
                        "margins_str",
                        "margin_t", "margin_r", "margin_b", "margin_l",
                        "flags", NULL};

    GNU_PY_CHECK_INT(PyArg_ParseTupleAndKeywords(args, kwds, "|O! sO!i s O!O!O!O! K", keywords,
                                                 &PyLong_Type, &main_fd_object,
                                                 &term_type, &PyLong_Type, &render_fd_object, &log_level,
                                                 &margins_str,
                                                 &PyLong_Type, &margin_top, &PyLong_Type, &margin_right, &PyLong_Type, &margin_bottom, &PyLong_Type, &margin_left,
                                                 &flags));

    notcurses_options options = {0};

    options.termtype = term_type;

    if (NULL != render_fd_object)
    {
        long render_fd = GNU_PY_LONG_CHECK(render_fd_object);

        FILE *renderfp = fdopen((int)render_fd, "w");
        if (NULL == renderfp)
        {
            PyErr_SetString(PyExc_ValueError, "Failed to open render file descriptor.");
            return NULL;
        }

        options.renderfp = renderfp;
    }

    options.loglevel = (ncloglevel_e)log_level;

    if (NULL != margins_str)
    {
        CHECK_NOTCURSES(notcurses_lex_margins(margins_str, &options));
    }

    if (NULL != margin_top)
    {
        options.margin_t = (int)GNU_PY_LONG_CHECK(margin_top);
    }

    if (NULL != margin_right)
    {
        options.margin_r = (int)GNU_PY_LONG_CHECK(margin_right);
    }

    if (NULL != margin_bottom)
    {
        options.margin_b = (int)GNU_PY_LONG_CHECK(margin_bottom);
    }

    if (NULL != margin_left)
    {
        options.margin_l = (int)GNU_PY_LONG_CHECK(margin_left);
    }

    options.flags = (uint64_t)flags;

    FILE *main_tty_fp = NULL;

    if (NULL != main_fd_object)
    {
        int main_fd = (int)GNU_PY_LONG_CHECK(main_fd_object);
        main_tty_fp = fdopen((int)main_fd, "w");
        if (NULL == main_tty_fp)
        {

            PyErr_SetString(PyExc_ValueError, "Failed to open render file descriptor.");
            return NULL;
        }
    }

    struct notcurses *new_notcurses = CHECK_NOTCURSES_PTR(notcurses_init(&options, main_tty_fp));

    NotcursesObject *new_context = (NotcursesObject *)GNU_PY_CHECK(subtype->tp_alloc(subtype, 0));

    new_context->notcurses_ptr = new_notcurses;

    return (PyObject *)new_context;
}

static PyObject *
Notcurses_drop_planes(NotcursesObject *self, PyObject *Py_UNUSED(args))
{
    notcurses_drop_planes(self->notcurses_ptr);
    Py_RETURN_NONE;
}

static PyMethodDef Notcurses_methods[] = {
    {"drop_planes", (PyCFunction)Notcurses_drop_planes, METH_NOARGS, "Destroy all ncplanes other than the stdplane."},
    {NULL, NULL, 0, NULL},
};

PyTypeObject Notcurses_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
        .tp_name = "notcurses.Notcurses",
    .tp_doc = "Notcurses Context",
    .tp_basicsize = sizeof(NotcursesObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = Notcurses_new,
    .tp_dealloc = (destructor)Notcurses_dealloc,
    .tp_methods = Notcurses_methods,
};
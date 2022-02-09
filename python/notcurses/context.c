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

static PyObject *uncaught_exception_unicode = NULL;

static int
Notcurses_uncaught_audit(const char *event, PyObject *args, void *Py_UNUSED(userData))
{
    if (0 != strcmp(event, "sys.excepthook"))
    {
        return 0;
    }

    Py_XDECREF(uncaught_exception_unicode);
    uncaught_exception_unicode = NULL;

    PyObject *uncaught_type = GNU_PY_CHECK_RET_NEG1(PyTuple_GetItem(args, 1));
    PyObject *uncaught_value = GNU_PY_CHECK_RET_NEG1(PyTuple_GetItem(args, 2));
    PyObject *uncaught_traceback = GNU_PY_CHECK_RET_NEG1(PyTuple_GetItem(args, 3));

    PyObject *uncaught_expection_format_list CLEANUP_PY_OBJ = GNU_PY_CHECK_RET_NEG1(PyObject_CallFunctionObjArgs(traceback_format_exception, uncaught_type, uncaught_value, uncaught_traceback, NULL));

    uncaught_exception_unicode = GNU_PY_CHECK_RET_NEG1(PyUnicode_Join(new_line_unicode, uncaught_expection_format_list));

    return 0;
}

static void
Notcurses_dealloc(NotcursesObject *self)
{
    if (NULL != self->notcurses_ptr)
    {
        notcurses_stop(self->notcurses_ptr);
    }

    Py_TYPE(self)->tp_free(self);

    if (NULL != uncaught_exception_unicode)
    {
        fprintf(stderr, "While Notcurses was running the following exception was uncaught:\n");
        PyObject_Print(uncaught_exception_unicode, stderr, 1);

        Py_XDECREF(uncaught_exception_unicode);
        uncaught_exception_unicode = NULL;
    }
};

static PyObject *
Notcurses_new(PyTypeObject *subtype, PyObject *args, PyObject *kwds)
{

    PyObject *main_fd_object = NULL;

    const char *term_type = NULL;
    int log_level = {0};
    const char *margins_str = NULL;
    PyObject *margin_top = NULL, *margin_right = NULL, *margin_bottom = NULL, *margin_left = NULL;
    unsigned long long flags = 0;

    char *keywords[] = {"tty_fd",
                        "term_type", "renderfd", "loglevel",
                        "margins_str",
                        "margin_t", "margin_r", "margin_b", "margin_l",
                        "flags", NULL};

    GNU_PY_CHECK_BOOL(PyArg_ParseTupleAndKeywords(args, kwds, "|O!sO!isO!O!O!O!K", keywords,
                                                  &PyLong_Type, &main_fd_object,
                                                  &term_type, &PyLong_Type, &log_level,
                                                  &margins_str,
                                                  &PyLong_Type, &margin_top, &PyLong_Type, &margin_right, &PyLong_Type, &margin_bottom, &PyLong_Type, &margin_left,
                                                  &flags));

    notcurses_options options = {0};

    options.termtype = term_type;

    options.loglevel = (ncloglevel_e)log_level;

    if (NULL != margins_str)
    {
        CHECK_NOTCURSES(notcurses_lex_margins(margins_str, &options));
    }

    if (NULL != margin_top)
    {
        options.margin_t = GNU_PY_UINT_CHECK(margin_top);
    }

    if (NULL != margin_right)
    {
        options.margin_r = GNU_PY_UINT_CHECK(margin_right);
    }

    if (NULL != margin_bottom)
    {
        options.margin_b = GNU_PY_UINT_CHECK(margin_bottom);
    }

    if (NULL != margin_left)
    {
        options.margin_l = GNU_PY_UINT_CHECK(margin_left);
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

    GNU_PY_CHECK_INT(PySys_AddAuditHook(Notcurses_uncaught_audit, NULL));

    return (PyObject *)new_context;
}

static PyObject *
Notcurses_drop_planes(NotcursesObject *self, PyObject *Py_UNUSED(args))
{
    notcurses_drop_planes(self->notcurses_ptr);
    Py_RETURN_NONE;
}

static PyObject *
Notcurses_render(NotcursesObject *self, PyObject *Py_UNUSED(args))
{
    CHECK_NOTCURSES(notcurses_render(self->notcurses_ptr));
    Py_RETURN_NONE;
}

static PyObject *
Notcurses_top(NotcursesObject *self, PyObject *Py_UNUSED(args))
{
    PyObject *new_object CLEANUP_PY_OBJ = NcPlane_Type.tp_alloc((PyTypeObject *)&NcPlane_Type, 0);
    NcPlaneObject *new_plane = (NcPlaneObject *)new_object;
    new_plane->ncplane_ptr = CHECK_NOTCURSES_PTR(notcurses_top(self->notcurses_ptr));

    Py_INCREF(new_object);
    return new_object;
}

static PyObject *
Notcurses_bottom(NotcursesObject *self, PyObject *Py_UNUSED(args))
{
    PyObject *new_object CLEANUP_PY_OBJ = NcPlane_Type.tp_alloc((PyTypeObject *)&NcPlane_Type, 0);
    NcPlaneObject *new_plane = (NcPlaneObject *)new_object;
    new_plane->ncplane_ptr = CHECK_NOTCURSES_PTR(notcurses_bottom(self->notcurses_ptr));

    Py_INCREF(new_object);
    return new_object;
}

static PyObject *
Notcurses_inputready_fd(NotcursesObject *self, PyObject *Py_UNUSED(args))
{
    int input_fd = notcurses_inputready_fd(self->notcurses_ptr);

    return PyLong_FromLong((long)input_fd);
}

static PyObject *
build_NcInput(uint32_t const id, ncinput const *const ni)
{
    if (id == (uint32_t)-1)
    {
        PyErr_Format(PyExc_RuntimeError, "notcurses_get_nblock return -1");
        return NULL;
    }
    else if (id == 0)
        // No input event.
        Py_RETURN_NONE;
    else
    {
        PyObject *input = GNU_PY_CHECK(PyStructSequence_New(NcInput_Type));
        PyObject *utf8 = GNU_PY_CHECK(PyUnicode_FromStringAndSize(ni->utf8, (Py_ssize_t)strlen(ni->utf8)));
        PyStructSequence_SET_ITEM(input, 0, PyLong_FromLong(ni->id));
        PyStructSequence_SET_ITEM(input, 1, PyLong_FromLong(ni->y));
        PyStructSequence_SET_ITEM(input, 2, PyLong_FromLong(ni->x));
        PyStructSequence_SET_ITEM(input, 3, utf8);
        PyStructSequence_SET_ITEM(input, 4, PyLong_FromLong(ni->evtype));
        PyStructSequence_SET_ITEM(input, 5, PyLong_FromLong(ni->modifiers));
        PyStructSequence_SET_ITEM(input, 6, PyLong_FromLong(ni->ypx));
        PyStructSequence_SET_ITEM(input, 7, PyLong_FromLong(ni->xpx));
        return input;
    }
}

static inline struct timespec secs_to_timespec(double const sec) {
    assert(sec >= 0);
    struct timespec const timespec = {
        .tv_sec = (time_t)sec,
        .tv_nsec = (long)((sec - (double)(time_t)sec) * 1E+9) };
    return timespec;
}

static PyObject *
Notcurses_get(NotcursesObject *self, PyObject *args, PyObject *kw)
{
    static char *keywords[] = {"deadline", NULL};
    PyObject* deadline_arg;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O", keywords, &deadline_arg))
        return NULL;

    struct timespec timespec;
    struct timespec *ts;
    if (deadline_arg == Py_None)
        // No deadline.
        ts = NULL;
    else
    {
        double const deadline = PyFloat_AsDouble(deadline_arg);
        if (PyErr_Occurred())
            // Can't convert to float sec.
            return NULL;
        if (deadline < 0)
        {
            PyErr_Format(PyExc_ValueError, "negative deadline");
            return NULL;
        }
        timespec = secs_to_timespec(deadline);
        ts = &timespec;
    }

    struct ncinput ni;
    uint32_t const id = notcurses_get(self->notcurses_ptr, ts, &ni);
    return build_NcInput(id, &ni);
}

static PyObject *
Notcurses_get_nblock(NotcursesObject *self, PyObject *Py_UNUSED(args))
{
    struct ncinput ni;
    uint32_t const id = notcurses_get_nblock(self->notcurses_ptr, &ni);
    return build_NcInput(id, &ni);
}

static PyObject *
Notcurses_get_blocking(NotcursesObject *self, PyObject *Py_UNUSED(args))
{
    struct ncinput ni;
    uint32_t const id = notcurses_get_blocking(self->notcurses_ptr, &ni);
    return build_NcInput(id, &ni);
}

static PyObject *
Notcurses_mice_enable(NotcursesObject *self, PyObject *Py_UNUSED(args))
{
    CHECK_NOTCURSES(notcurses_mice_enable(self->notcurses_ptr, NCMICE_BUTTON_EVENT));
    Py_RETURN_NONE;
}

static PyObject *
Notcurses_mice_disable(NotcursesObject *self, PyObject *Py_UNUSED(args))
{
    CHECK_NOTCURSES(notcurses_mice_disable(self->notcurses_ptr));
    Py_RETURN_NONE;
}

static PyObject *
Notcurses_linesigs_disable(NotcursesObject *self, PyObject *Py_UNUSED(args))
{
    CHECK_NOTCURSES(notcurses_linesigs_disable(self->notcurses_ptr));
    Py_RETURN_NONE;
}

static PyObject *
Notcurses_linesigs_enable(NotcursesObject *self, PyObject *Py_UNUSED(args))
{
    CHECK_NOTCURSES(notcurses_linesigs_enable(self->notcurses_ptr));
    Py_RETURN_NONE;
}

static PyObject *
Notcurses_refresh(NotcursesObject *self, PyObject *Py_UNUSED(args))
{
    unsigned rows = 0, columns = 0;
    CHECK_NOTCURSES(notcurses_refresh(self->notcurses_ptr, &rows, &columns));

    return Py_BuildValue("II", rows, columns);
}

static PyObject *
Notcurses_stdplane(NotcursesObject *self, PyObject *Py_UNUSED(args))
{
    PyObject *new_object CLEANUP_PY_OBJ = NcPlane_Type.tp_alloc((PyTypeObject *)&NcPlane_Type, 0);
    NcPlaneObject *new_plane = (NcPlaneObject *)new_object;
    new_plane->ncplane_ptr = CHECK_NOTCURSES_PTR(notcurses_stdplane(self->notcurses_ptr));

    Py_INCREF(new_object);
    return new_object;
}

static PyObject *
Notcurses_stddim_yx(NotcursesObject *self, PyObject *Py_UNUSED(args))
{
    unsigned y = 0, x = 0;
    PyObject *new_object CLEANUP_PY_OBJ = NcPlane_Type.tp_alloc((PyTypeObject *)&NcPlane_Type, 0);
    NcPlaneObject *new_plane = (NcPlaneObject *)new_object;
    new_plane->ncplane_ptr = CHECK_NOTCURSES_PTR(notcurses_stddim_yx(self->notcurses_ptr, &y, &x));

    Py_INCREF(new_object);
    return Py_BuildValue("OII", new_object, y, x);
}

static PyObject *
Notcurses_term_dim_yx(NotcursesObject *self, PyObject *Py_UNUSED(args))
{
    unsigned rows = 0, columns = 0;
    notcurses_term_dim_yx(self->notcurses_ptr, &rows, &columns);

    return Py_BuildValue("II", rows, columns);
}

static PyObject *
Notcurses_at_yx(NotcursesObject *Py_UNUSED(self), PyObject *Py_UNUSED(args), PyObject *Py_UNUSED(kwds))
{
    PyErr_SetString(PyExc_NotImplementedError, "TODO when EGC is implemented");
    return NULL;
}

static PyObject *
Notcurses_pile_create(NotcursesObject *self, PyObject *args, PyObject *kwds)
{
    int y = 0, x = 0;
    unsigned rows = 0, cols = 0;
    const char *name = NULL;
    // TODO reseize callback
    unsigned long long flags = 0;
    unsigned int margin_b = 0, margin_r = 0;

    char *keywords[] = {"y_pos", "x_pos",
                        "rows", "cols",
                        "name",
                        "flags",
                        "margin_b", "margin_r", NULL};

    GNU_PY_CHECK_BOOL(PyArg_ParseTupleAndKeywords(args, kwds, "|iiIIsKii", keywords,
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
    new_plane->ncplane_ptr = CHECK_NOTCURSES_PTR(ncpile_create(self->notcurses_ptr, &options));

    Py_INCREF(new_object);
    return new_object;
}

static PyObject *
Notcurses_supported_styles(NotcursesObject *self, PyObject *Py_UNUSED(args))
{
    unsigned int styles = notcurses_supported_styles(self->notcurses_ptr);
    return PyLong_FromLong((long)styles);
}

static PyObject *
Notcurses_palette_size(NotcursesObject *self, PyObject *Py_UNUSED(args))
{
    unsigned int pallete_size = notcurses_palette_size(self->notcurses_ptr);
    return PyLong_FromLong((long)pallete_size);
}

static PyObject *
Notcurses_cantruecolor(NotcursesObject *self, PyObject *Py_UNUSED(args))
{
    return PyBool_FromLong((long)notcurses_cantruecolor(self->notcurses_ptr));
}

static PyObject *
Notcurses_canfade(NotcursesObject *self, PyObject *Py_UNUSED(args))
{
    return PyBool_FromLong((long)notcurses_canfade(self->notcurses_ptr));
}

static PyObject *
Notcurses_canchangecolor(NotcursesObject *self, PyObject *Py_UNUSED(args))
{
    return PyBool_FromLong((long)notcurses_canchangecolor(self->notcurses_ptr));
}

static PyObject *
Notcurses_canopen_images(NotcursesObject *self, PyObject *Py_UNUSED(args))
{
    return PyBool_FromLong((long)notcurses_canopen_images(self->notcurses_ptr));
}

static PyObject *
Notcurses_canopen_videos(NotcursesObject *self, PyObject *Py_UNUSED(args))
{
    return PyBool_FromLong((long)notcurses_canopen_videos(self->notcurses_ptr));
}

static PyObject *
Notcurses_canutf8(NotcursesObject *self, PyObject *Py_UNUSED(args))
{
    return PyBool_FromLong((long)notcurses_canutf8(self->notcurses_ptr));
}

static PyObject *
Notcurses_cansextant(NotcursesObject *self, PyObject *Py_UNUSED(args))
{
    return PyBool_FromLong((long)notcurses_cansextant(self->notcurses_ptr));
}

static PyObject *
Notcurses_canbraille(NotcursesObject *self, PyObject *Py_UNUSED(args))
{
    return PyBool_FromLong((long)notcurses_canbraille(self->notcurses_ptr));
}

static PyObject *
Notcurses_check_pixel_support(NotcursesObject *self, PyObject *Py_UNUSED(args))
{
    int pixel_output_supported = CHECK_NOTCURSES(notcurses_check_pixel_support(self->notcurses_ptr));
    return PyLong_FromLong((long)pixel_output_supported);
}

static PyObject *
Notcurses_stats(NotcursesObject *Py_UNUSED(self), PyObject *Py_UNUSED(args))
{
    PyErr_SetString(PyExc_NotImplementedError, "TODO when stats is implemented");
    return NULL;
}

static PyObject *
Notcurses_stats_reset(NotcursesObject *Py_UNUSED(self), PyObject *Py_UNUSED(args))
{
    PyErr_SetString(PyExc_NotImplementedError, "TODO when stats is implemented");
    return NULL;
}

static PyObject *
Notcurses_cursor_enable(NotcursesObject *self, PyObject *args, PyObject *kwds)
{
    int y = 0, x = 0;

    char *keywords[] = {"y", "x", NULL};

    GNU_PY_CHECK_BOOL(
        PyArg_ParseTupleAndKeywords(args, kwds, "|ii", keywords,
                                    &y, &x));

    CHECK_NOTCURSES(notcurses_cursor_enable(self->notcurses_ptr, y, x));
    Py_RETURN_NONE;
}

static PyObject *
Notcurses_cursor_disable(NotcursesObject *self, PyObject *Py_UNUSED(args))
{
    CHECK_NOTCURSES(notcurses_cursor_disable(self->notcurses_ptr));
    Py_RETURN_NONE;
}

static PyMethodDef Notcurses_methods[] = {
    {"drop_planes", (PyCFunction)Notcurses_drop_planes, METH_NOARGS, "Destroy all ncplanes other than the stdplane."},

    {"render", (PyCFunction)Notcurses_render, METH_NOARGS, "Renders and rasterizes the standard pile in one shot. Blocking call."},

    {"top", (PyCFunction)Notcurses_top, METH_NOARGS, "Return the topmost ncplane of the standard pile."},
    {"bottom", (PyCFunction)Notcurses_bottom, METH_NOARGS, "Return the bottommost ncplane of the standard pile."},

    {"get", (void *)Notcurses_get, METH_VARARGS | METH_KEYWORDS, "See ppoll(2) for more detail. Provide a None 'ts' to block at length, a 'ts' of 0 for non-blocking operation, and otherwise a timespec to bound blocking. Signals in sigmask (less several we handle internally) will be atomically masked and unmasked per ppoll(2). It should generally contain all signals. Returns a single Unicode code point, or (char32_t)-1 on error. 'sigmask' may be NULL. Returns 0 on a timeout. If an event is processed, the return value is the 'id' field from that event. 'ni' may be NULL."},
    {"inputready_fd", (PyCFunction)Notcurses_inputready_fd, METH_NOARGS, "Get a file descriptor suitable for input event poll()ing. When this descriptor becomes available, you can call notcurses_getc_nblock(), and input ought be ready. This file descriptor is *not* necessarily the file descriptor associated with stdin (but it might be!)."},
    {"get_nblock", (PyCFunction)Notcurses_get_nblock, METH_NOARGS, "Get input event without blocking. If no event is ready, returns None."},
    {"get_blocking", (PyCFunction)Notcurses_get_blocking, METH_NOARGS, "Get input event completely blocking until and event or signal received."},

    {"mice_enable", (PyCFunction)Notcurses_mice_enable, METH_NOARGS, "Enable the mouse in \"button-event tracking\" mode with focus detection and UTF8-style extended coordinates. On success mouse events will be published to getc()"},
    {"mice_disable", (PyCFunction)Notcurses_mice_disable, METH_NOARGS, "Disable mouse events. Any events in the input queue can still be delivered."},
    {"linesigs_disable", (PyCFunction)Notcurses_linesigs_disable, METH_NOARGS, "Disable signals originating from the terminal's line discipline, i.e. SIGINT (^C), SIGQUIT (^\\), and SIGTSTP (^Z). They are enabled by default."},
    {"linesigs_enable", (PyCFunction)Notcurses_linesigs_enable, METH_NOARGS, "Restore signals originating from the terminal's line discipline, i.e. SIGINT (^C), SIGQUIT (^\\), and SIGTSTP (^Z), if disabled."},

    {"refresh", (PyCFunction)Notcurses_refresh, METH_NOARGS, "Refresh the physical screen to match what was last rendered. Return the new dimensions"},
    {"stdplane", (PyCFunction)Notcurses_stdplane, METH_NOARGS, "Get a reference to the standard plane (one matching our current idea of the terminal size) for this terminal. The standard plane always exists, and its origin is always at the uppermost, leftmost cell of the terminal."},
    {"stddim_yx", (PyCFunction)Notcurses_stddim_yx, METH_NOARGS, "Get standard plane plus dimensions dimensions."},
    {"term_dim_yx", (PyCFunction)Notcurses_term_dim_yx, METH_NOARGS, "Return our current idea of the terminal dimensions in rows and cols."},

    {"at_yx", (void *)Notcurses_at_yx, METH_VARARGS | METH_KEYWORDS, "Retrieve the contents of the specified cell as last rendered."},
    {"pile_create", (void *)Notcurses_pile_create, METH_VARARGS | METH_KEYWORDS, "Same as ncplane_create(), but creates a new pile. The returned plane will be the top, bottom, and root of this new pile."},

    {"supported_styles", (PyCFunction)Notcurses_supported_styles, METH_NOARGS, PyDoc_STR("Returns a 16-bit bitmask of supported curses-style attributes (NCSTYLE_UNDERLINE, NCSTYLE_BOLD, etc.) The attribute is only indicated as supported if the terminal can support it together with color. For more information, see the \"ncv\" capability in terminfo(5).")},
    {"palette_size", (PyCFunction)Notcurses_palette_size, METH_NOARGS, PyDoc_STR("Returns the number of simultaneous colors claimed to be supported, or 1 if there is no color support. Note that several terminal emulators advertise more colors than they actually support, downsampling internally.")},
    {"cantruecolor", (PyCFunction)Notcurses_cantruecolor, METH_NOARGS, PyDoc_STR("Can we directly specify RGB values per cell, or only use palettes?")},
    {"canfade", (PyCFunction)Notcurses_canfade, METH_NOARGS, PyDoc_STR("Can we fade? Fading requires either the \"rgb\" or \"ccc\" terminfo capability.")},
    {"canchangecolor", (PyCFunction)Notcurses_canchangecolor, METH_NOARGS, PyDoc_STR("Can we set the \"hardware\" palette? Requires the \"ccc\" terminfo capability.")},
    {"canopen_images", (PyCFunction)Notcurses_canopen_images, METH_NOARGS, PyDoc_STR("Can we set the \"hardware\" palette? Requires the \"ccc\" terminfo capability.")},
    {"canopen_videos", (PyCFunction)Notcurses_canopen_videos, METH_NOARGS, PyDoc_STR("Can we load videos? This requires being built against FFmpeg.")},
    {"canutf8", (PyCFunction)Notcurses_canutf8, METH_NOARGS, PyDoc_STR("Is our encoding UTF-8? Requires LANG being set to a UTF8 locale.")},
    {"cansextant", (PyCFunction)Notcurses_cansextant, METH_NOARGS, PyDoc_STR("Can we reliably use Unicode 13 sextants?")},
    {"canbraille", (PyCFunction)Notcurses_canbraille, METH_NOARGS, PyDoc_STR("Can we reliably use Unicode Braille?")},
    {"check_pixel_support", (PyCFunction)Notcurses_check_pixel_support, METH_NOARGS, PyDoc_STR("This function must successfully return before NCBLIT_PIXEL is available. Raises exception on error, 0 for no support, or 1 if pixel output is supported. Must not be called concurrently with either input or rasterization.")},

    {"stats", (PyCFunction)Notcurses_stats, METH_NOARGS, PyDoc_STR("Acquire an atomic snapshot of the Notcurses object's stats.")},
    {"stats_reset", (PyCFunction)Notcurses_stats_reset, METH_NOARGS, PyDoc_STR("Reset all cumulative stats (immediate ones, such as fbbytes, are not reset) and first returning a copy before reset.")},

    {"cursor_enable", (void *)Notcurses_cursor_enable, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Enable the terminal's cursor, if supported, placing it at 'y', 'x'. Immediate effect (no need for a call to notcurses_render()). It is an error if 'y', 'x' lies outside the standard plane.")},
    {"cursor_disable", (PyCFunction)Notcurses_cursor_disable, METH_NOARGS, PyDoc_STR("Disable the terminal's cursor.")},

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

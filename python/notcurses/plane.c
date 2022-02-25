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
NcPlane_new(PyTypeObject *Py_UNUSED(subtype), PyObject *Py_UNUSED(args), PyObject *Py_UNUSED(kwds))
{
    PyErr_SetString(PyExc_ValueError, "NcPlane should NOT be initialied directly.");
    return NULL;
}

static PyObject *
Ncplane_create(NcPlaneObject *self, PyObject *args, PyObject *kwds)
{
    int y = 0, x = 0;
    unsigned rows = 0, cols = 0;
    const char *name = NULL;
    // TODO resize callback
    unsigned long long flags = 0;
    unsigned int margin_b = 0, margin_r = 0;

    char *keywords[] = {"rows", "cols",
                        "y_pos", "x_pos",
                        "name",
                        "flags",
                        "margin_b", "margin_r", NULL};

    GNU_PY_CHECK_BOOL(PyArg_ParseTupleAndKeywords(args, kwds, "ii|iisKii", keywords,
                                                  &rows, &cols,
                                                  &y, &x,
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
        .resizecb = ncplane_resize_maximize,
    };

    PyObject *new_object CLEANUP_PY_OBJ = NcPlane_Type.tp_alloc((PyTypeObject *)&NcPlane_Type, 0);
    NcPlaneObject *new_plane = (NcPlaneObject *)new_object;
    new_plane->ncplane_ptr = CHECK_NOTCURSES_PTR(ncplane_create(self->ncplane_ptr, &options));

    Py_INCREF(new_object);
    return new_object;
}

static PyObject *
NcPlane_destroy(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    struct ncplane *temp_ptr = self->ncplane_ptr;
    self->ncplane_ptr = NULL;
    CHECK_NOTCURSES(ncplane_destroy(temp_ptr));

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_notcurses(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    PyObject *new_object CLEANUP_PY_OBJ = Notcurses_Type.tp_alloc((PyTypeObject *)&Notcurses_Type, 0);
    NotcursesObject *new_notcurses = (NotcursesObject *)new_object;
    new_notcurses->notcurses_ptr = CHECK_NOTCURSES_PTR(ncplane_notcurses(self->ncplane_ptr));

    Py_INCREF(new_object);
    return new_object;
}

static PyObject *
NcPlane_dim_yx(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    unsigned y = 0, x = 0;
    ncplane_dim_yx(self->ncplane_ptr, &y, &x);

    return Py_BuildValue("II", y, x);
}

static PyObject *
NcPlane_dim_x(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    return Py_BuildValue("I", ncplane_dim_x(self->ncplane_ptr));
}

static PyObject *
NcPlane_dim_y(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    return Py_BuildValue("I", ncplane_dim_y(self->ncplane_ptr));
}

static PyObject *
NcPlane_pixel_geom(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    unsigned pxy = 0, pxx = 0, celldimy = 0, celldimx = 0, maxbmapy = 0, maxbmapx = 0;
    ncplane_pixel_geom(self->ncplane_ptr, &pxy, &pxx, &celldimy, &celldimx, &maxbmapy, &maxbmapx);

    return Py_BuildValue("II II II", pxy, pxx, celldimy, celldimx, maxbmapy, maxbmapx);
}

static PyObject *
NcPlane_set_resizecb(NcPlaneObject *Py_UNUSED(self), PyObject *Py_UNUSED(args))
{
    PyErr_SetString(PyExc_NotImplementedError, "TODO");
    return NULL;
}

static PyObject *
NcPlane_reparent(NcPlaneObject *self, PyObject *args)
{
    NcPlaneObject *new_parent = NULL;

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "O!", &NcPlane_Type, &new_parent));

    CHECK_NOTCURSES_PTR(ncplane_reparent(self->ncplane_ptr, new_parent->ncplane_ptr));

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_reparent_family(NcPlaneObject *self, PyObject *args)
{
    NcPlaneObject *new_parent = NULL;

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "O!", &NcPlane_Type, &new_parent));

    CHECK_NOTCURSES_PTR(ncplane_reparent_family(self->ncplane_ptr, new_parent->ncplane_ptr));

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_dup(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    PyObject *new_object CLEANUP_PY_OBJ = NcPlane_Type.tp_alloc((PyTypeObject *)&NcPlane_Type, 0);
    NcPlaneObject *new_plane = (NcPlaneObject *)new_object;
    new_plane->ncplane_ptr = CHECK_NOTCURSES_PTR(ncplane_dup(self->ncplane_ptr, NULL));

    Py_INCREF(new_object);
    return new_object;
}

static PyObject *
NcPlane_translate(NcPlaneObject *self, PyObject *args, PyObject *kwds)
{
    NcPlaneObject *dst_obj = NULL;
    int y = 0, x = 0;

    char *keywords[] = {"dst",
                        NULL};

    GNU_PY_CHECK_BOOL(PyArg_ParseTupleAndKeywords(args, kwds, "O!", keywords,
                                                  &NcPlane_Type, &dst_obj));

    ncplane_translate(self->ncplane_ptr, dst_obj->ncplane_ptr, &y, &x);

    return Py_BuildValue("ii", &y, &x);
}

static PyObject *
NcPlane_translate_abs(NcPlaneObject *self, PyObject *args)
{
    int x = 0, y = 0;
    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "ii", &y, &x));

    return PyBool_FromLong((long)ncplane_translate_abs(self->ncplane_ptr, &y, &x));
}

static PyObject *
NcPlane_set_scrolling(NcPlaneObject *self, PyObject *args)
{
    int scrollp_int = 0;

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "p", &scrollp_int));

    return PyBool_FromLong((long)ncplane_set_scrolling(self->ncplane_ptr, (bool)scrollp_int));
}

static PyObject *
NcPlane_resize(NcPlaneObject *self, PyObject *args, PyObject *kwds)
{
    int keepy = 0, keepx = 0;
    unsigned keepleny = 0, keeplenx = 0;
    int yoff = 0, xoff = 0;
    unsigned ylen = 0, xlen = 0;

    char *keywords[] = {"keepy", "keepx",
                        "keepleny", "keeplenx",
                        "yoff", "xoff",
                        "ylen", "xlen",
                        NULL};

    GNU_PY_CHECK_BOOL(PyArg_ParseTupleAndKeywords(args, kwds, "iiIIiiII", keywords,
                                                  &keepy, &keepx,
                                                  &keepleny, &keeplenx,
                                                  &yoff, &xoff,
                                                  &ylen, &xlen));

    CHECK_NOTCURSES(ncplane_resize(self->ncplane_ptr, keepy, keepx, keepleny, keeplenx, yoff, xoff, ylen, xlen));

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_resize_simple(NcPlaneObject *self, PyObject *args)
{
    unsigned ylen = 0, xlen = 0;

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "II", &ylen, &xlen));

    CHECK_NOTCURSES(ncplane_resize_simple(self->ncplane_ptr, ylen, xlen));

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_set_base_cell(NcPlaneObject *Py_UNUSED(self), PyObject *Py_UNUSED(args))
{
    PyErr_SetString(PyExc_NotImplementedError, "TODO when cells are added");
    return NULL;
}

static PyObject *
NcPlane_set_base(NcPlaneObject *Py_UNUSED(self), PyObject *Py_UNUSED(args), PyObject *Py_UNUSED(kwds))
{
    PyErr_SetString(PyExc_NotImplementedError, "TODO when cells are added");
    return NULL;
}

static PyObject *
NcPlane_base(NcPlaneObject *Py_UNUSED(self), PyObject *Py_UNUSED(args))
{
    PyErr_SetString(PyExc_NotImplementedError, "TODO when cells are added");
    return NULL;
}

static PyObject *
NcPlane_move_yx(NcPlaneObject *self, PyObject *args)
{
    int y = 0, x = 0;

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "ii", &y, &x));

    CHECK_NOTCURSES(ncplane_move_yx(self->ncplane_ptr, y, x));

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_yx(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    int y = 0, x = 0;

    ncplane_yx(self->ncplane_ptr, &y, &x);

    return Py_BuildValue("ii", y, x);
}

static PyObject *
NcPlane_y(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    return PyLong_FromLong((long)ncplane_y(self->ncplane_ptr));
}

static PyObject *
NcPlane_x(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    return PyLong_FromLong((long)ncplane_x(self->ncplane_ptr));
}

static PyObject *
NcPlane_abs_yx(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    int y = 0, x = 0;

    ncplane_abs_yx(self->ncplane_ptr, &y, &x);

    return Py_BuildValue("ii", y, x);
}

static PyObject *
NcPlane_abs_y(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    return PyLong_FromLong((long)ncplane_abs_y(self->ncplane_ptr));
}

static PyObject *
NcPlane_abs_x(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    return PyLong_FromLong((long)ncplane_abs_x(self->ncplane_ptr));
}

static PyObject *
NcPlane_parent(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    struct ncplane *possible_parent = ncplane_parent(self->ncplane_ptr);

    if (NULL == possible_parent)
    {
        Py_RETURN_NONE;
    }
    else
    {
        PyObject *new_object CLEANUP_PY_OBJ = NcPlane_Type.tp_alloc((PyTypeObject *)&Notcurses_Type, 0);
        NcPlaneObject *new_plane = (NcPlaneObject *)new_object;
        new_plane->ncplane_ptr = possible_parent;

        Py_INCREF(new_object);
        return new_object;
    }
}

static PyObject *
NcPlane_descendant_p(NcPlaneObject *self, PyObject *args)
{
    NcPlaneObject *ancestor_obj = NULL;

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "O!", &NcPlane_Type, &ancestor_obj));

    return PyBool_FromLong((long)ncplane_descendant_p(self->ncplane_ptr, ancestor_obj->ncplane_ptr));
}

static PyObject *
NcPlane_move_top(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    ncplane_move_top(self->ncplane_ptr);
    Py_RETURN_NONE;
}

static PyObject *
NcPlane_move_bottom(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    ncplane_move_bottom(self->ncplane_ptr);
    Py_RETURN_NONE;
}

static PyObject *
NcPlane_move_above(NcPlaneObject *self, PyObject *args)
{
    NcPlaneObject *above_obj = NULL;

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "O!", &NcPlane_Type, &above_obj));

    CHECK_NOTCURSES(ncplane_move_above(self->ncplane_ptr, above_obj->ncplane_ptr));

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_move_below(NcPlaneObject *self, PyObject *args)
{
    NcPlaneObject *bellow_obj = NULL;

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "O!", &NcPlane_Type, &bellow_obj));

    CHECK_NOTCURSES(ncplane_move_below(self->ncplane_ptr, bellow_obj->ncplane_ptr));

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_below(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    struct ncplane *possible_bellow = ncplane_below(self->ncplane_ptr);

    if (NULL == possible_bellow)
    {
        Py_RETURN_NONE;
    }
    else
    {
        NcPlaneObject *new_plane = (NcPlaneObject *)NcPlane_Type.tp_alloc((PyTypeObject *)&Notcurses_Type, 0);
        new_plane->ncplane_ptr = possible_bellow;

        return (PyObject *)new_plane;
    }
}

static PyObject *
NcPlane_above(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    struct ncplane *possible_above = ncplane_above(self->ncplane_ptr);

    if (NULL == possible_above)
    {
        Py_RETURN_NONE;
    }
    else
    {
        NcPlaneObject *new_plane = (NcPlaneObject *)NcPlane_Type.tp_alloc((PyTypeObject *)&Notcurses_Type, 0);
        new_plane->ncplane_ptr = possible_above;

        return (PyObject *)new_plane;
    }
}

static PyObject *
NcPlane_rotate_cw(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    CHECK_NOTCURSES(ncplane_rotate_cw(self->ncplane_ptr));
    Py_RETURN_NONE;
}

static PyObject *
NcPlane_rotate_ccw(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    CHECK_NOTCURSES(ncplane_rotate_ccw(self->ncplane_ptr));
    Py_RETURN_NONE;
}

static PyObject *
NcPlane_at_cursor(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    uint16_t style_mask = 0;
    uint64_t channels = 0;
    char *egc = CHECK_NOTCURSES_PTR(ncplane_at_cursor(self->ncplane_ptr, &style_mask, &channels));

    PyObject *egc_str CLEANUP_PY_OBJ = GNU_PY_CHECK(PyUnicode_FromString(egc));
    free(egc);

    return Py_BuildValue("OHK", egc_str, (unsigned short)style_mask, (unsigned long long)channels);
}

static PyObject *
NcPlane_at_cursor_cell(NcPlaneObject *Py_UNUSED(self), PyObject *Py_UNUSED(args))
{
    PyErr_SetString(PyExc_NotImplementedError, "TODO when cells are added");
    return NULL;
    // ncplane_at_cursor_cell();
}

static PyObject *
NcPlane_at_yx(NcPlaneObject *self, PyObject *args)
{
    uint16_t style_mask = 0;
    uint64_t channels = 0;
    int y = 0, x = 0;

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "ii", &y, &x));

    char *egc = CHECK_NOTCURSES_PTR(ncplane_at_yx(self->ncplane_ptr, y, x, &style_mask, &channels));

    PyObject *egc_str CLEANUP_PY_OBJ = GNU_PY_CHECK(PyUnicode_FromString(egc));
    free(egc);

    return Py_BuildValue("OHK", egc_str, (unsigned short)style_mask, (unsigned long long)channels);
}

static PyObject *
NcPlane_at_yx_cell(NcPlaneObject *Py_UNUSED(self), PyObject *Py_UNUSED(args))
{
    PyErr_SetString(PyExc_NotImplementedError, "TODO when cells are added");
    return NULL;
    // ncplane_at_yx_cell();
}

static PyObject *
NcPlane_contents(NcPlaneObject *self, PyObject *args, PyObject *kwds)
{
    int beg_y = 0, beg_x = 0;
    unsigned len_y = 0, len_x = 0;

    char *keywords[] = {"begy", "begx", "leny", "lenx", NULL};

    GNU_PY_CHECK_BOOL(PyArg_ParseTupleAndKeywords(args, kwds, "ii|II", keywords,
                                                  &beg_y, &beg_x,
                                                  &len_y, &len_x));

    char *egcs = CHECK_NOTCURSES_PTR(ncplane_contents(self->ncplane_ptr, beg_y, beg_x, len_y, len_x));

    PyObject *egcs_str CLEANUP_PY_OBJ = GNU_PY_CHECK(PyUnicode_FromString(egcs));
    free(egcs);

    return Py_BuildValue("s", egcs_str);
}

static PyObject *
NcPlane_center_abs(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    int y = 0, x = 0;
    ncplane_center_abs(self->ncplane_ptr, &y, &x);

    return Py_BuildValue("ii", y, x);
}

static PyObject *
NcPlane_halign(NcPlaneObject *self, PyObject *args)
{
    int align = 0, c = 0;

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "ii", &align, &c));
    int collumn = CHECK_NOTCURSES(ncplane_halign(self->ncplane_ptr, (ncalign_e)align, c));

    return Py_BuildValue("i", collumn);
}

static PyObject *
NcPlane_valign(NcPlaneObject *self, PyObject *args)
{
    int align = 0, r = 0;

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "ii", &align, &r));
    int row = CHECK_NOTCURSES(ncplane_valign(self->ncplane_ptr, (ncalign_e)align, r));

    return Py_BuildValue("i", row);
}

static PyObject *
NcPlane_cursor_move_yx(NcPlaneObject *self, PyObject *args)
{
    int y = 0, x = 0;

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "ii", &y, &x));

    CHECK_NOTCURSES(ncplane_cursor_move_yx(self->ncplane_ptr, y, x));

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_home(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    ncplane_home(self->ncplane_ptr);
    Py_RETURN_NONE;
}

static PyObject *
NcPlane_cursor_yx(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    unsigned y = 0, x = 0;

    ncplane_cursor_yx(self->ncplane_ptr, &y, &x);

    return Py_BuildValue("II", y, x);
}

static PyObject *
NcPlane_channels(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    uint64_t channels = ncplane_channels(self->ncplane_ptr);
    return Py_BuildValue("K", (unsigned long long)channels);
}

static PyObject *
NcPlane_styles(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    uint16_t styles = ncplane_styles(self->ncplane_ptr);
    return Py_BuildValue("H", (unsigned short)styles);
}

static PyObject *
NcPlane_putc_yx(NcPlaneObject *Py_UNUSED(self), PyObject *Py_UNUSED(args))
{
    PyErr_SetString(PyExc_NotImplementedError, "TODO when cells are added");
    return NULL;
}

static PyObject *
NcPlane_putc(NcPlaneObject *Py_UNUSED(self), PyObject *Py_UNUSED(args))
{
    PyErr_SetString(PyExc_NotImplementedError, "TODO when cells are added");
    return NULL;
}

static PyObject *
NcPlane_putchar_yx(NcPlaneObject *self, PyObject *args)
{
    int y = 0, x = 0;
    const char *c_str = NULL;

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "iis", &y, &x, &c_str));

    CHECK_NOTCURSES(ncplane_putchar_yx(self->ncplane_ptr, y, x, c_str[0]));

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_putchar(NcPlaneObject *self, PyObject *args)
{
    const char *c_str = NULL;

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "s", &c_str));

    CHECK_NOTCURSES(ncplane_putchar(self->ncplane_ptr, c_str[0]));

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_putchar_stained(NcPlaneObject *self, PyObject *args)
{
    const char *c_str = NULL;

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "s", &c_str));

    CHECK_NOTCURSES(ncplane_putchar_stained(self->ncplane_ptr, c_str[0]));

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_putegc_yx(NcPlaneObject *self, PyObject *args)
{
    int y = 0, x = 0;
    const char *egc = NULL;
    size_t sbytes = 0;

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "iis",
                                       &y, &x,
                                       &egc));

    CHECK_NOTCURSES(ncplane_putegc_yx(self->ncplane_ptr, y, x, egc, &sbytes));

    return Py_BuildValue("n", sbytes);
}

static PyObject *
NcPlane_putegc(NcPlaneObject *self, PyObject *args)
{
    const char *egc = NULL;
    size_t sbytes = 0;

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "s",
                                       &egc));

    CHECK_NOTCURSES(ncplane_putegc(self->ncplane_ptr, egc, &sbytes));

    return Py_BuildValue("n", sbytes);
}

static PyObject *
NcPlane_putegc_stained(NcPlaneObject *self, PyObject *args)
{
    const char *egc = NULL;
    size_t sbytes = 0;

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "s",
                                       &egc));

    CHECK_NOTCURSES(ncplane_putegc(self->ncplane_ptr, egc, &sbytes));

    return Py_BuildValue("n", sbytes);
}

static PyObject *
NcPlane_putstr_yx(NcPlaneObject *self, PyObject *args)
{
    int y = 0, x = 0;
    const char *egc = NULL;

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "iis",
                                       &y, &x,
                                       &egc));

    CHECK_NOTCURSES(ncplane_putstr_yx(self->ncplane_ptr, y, x, egc));

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_putstr(NcPlaneObject *self, PyObject *args)
{
    const char *egc = NULL;

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "s",
                                       &egc));

    CHECK_NOTCURSES(ncplane_putstr(self->ncplane_ptr, egc));

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_putstr_aligned(NcPlaneObject *self, PyObject *args)
{
    int y = 0, align_int = 0;
    const char *egc = NULL;

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "iis",
                                       &y, &align_int,
                                       &egc));

    CHECK_NOTCURSES(ncplane_putstr_aligned(self->ncplane_ptr, y, (ncalign_e)align_int, egc));

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_putstr_stained(NcPlaneObject *self, PyObject *args)
{
    const char *egc = NULL;

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "s",
                                       &egc));

    CHECK_NOTCURSES(ncplane_putstr_stained(self->ncplane_ptr, egc));

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_putnstr_yx(NcPlaneObject *self, PyObject *args)
{
    int y = 0, x = 0;
    Py_ssize_t s = 0;
    const char *egc = NULL;

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "iins",
                                       &y, &x,
                                       &s,
                                       &egc));

    CHECK_NOTCURSES(ncplane_putnstr_yx(self->ncplane_ptr, y, x, (size_t)s, egc));

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_putnstr(NcPlaneObject *self, PyObject *args)
{
    Py_ssize_t s = 0;
    const char *egc = NULL;

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "ns",
                                       &s,
                                       &egc));

    CHECK_NOTCURSES(ncplane_putnstr(self->ncplane_ptr, (size_t)s, egc));

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_putnstr_aligned(NcPlaneObject *self, PyObject *args)
{
    int y = 0, align_int = 0;
    Py_ssize_t s = 0;
    const char *egc = NULL;

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "iins",
                                       &y, &align_int,
                                       &s,
                                       &egc));

    CHECK_NOTCURSES(ncplane_putnstr_aligned(self->ncplane_ptr, y, (ncalign_e)align_int, (size_t)s, egc));

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_puttext(NcPlaneObject *self, PyObject *args)
{
    int y = 0, align_int = 0;
    const char *text = NULL;
    size_t bytes_written = 0;

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "iis",
                                       &y, &align_int,
                                       &text));

    CHECK_NOTCURSES(ncplane_puttext(self->ncplane_ptr, y, (ncalign_e)align_int, text, &bytes_written));

    return Py_BuildValue("n", (Py_ssize_t)bytes_written);
}

static PyObject *
NcPlane_box(NcPlaneObject *Py_UNUSED(self), PyObject *Py_UNUSED(args), PyObject *Py_UNUSED(kwds))
{
    PyErr_SetString(PyExc_NotImplementedError, "TODO when cells are added");
    return NULL;
    // ncplane_box
}

static PyObject *
NcPlane_box_sized(NcPlaneObject *Py_UNUSED(self), PyObject *Py_UNUSED(args), PyObject *Py_UNUSED(kwds))
{
    PyErr_SetString(PyExc_NotImplementedError, "TODO when cells are added");
    return NULL;
    // ncplane_box_sized
}

static PyObject *
NcPlane_perimeter(NcPlaneObject *Py_UNUSED(self), PyObject *Py_UNUSED(args), PyObject *Py_UNUSED(kwds))
{
    PyErr_SetString(PyExc_NotImplementedError, "TODO when cells are added");
    return NULL;
    // ncplane_perimeter
}

static PyObject *
NcPlane_polyfill_yx(NcPlaneObject *Py_UNUSED(self), PyObject *Py_UNUSED(args), PyObject *Py_UNUSED(kwds))
{
    PyErr_SetString(PyExc_NotImplementedError, "TODO when cells are added");
    return NULL;
    // ncplane_polyfill_yx
}

static PyObject *
NcPlane_gradient(NcPlaneObject *self, PyObject *args, PyObject *kwds)
{
    int y = -1, x = -1;
    unsigned ylen = 0, xlen = 0;
    const char *egc = NULL;
    unsigned long stylemask = 0;
    unsigned long long ul = 0, ur = 0, ll = 0, lr = 0;

    char *keywords[] = {"y", "x", "ylen", "xlen", "egc",
                        "stylemask",
                        "ul", "ur", "ll", "lr",
                        NULL};
    GNU_PY_CHECK_BOOL(PyArg_ParseTupleAndKeywords(args, kwds, "siiIIkKKKK", keywords,
                                                  &y, &x, &ylen, &xlen, &egc,
                                                  &stylemask,
                                                  &ul, &ur, &ll, &lr));

    int cells_filled = CHECK_NOTCURSES(
        ncplane_gradient(
            self->ncplane_ptr, y, x, ylen, xlen, egc,
            (uint16_t)stylemask,
            (uint64_t)ul, (uint64_t)ur, (uint64_t)ll, (uint64_t)lr));

    return Py_BuildValue("i", cells_filled);
}

static PyObject *
NcPlane_gradient2x1(NcPlaneObject *self, PyObject *args, PyObject *kwds)
{
    unsigned long ul = 0, ur = 0, ll = 0, lr = 0;
    int y = -1, x = -1;
    unsigned ylen = 0, xlen = 0;

    char *keywords[] = {"y", "x", "ylen", "xlen", "ul", "ur", "ll", "lr",
                        NULL};
    GNU_PY_CHECK_BOOL(PyArg_ParseTupleAndKeywords(args, kwds, "iiIIkkkk", keywords,
                                                  &y, &x, &ylen, &xlen,
                                                  &ul, &ur, &ll, &lr));

    int cells_filled = CHECK_NOTCURSES(
        ncplane_gradient2x1(
            self->ncplane_ptr, y, x, ylen, xlen,
            (uint32_t)ul, (uint32_t)ur, (uint32_t)ll, (uint32_t)lr));

    return Py_BuildValue("i", cells_filled);
}

static PyObject *
NcPlane_format(NcPlaneObject *self, PyObject *args)
{
    int y = -1, x = -1;
    unsigned ylen = 0, xlen = 0;
    unsigned long stylemark = 0;

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "iiIIk",
                                       &y, &x, &ylen, &xlen,
                                       &stylemark));

    int cells_set = CHECK_NOTCURSES(ncplane_format(self->ncplane_ptr,
                                                   y, x, ylen, xlen,
                                                   (uint16_t)stylemark));

    return Py_BuildValue("i", cells_set);
}

static PyObject *
NcPlane_stain(NcPlaneObject *self, PyObject *args, PyObject *kwds)
{
    int x = -1, y = -1;
    unsigned ylen = 0, xlen = 0;
    unsigned long long ul = 0, ur = 0, ll = 0, lr = 0;

    char *keywords[] = {
        "ystop", "xstop",
        "ul", "ur", "ll", "lr",
        NULL};

    GNU_PY_CHECK_BOOL(PyArg_ParseTupleAndKeywords(args, kwds, "iiIIKKKK", keywords,
                                                  &y, &x, &ylen, &xlen,
                                                  &ul, &ur, &ll, &lr));

    int cells_set = CHECK_NOTCURSES(
        ncplane_stain(
            self->ncplane_ptr,
            y, x, ylen, xlen,
            (uint64_t)ul, (uint64_t)ur, (uint64_t)ll, (uint64_t)lr));

    return Py_BuildValue("i", cells_set);
}

static PyObject *
NcPlane_mergedown_simple(NcPlaneObject *self, PyObject *args)
{
    NcPlaneObject *dst_obj = NULL;

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "|O!", &NcPlane_Type, &dst_obj));

    CHECK_NOTCURSES(ncplane_mergedown_simple(self->ncplane_ptr, dst_obj->ncplane_ptr));

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_mergedown(NcPlaneObject *self, PyObject *args, PyObject *kwds)
{
    NcPlaneObject *dst_obj = NULL;
    int begsrcy = 0, begsrcx = 0;
    unsigned leny = 0, lenx = 0;
    int dsty = 0, dstx = 0;

    char *keywords[] = {"dst",
                        "begsrcy", "begsrcx", "leny", "lenx",
                        "dsty", "dstx",
                        NULL};
    GNU_PY_CHECK_BOOL(PyArg_ParseTupleAndKeywords(args, kwds, "O!iiIIii", keywords,
                                                  &NcPlane_Type, &dst_obj,
                                                  &begsrcy, &begsrcx, &leny, &lenx,
                                                  &dsty, &dstx));

    CHECK_NOTCURSES(ncplane_mergedown(
        self->ncplane_ptr, dst_obj->ncplane_ptr,
        begsrcy, begsrcx, leny, lenx,
        dsty, dstx));

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_erase(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    ncplane_erase(self->ncplane_ptr);
    Py_RETURN_NONE;
}

static PyObject *
NcPlane_bchannel(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    return Py_BuildValue("k", (unsigned long)ncplane_bchannel(self->ncplane_ptr));
}

static PyObject *
NcPlane_fchannel(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    return Py_BuildValue("k", (unsigned long)ncplane_fchannel(self->ncplane_ptr));
}

static PyObject *
NcPlane_set_channels(NcPlaneObject *self, PyObject *args)
{
    unsigned long long channels = 0;

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "K", &channels));

    ncplane_set_channels(self->ncplane_ptr, (uint64_t)channels);

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_set_styles(NcPlaneObject *self, PyObject *args)
{
    unsigned int stylebits = 0;

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "I", &stylebits));

    ncplane_set_styles(self->ncplane_ptr, stylebits);

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_on_styles(NcPlaneObject *self, PyObject *args)
{
    unsigned int stylebits = 0;

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "I", &stylebits));

    ncplane_on_styles(self->ncplane_ptr, stylebits);

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_off_styles(NcPlaneObject *self, PyObject *args)
{
    unsigned int stylebits = 0;

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "I", &stylebits));

    ncplane_off_styles(self->ncplane_ptr, stylebits);

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_fg_rgb(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    return Py_BuildValue("k", (unsigned long)ncplane_fg_rgb(self->ncplane_ptr));
}

static PyObject *
NcPlane_bg_rgb(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    return Py_BuildValue("k", (unsigned long)ncplane_bg_rgb(self->ncplane_ptr));
}

static PyObject *
NcPlane_fg_alpha(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    return Py_BuildValue("k", (unsigned long)ncplane_fg_alpha(self->ncplane_ptr));
}

static PyObject *
NcPlane_fg_default_p(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    return PyBool_FromLong((long)ncplane_fg_default_p(self->ncplane_ptr));
}

static PyObject *
NcPlane_bg_alpha(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    return Py_BuildValue("k", (unsigned long)ncplane_bg_alpha(self->ncplane_ptr));
}

static PyObject *
NcPlane_bg_default_p(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    return PyBool_FromLong((long)ncplane_bg_default_p(self->ncplane_ptr));
}

static PyObject *
NcPlane_fg_rgb8(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    unsigned int r = 0, g = 0, b = 0;
    ncplane_fg_rgb8(self->ncplane_ptr, &r, &g, &b);

    return Py_BuildValue("III", r, g, b);
}

static PyObject *
NcPlane_bg_rgb8(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    unsigned int r = 0, g = 0, b = 0;
    ncplane_bg_rgb8(self->ncplane_ptr, &r, &g, &b);

    return Py_BuildValue("III", r, g, b);
}

static PyObject *
NcPlane_set_fchannel(NcPlaneObject *self, PyObject *args)
{
    unsigned long channel = 0;
    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "k", &channel));

    return Py_BuildValue("K", (unsigned long long)ncplane_set_fchannel(self->ncplane_ptr, (uint32_t)channel));
}

static PyObject *
NcPlane_set_bchannel(NcPlaneObject *self, PyObject *args)
{
    unsigned long channel = 0;
    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "k", &channel));

    return Py_BuildValue("K", (unsigned long long)ncplane_set_bchannel(self->ncplane_ptr, (uint32_t)channel));
}

static PyObject *
NcPlane_set_fg_rgb8(NcPlaneObject *self, PyObject *args)
{
    unsigned r = 0, g = 0, b = 0;

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "III", &r, &g, &b));

    CHECK_NOTCURSES(ncplane_set_fg_rgb8(self->ncplane_ptr, r, g, b));

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_set_bg_rgb8(NcPlaneObject *self, PyObject *args)
{
    unsigned r = 0, g = 0, b = 0;

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "III", &r, &g, &b));

    CHECK_NOTCURSES(ncplane_set_bg_rgb8(self->ncplane_ptr, r, g, b));

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_set_bg_rgb8_clipped(NcPlaneObject *self, PyObject *args)
{
    int r = 0, g = 0, b = 0;

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "iii", &r, &g, &b));

    ncplane_set_bg_rgb8_clipped(self->ncplane_ptr, r, g, b);

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_set_fg_rgb8_clipped(NcPlaneObject *self, PyObject *args)
{
    int r = 0, g = 0, b = 0;

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "iii", &r, &g, &b));

    ncplane_set_fg_rgb8_clipped(self->ncplane_ptr, r, g, b);

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_set_fg_rgb(NcPlaneObject *self, PyObject *args)
{
    unsigned long channel = 0;

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "k", &channel));

    CHECK_NOTCURSES(ncplane_set_fg_rgb(self->ncplane_ptr, (uint32_t)channel));

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_set_bg_rgb(NcPlaneObject *self, PyObject *args)
{
    unsigned long channel = 0;

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "k", &channel));

    CHECK_NOTCURSES(ncplane_set_bg_rgb(self->ncplane_ptr, (uint32_t)channel));

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_set_fg_default(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    ncplane_set_fg_default(self->ncplane_ptr);

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_set_bg_default(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    ncplane_set_bg_default(self->ncplane_ptr);

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_set_fg_palindex(NcPlaneObject *self, PyObject *args)
{
    unsigned idx = 0;
    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "I", &idx));
    ncplane_set_fg_palindex(self->ncplane_ptr, idx);

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_set_bg_palindex(NcPlaneObject *self, PyObject *args)
{
    unsigned idx = 0;
    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "I", &idx));
    ncplane_set_bg_palindex(self->ncplane_ptr, idx);

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_set_fg_alpha(NcPlaneObject *self, PyObject *args)
{
    int alpha = 0;
    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "i", &alpha));
    ncplane_set_fg_alpha(self->ncplane_ptr, alpha);

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_set_bg_alpha(NcPlaneObject *self, PyObject *args)
{
    int alpha = 0;
    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "i", &alpha));
    ncplane_set_bg_alpha(self->ncplane_ptr, alpha);

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_fadeout(NcPlaneObject *Py_UNUSED(self), PyObject *Py_UNUSED(args))
{
    PyErr_SetString(PyExc_NotImplementedError, "TODO when fader is added");
    return NULL;
    // ncplane_fadeout
}

static PyObject *
NcPlane_fadein(NcPlaneObject *Py_UNUSED(self), PyObject *Py_UNUSED(args))
{
    PyErr_SetString(PyExc_NotImplementedError, "TODO when fader is added");
    return NULL;
    // ncplane_fadein
}

static PyObject *
NcPlane_fade_setup(NcPlaneObject *Py_UNUSED(self), PyObject *Py_UNUSED(args))
{
    PyErr_SetString(PyExc_NotImplementedError, "TODO when fader is added");
    return NULL;
    // ncfadectx_setup
}

static PyObject *
NcPlane_fadeout_iteration(NcPlaneObject *Py_UNUSED(self), PyObject *Py_UNUSED(args))
{
    PyErr_SetString(PyExc_NotImplementedError, "TODO when fader is added");
    return NULL;
    // ncplane_fadeout_iteration
}

static PyObject *
NcPlane_fadein_iteration(NcPlaneObject *Py_UNUSED(self), PyObject *Py_UNUSED(args))
{
    PyErr_SetString(PyExc_NotImplementedError, "TODO when fader is added");
    return NULL;
    // ncplane_fadein_iteration
}

static PyObject *
NcPlane_pulse(NcPlaneObject *Py_UNUSED(self), PyObject *Py_UNUSED(args))
{
    PyErr_SetString(PyExc_NotImplementedError, "TODO when fader is added");
    return NULL;
    // ncplane_pulse
}

static PyObject *
NcPlane_cells_load_box(NcPlaneObject *Py_UNUSED(self), PyObject *Py_UNUSED(args), PyObject *Py_UNUSED(kwds))
{
    PyErr_SetString(PyExc_NotImplementedError, "TODO when cells are added");
    return NULL;
    // nccells_load_box
}

static PyObject *
NcPlane_cells_rounded_box(NcPlaneObject *Py_UNUSED(self), PyObject *Py_UNUSED(args), PyObject *Py_UNUSED(kwds))
{
    PyErr_SetString(PyExc_NotImplementedError, "TODO when cells are added");
    return NULL;
    // nccells_rounded_box
}

static PyObject *
NcPlane_perimeter_rounded(NcPlaneObject *self, PyObject *args, PyObject *kwds)
{
    unsigned long stylemask = 0;
    unsigned long long channels = 0;
    unsigned int ctlword = 0;

    char *keywords[] = {"stylemask", "channels", "ctlword", NULL};

    GNU_PY_CHECK_BOOL(PyArg_ParseTupleAndKeywords(args, kwds, "kKI", keywords,
                                                  &stylemask,
                                                  &channels,
                                                  &ctlword));

    CHECK_NOTCURSES(ncplane_perimeter_rounded(self->ncplane_ptr, (uint16_t)stylemask, (uint64_t)channels, ctlword));

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_rounded_box_sized(NcPlaneObject *self, PyObject *args, PyObject *kwds)
{
    unsigned long styles = 0;
    unsigned long long channels = 0;
    unsigned ylen = 0, xlen = 0;
    unsigned int ctlword = 0;

    char *keywords[] = {"styles",
                        "channels",
                        "ylen", "xlen",
                        "ctlword",
                        NULL};

    GNU_PY_CHECK_BOOL(
        PyArg_ParseTupleAndKeywords(
            args, kwds, "kKIII", keywords,
            &styles, &channels,
            &ylen, &xlen,
            &ctlword));

    CHECK_NOTCURSES(ncplane_rounded_box_sized(
        self->ncplane_ptr,
        (uint16_t)styles,
        (uint64_t)channels,
        ylen, xlen,
        ctlword));

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_cells_double_box(NcPlaneObject *Py_UNUSED(self), PyObject *Py_UNUSED(args), PyObject *Py_UNUSED(kwds))
{
    PyErr_SetString(PyExc_NotImplementedError, "TODO when cells are added");
    return NULL;
    // nccells_double_box
}

static PyObject *
NcPlane_double_box(NcPlaneObject *self, PyObject *args, PyObject *kwds)
{
    unsigned long styles = 0;
    unsigned long long channels = 0;
    unsigned ylen = 0, xlen = 0;
    unsigned int ctlword = 0;

    char *keywords[] = {"styles",
                        "channels",
                        "ylen", "xlen",
                        "ctlword",
                        NULL};

    GNU_PY_CHECK_BOOL(
        PyArg_ParseTupleAndKeywords(
            args, kwds, "kKIII", keywords,
            &styles, &channels,
            &ylen, &xlen,
            &ctlword));

    CHECK_NOTCURSES(ncplane_double_box(
        self->ncplane_ptr,
        (uint16_t)styles,
        (uint64_t)channels,
        ylen, xlen,
        ctlword));

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_perimeter_double(NcPlaneObject *self, PyObject *args, PyObject *kwds)
{
    unsigned long styles = 0;
    unsigned long long channels = 0;
    unsigned int ctlword = 0;

    char *keywords[] = {"styles",
                        "channels",
                        "ctlword",
                        NULL};

    GNU_PY_CHECK_BOOL(
        PyArg_ParseTupleAndKeywords(
            args, kwds, "kKI", keywords,
            &styles, &channels,
            &ctlword));

    CHECK_NOTCURSES(ncplane_perimeter_double(
        self->ncplane_ptr,
        (uint16_t)styles,
        (uint64_t)channels,
        ctlword));

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_double_box_sized(NcPlaneObject *self, PyObject *args, PyObject *kwds)
{

    unsigned long styles = 0;
    unsigned long long channels = 0;
    unsigned ylen = 0, xlen = 0;
    unsigned int ctlword = 0;

    char *keywords[] = {"styles",
                        "channels",
                        "ylen", "xlen",
                        "ctlword",
                        NULL};

    GNU_PY_CHECK_BOOL(
        PyArg_ParseTupleAndKeywords(
            args, kwds, "kKIII", keywords,
            &styles, &channels,
            &ylen, &xlen,
            &ctlword));

    CHECK_NOTCURSES(ncplane_double_box_sized(
        self->ncplane_ptr,
        (uint16_t)styles,
        (uint64_t)channels,
        ylen, xlen,
        ctlword));

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_ncvisual_from_plane(NcPlaneObject *Py_UNUSED(self), PyObject *Py_UNUSED(args), PyObject *Py_UNUSED(kwds))
{
    PyErr_SetString(PyExc_NotImplementedError, "TODO when visual is added");
    return NULL;
    // ncvisual_from_plane
}

static PyObject *
NcPlane_as_rgba(NcPlaneObject *Py_UNUSED(self), PyObject *Py_UNUSED(args), PyObject *Py_UNUSED(kwds))
{
    PyErr_SetString(PyExc_NotImplementedError, "TODO when visual is added");
    return NULL;
    // ncplane_as_rgba
}

static PyObject *
NcPlane_reel_create(NcPlaneObject *Py_UNUSED(self), PyObject *Py_UNUSED(args), PyObject *Py_UNUSED(kwds))
{
    PyErr_SetString(PyExc_NotImplementedError, "TODO when NcReel is added");
    return NULL;
    // ncreel_create
}

static PyObject *
NcPlane_greyscale(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    ncplane_greyscale(self->ncplane_ptr);
    Py_RETURN_NONE;
}

static PyObject *
NcPlane_selector_create(NcPlaneObject *Py_UNUSED(self), PyObject *Py_UNUSED(args), PyObject *Py_UNUSED(kwds))
{
    PyErr_SetString(PyExc_NotImplementedError, "TODO when NcSelector is added");
    return NULL;
    // ncselector_create
}

static PyObject *
NcPlane_multiselector_create(NcPlaneObject *Py_UNUSED(self), PyObject *Py_UNUSED(args), PyObject *Py_UNUSED(kwds))
{
    PyErr_SetString(PyExc_NotImplementedError, "TODO when NcSelector is added");
    return NULL;
    // ncmultiselector_create
}

static PyObject *
NcPlane_tree_create(NcPlaneObject *Py_UNUSED(self), PyObject *Py_UNUSED(args), PyObject *Py_UNUSED(kwds))
{
    PyErr_SetString(PyExc_NotImplementedError, "TODO when NcTree is added");
    return NULL;
    // nctree_create
}

static PyObject *
NcPlane_menu_create(NcPlaneObject *Py_UNUSED(self), PyObject *Py_UNUSED(args), PyObject *Py_UNUSED(kwds))
{
    PyErr_SetString(PyExc_NotImplementedError, "TODO when NcMenu is added");
    return NULL;
    // ncmenu_create
}

static PyObject *
NcPlane_progbar_create(NcPlaneObject *Py_UNUSED(self), PyObject *Py_UNUSED(args), PyObject *Py_UNUSED(kwds))
{
    PyErr_SetString(PyExc_NotImplementedError, "TODO when NcProgbar is added");
    return NULL;
    // ncprogbar_create
}

static PyObject *
NcPlane_tabbed_create(NcPlaneObject *Py_UNUSED(self), PyObject *Py_UNUSED(args), PyObject *Py_UNUSED(kwds))
{
    PyErr_SetString(PyExc_NotImplementedError, "TODO when NcTabbed is added");
    return NULL;
    // nctabbed_create
}

static PyObject *
NcPlane_uplot_create(NcPlaneObject *Py_UNUSED(self), PyObject *Py_UNUSED(args), PyObject *Py_UNUSED(kwds))
{
    PyErr_SetString(PyExc_NotImplementedError, "TODO when NcUplot is added");
    return NULL;
    // ncuplot_create
}

static PyObject *
NcPlane_dplot_create(NcPlaneObject *Py_UNUSED(self), PyObject *Py_UNUSED(args), PyObject *Py_UNUSED(kwds))
{
    PyErr_SetString(PyExc_NotImplementedError, "TODO when NcDplot is added");
    return NULL;
    // ncdplot_create
}

static PyObject *
NcPlane_fdplane_create(NcPlaneObject *Py_UNUSED(self), PyObject *Py_UNUSED(args), PyObject *Py_UNUSED(kwds))
{
    PyErr_SetString(PyExc_NotImplementedError, "TODO when NcFdPlane is added");
    return NULL;
    // ncfdplane_create
}

static PyObject *
NcPlane_subproc_createv(NcPlaneObject *Py_UNUSED(self), PyObject *Py_UNUSED(args), PyObject *Py_UNUSED(kwds))
{
    PyErr_SetString(PyExc_NotImplementedError, "TODO when NcFdPlane is added");
    return NULL;
    // ncsubproc_createv
}

static PyObject *
NcPlane_subproc_createvp(NcPlaneObject *Py_UNUSED(self), PyObject *Py_UNUSED(args), PyObject *Py_UNUSED(kwds))
{
    PyErr_SetString(PyExc_NotImplementedError, "TODO when NcFdPlane is added");
    return NULL;
    // ncsubproc_createvp
}

static PyObject *
NcPlane_subproc_createvpe(NcPlaneObject *Py_UNUSED(self), PyObject *Py_UNUSED(args), PyObject *Py_UNUSED(kwds))
{
    PyErr_SetString(PyExc_NotImplementedError, "TODO when NcFdPlane is added");
    return NULL;
    // ncsubproc_createvpe
}

static PyObject *
NcPlane_qrcode(NcPlaneObject *self, PyObject *args)
{
    const char *data = NULL;
    Py_ssize_t len = 0;

    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "s#", &data, &len));

    unsigned ymax = 0, xmax = 0;

    CHECK_NOTCURSES(ncplane_qrcode(self->ncplane_ptr, &ymax, &xmax, (void *)data, (size_t)len));

    return Py_BuildValue("II", &ymax, &xmax);
}

static PyObject *
NcPlane_reader_create(NcPlaneObject *Py_UNUSED(self), PyObject *Py_UNUSED(args), PyObject *Py_UNUSED(kwds))
{
    PyErr_SetString(PyExc_NotImplementedError, "TODO when NcReader is added");
    return NULL;
    // ncreader_create
}

static PyObject *
NcPlane_pile_top(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    PyObject *new_object = NcPlane_Type.tp_alloc((PyTypeObject *)&NcPlane_Type, 0);
    NcPlaneObject *new_plane = (NcPlaneObject *)new_object;
    new_plane->ncplane_ptr = ncpile_top(self->ncplane_ptr);
    return new_object;
}

static PyObject *
NcPlane_pile_bottom(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    PyObject *new_object = NcPlane_Type.tp_alloc((PyTypeObject *)&NcPlane_Type, 0);
    NcPlaneObject *new_plane = (NcPlaneObject *)new_object;
    new_plane->ncplane_ptr = ncpile_bottom(self->ncplane_ptr);
    return new_object;
}

static PyObject *
NcPlane_pile_render(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    ncpile_render(self->ncplane_ptr);
    Py_RETURN_NONE;
}

static PyObject *
NcPlane_pile_rasterize(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    ncpile_rasterize(self->ncplane_ptr);
    Py_RETURN_NONE;
}

static void cleanup_char_buffer(char **buffer_ptr)
{
    if (NULL != buffer_ptr)
    {
        free(*buffer_ptr);
    }
}

static PyObject *
NcPlane_pile_render_to_buffer(NcPlaneObject *self, PyObject *Py_UNUSED(args))
{
    char *buffer __attribute__((cleanup(cleanup_char_buffer))) = NULL;
    size_t buffer_len = 0;

    CHECK_NOTCURSES(ncpile_render_to_buffer(self->ncplane_ptr, &buffer, &buffer_len));

    return PyBytes_FromStringAndSize(buffer, (Py_ssize_t)buffer_len);
}

static void cleanup_file(FILE **file_to_close)
{
    if (NULL != file_to_close)
    {
        fclose(*file_to_close);
    }
}

static PyObject *
NcPlane_pile_render_to_file(NcPlaneObject *self, PyObject *args)
{
    int fd = INT_MAX;
    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "i", &fd));

    FILE *new_render_file __attribute__((cleanup(cleanup_file))) = fdopen(fd, "w");

    if (NULL == new_render_file)
    {
        return PyErr_SetFromErrno(PyExc_RuntimeError);
    }

    CHECK_NOTCURSES(ncpile_render_to_file(self->ncplane_ptr, new_render_file));

    Py_RETURN_NONE;
}

static PyObject *
NcPlane_scrollup(NcPlaneObject *self, PyObject *args)
{
    int r;
    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "i", &r));
    CHECK_NOTCURSES(ncplane_scrollup(self->ncplane_ptr, r));
    Py_RETURN_NONE;
}

/*
static PyObject *
NcPlane_(NcPlaneObject *self, PyObject *args)
{
    GNU_PY_CHECK_BOOL(PyArg_ParseTuple(args, "", ));
}
*/

static PyMethodDef NcPlane_methods[] = {
    {"create", (void *)Ncplane_create, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Create a new ncplane bound to plane 'n', at the offset 'y'x'x' (relative to the origin of 'n') and the specified size. The number of 'rows' and 'cols' must both be positive. This plane is initially at the top of the z-buffer, as if ncplane_move_top() had been called on it. The void* 'userptr' can be retrieved (and reset) later. A 'name' can be set, used in debugging.")},
    {"destroy", (PyCFunction)NcPlane_destroy, METH_NOARGS, "Destroy the plane."},

    {"notcurses", (PyCFunction)NcPlane_notcurses, METH_NOARGS, PyDoc_STR("Extract the Notcurses context to which this plane is attached.")},
    {"dim_yx", (PyCFunction)NcPlane_dim_yx, METH_NOARGS, PyDoc_STR("Return the dimensions of this ncplane.")},
    {"dim_x", (PyCFunction)NcPlane_dim_x, METH_NOARGS, PyDoc_STR("Return X dimension of this ncplane.")},
    {"dim_y", (PyCFunction)NcPlane_dim_y, METH_NOARGS, PyDoc_STR("Return Y dimension of this ncplane.")},
    {"pixel_geom", (PyCFunction)NcPlane_pixel_geom, METH_NOARGS, PyDoc_STR("Retrieve pixel geometry for the display region ('pxy', 'pxx'), each cell ('celldimy', 'celldimx'), and the maximum displayable bitmap ('maxbmapy', 'maxbmapx'). Note that this will call notcurses_check_pixel_support(), possibly leading to an interrogation of the terminal. If bitmaps are not supported, 'maxbmapy' and 'maxbmapx' will be 0. Any of the geometry arguments may be NULL.")},

    {"set_resizecb", (PyCFunction)NcPlane_set_resizecb, METH_VARARGS, PyDoc_STR("Replace the ncplane's existing resizecb with 'resizecb' (which may be NULL). The standard plane's resizecb may not be changed.")},
    {"reparent", (PyCFunction)NcPlane_reparent, METH_VARARGS, PyDoc_STR("Plane 'n' will be unbound from its parent plane, and will be made a bound child of 'newparent'.")},
    {"reparent_family", (PyCFunction)NcPlane_reparent_family, METH_VARARGS, PyDoc_STR("The same as reparent(), except any planes bound to 'n' come along with it to its new destination.")},
    {"dup", (PyCFunction)NcPlane_dup, METH_NOARGS, PyDoc_STR("Duplicate an existing ncplane.")},

    {"translate", (void *)NcPlane_translate, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("provided a coordinate relative to the origin of 'src', map it to the same absolute coordinate relative to the origin of 'dst'.")},
    {"translate_abs", (PyCFunction)NcPlane_translate_abs, METH_VARARGS, PyDoc_STR("Fed absolute 'y'/'x' coordinates, determine whether that coordinate is within the ncplane.")},
    {"set_scrolling", (PyCFunction)NcPlane_set_scrolling, METH_VARARGS, PyDoc_STR("All planes are created with scrolling disabled. Returns true if scrolling was previously enabled, or false if it was disabled.")},

    {"resize", (void *)NcPlane_resize, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Resize the specified ncplane.")},
    {"resize_simple", (PyCFunction)NcPlane_resize_simple, METH_VARARGS, PyDoc_STR("Resize the plane, retaining what data we can (everything, unless we're shrinking in some dimension). Keep the origin where it is.")},

    {"set_base_cell", (PyCFunction)NcPlane_set_base_cell, METH_VARARGS, PyDoc_STR("Set the ncplane's base nccell to 'c'.")},
    {"set_base", (void *)NcPlane_set_base, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Set the ncplane's base nccell.")},
    {"base", (PyCFunction)NcPlane_base, METH_NOARGS, PyDoc_STR("Extract the ncplane's base nccell.")},

    {"move_yx", (PyCFunction)NcPlane_move_yx, METH_VARARGS, PyDoc_STR("Move this plane relative to the standard plane, or the plane to which it is bound (if it is bound to a plane).")},
    {"yx", (PyCFunction)NcPlane_yx, METH_NOARGS, PyDoc_STR("Get the origin of plane relative to its bound plane, or pile.")},
    {"y", (PyCFunction)NcPlane_y, METH_NOARGS, PyDoc_STR("Get the Y origin of plane relative to its bound plane, or pile.")},
    {"x", (PyCFunction)NcPlane_x, METH_NOARGS, PyDoc_STR("Get the X origin of plane relative to its bound plane, or pile.")},
    {"abs_yx", (PyCFunction)NcPlane_abs_yx, METH_NOARGS, PyDoc_STR("Get the origin of plane relative to its pile.")},
    {"abs_y", (PyCFunction)NcPlane_abs_y, METH_NOARGS, PyDoc_STR("Get the Y origin of plane relative to its pile.")},
    {"abs_x", (PyCFunction)NcPlane_abs_x, METH_NOARGS, PyDoc_STR("Get the X origin of plane relative to its pile.")},

    {"parent", (PyCFunction)NcPlane_parent, METH_NOARGS, PyDoc_STR("Get the plane to which the plane is bound or None if plane does not have parent.")},
    {"descendant_p", (PyCFunction)NcPlane_descendant_p, METH_VARARGS, PyDoc_STR("Return True if plane is a proper descendent of passed 'ancestor' plane.")},

    {"move_top", (PyCFunction)NcPlane_move_top, METH_NOARGS, PyDoc_STR("Splice ncplane out of the z-buffer, and reinsert it at the top.")},
    {"move_bottom", (PyCFunction)NcPlane_move_bottom, METH_NOARGS, PyDoc_STR("Splice ncplane out of the z-buffer, and reinsert it at the bottom.")},
    {"move_above", (PyCFunction)NcPlane_move_above, METH_VARARGS, PyDoc_STR("Splice ncplane out of the z-buffer, and reinsert it above passed plane.")},
    {"move_below", (PyCFunction)NcPlane_move_below, METH_VARARGS, PyDoc_STR("Splice ncplane out of the z-buffer, and reinsert it bellow passed plane.")},
    {"below", (PyCFunction)NcPlane_below, METH_NOARGS, PyDoc_STR("Return the plane below this one, or None if this is at the bottom.")},
    {"above", (PyCFunction)NcPlane_above, METH_NOARGS, PyDoc_STR("Return the plane above this one, or None if this is at the top.")},

    {"rotate_cw", (PyCFunction)NcPlane_rotate_cw, METH_NOARGS, PyDoc_STR("Rotate the plane π/2 radians clockwise.")},
    {"rotate_ccw", (PyCFunction)NcPlane_rotate_ccw, METH_NOARGS, PyDoc_STR("Rotate the plane π/2 radians counterclockwise.")},

    {"at_cursor", (PyCFunction)NcPlane_at_cursor, METH_NOARGS, PyDoc_STR("Retrieve the current contents of the cell under the cursor.")},
    {"at_cursor_cell", (PyCFunction)NcPlane_at_cursor_cell, METH_NOARGS, PyDoc_STR("Retrieve the current contents of the cell under the cursor.")},
    {"at_yx", (PyCFunction)NcPlane_at_yx, METH_VARARGS, PyDoc_STR("Retrieve the current contents of the specified cell.")},
    {"at_yx_cell", (PyCFunction)NcPlane_at_yx_cell, METH_VARARGS, PyDoc_STR("Retrieve the current contents of the specified cell")},
    {"contents", (void *)NcPlane_contents, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Create a flat string from the EGCs of the selected region of the ncplane.")},

    {"center_abs", (PyCFunction)NcPlane_center_abs, METH_NOARGS, PyDoc_STR("Return the plane center absolute coordiantes.")},

    {"halign", (PyCFunction)NcPlane_halign, METH_VARARGS, PyDoc_STR("Return the column at which cols ought start in order to be aligned.")},
    {"valign", (PyCFunction)NcPlane_valign, METH_VARARGS, PyDoc_STR("Return the row at which rows ought start in order to be aligned.")},

    {"cursor_move_yx", (PyCFunction)NcPlane_cursor_move_yx, METH_VARARGS, PyDoc_STR("Move the cursor to the specified position (the cursor needn't be visible).")},
    {"home", (PyCFunction)NcPlane_home, METH_NOARGS, PyDoc_STR("Move the cursor to 0, 0.")},
    {"cursor_yx", (PyCFunction)NcPlane_cursor_yx, METH_NOARGS, PyDoc_STR("Get the current position of the cursor within plane.")},

    {"channels", (PyCFunction)NcPlane_channels, METH_NOARGS, PyDoc_STR("Get the current channels or attribute word.")},
    {"styles", (PyCFunction)NcPlane_styles, METH_NOARGS, PyDoc_STR("Return the current styling for this ncplane.")},

    {"putc_yx", (PyCFunction)NcPlane_putc_yx, METH_VARARGS, PyDoc_STR("Replace the cell at the specified coordinates with the provided cell.")},
    {"putc", (PyCFunction)NcPlane_putc, METH_VARARGS, PyDoc_STR("Replace cell at the current cursor location.")},

    {"putchar_yx", (PyCFunction)NcPlane_putchar_yx, METH_VARARGS, PyDoc_STR("Replace the cell at the specified coordinates with the provided 7-bit char.")},
    {"putchar", (PyCFunction)NcPlane_putchar, METH_VARARGS, PyDoc_STR("Replace the cell at the current cursor location.")},
    {"putchar_stained", (PyCFunction)NcPlane_putchar_stained, METH_VARARGS, PyDoc_STR("Replace the EGC underneath us, but retain the styling.")},

    {"putegc_yx", (PyCFunction)NcPlane_putegc_yx, METH_VARARGS, PyDoc_STR("Replace the cell at the specified coordinates with the provided EGC.")},
    {"putegc", (PyCFunction)NcPlane_putegc, METH_VARARGS, PyDoc_STR("Replace the cell at the current cursor location with the provided EGC")},
    {"putegc_stained", (PyCFunction)NcPlane_putegc_stained, METH_VARARGS, PyDoc_STR("Replace the EGC underneath us, but retain the styling.")},

    {"putstr_yx", (PyCFunction)NcPlane_putstr_yx, METH_VARARGS, PyDoc_STR("Write a series of EGCs to the location, using the current style.")},
    {"putstr", (PyCFunction)NcPlane_putstr, METH_VARARGS, PyDoc_STR("Write a series of EGCs to the current location, using the current style.")},
    {"putstr_aligned", (PyCFunction)NcPlane_putstr_aligned, METH_VARARGS, PyDoc_STR("Write a series of EGCs to the current location, using the alignment.")},
    {"putstr_stained", (PyCFunction)NcPlane_putstr_stained, METH_VARARGS, PyDoc_STR("Replace a string's worth of glyphs at the current cursor location, but retain the styling.")},
    {"putnstr_yx", (PyCFunction)NcPlane_putnstr_yx, METH_VARARGS, PyDoc_STR("Write a series of EGCs to the location, using the current style.")},
    {"putnstr", (PyCFunction)NcPlane_putnstr, METH_VARARGS, PyDoc_STR("Write a series of EGCs to the current location, using the current style.")},
    {"putnstr_aligned", (PyCFunction)NcPlane_putnstr_aligned, METH_VARARGS, PyDoc_STR("Write a series of EGCs to the current location, using the alignment.")},

    {"puttext", (PyCFunction)NcPlane_puttext, METH_VARARGS, PyDoc_STR("Write the specified text to the plane, breaking lines sensibly, beginning at the specified line.")},

    {"box", (void *)NcPlane_box, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Draw a box with its upper-left corner at the current cursor position.")},
    {"box_sized", (void *)NcPlane_box_sized, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Draw a box with its upper-left corner at the current cursor position, having dimensions.")},
    {"perimeter", (void *)NcPlane_perimeter, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Draw a perimeter with its upper-left corner at the current cursor position")},
    {"polyfill_yx", (void *)NcPlane_polyfill_yx, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Starting at the specified coordinate, if its glyph is different from that of is copied into it, and the original glyph is considered the fill target.")},

    {"gradient", (void *)NcPlane_gradient, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Draw a gradient with its upper-left corner at the current cursor position.")},
    {"gradient2x1", (void *)NcPlane_gradient2x1, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("NcPlane.gradent_sized() meets NcPlane.highgradient().")},

    {"format", (PyCFunction)NcPlane_format, METH_VARARGS, PyDoc_STR("Set the given style throughout the specified region, keeping content and attributes unchanged. Returns the number of cells set.")},
    {"stain", (void *)NcPlane_stain, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Set the given style throughout the specified region, keeping content and attributes unchanged. Returns the number of cells set.")},

    {"mergedown_simple", (PyCFunction)NcPlane_mergedown_simple, METH_VARARGS, PyDoc_STR("Merge the ncplane down onto the passed ncplane.")},
    {"mergedown", (void *)NcPlane_mergedown, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Merge with parameters the ncplane down onto the passed ncplane.")},
    {"erase", (PyCFunction)NcPlane_erase, METH_NOARGS, PyDoc_STR("Erase every cell in the ncplane.")},

    {"bchannel", (PyCFunction)NcPlane_bchannel, METH_NOARGS, PyDoc_STR("Extract the 32-bit working background channel from an ncplane.")},
    {"fchannel", (PyCFunction)NcPlane_fchannel, METH_NOARGS, PyDoc_STR("Extract the 32-bit working foreground channel from an ncplane.")},
    {"set_channels", (PyCFunction)NcPlane_set_channels, METH_VARARGS, PyDoc_STR("Set both foreground and background channels of the plane.")},

    {"set_styles", (PyCFunction)NcPlane_set_styles, METH_VARARGS, PyDoc_STR("Set the specified style bits for the plane, whether they're actively supported or not.")},
    {"on_styles", (PyCFunction)NcPlane_on_styles, METH_VARARGS, PyDoc_STR("Add the specified styles to the ncplane's existing spec.")},
    {"off_styles", (PyCFunction)NcPlane_off_styles, METH_VARARGS, PyDoc_STR("Remove the specified styles from the ncplane's existing spec.")},

    {"fg_rgb", (PyCFunction)NcPlane_fg_rgb, METH_NOARGS, PyDoc_STR("Extract 24 bits of working foreground RGB from the plane, shifted to LSBs.")},
    {"bg_rgb", (PyCFunction)NcPlane_bg_rgb, METH_NOARGS, PyDoc_STR("Extract 24 bits of working background RGB from the plane, shifted to LSBs.")},
    {"fg_alpha", (PyCFunction)NcPlane_fg_alpha, METH_NOARGS, PyDoc_STR("Extract 2 bits of foreground alpha from the plane, shifted to LSBs.")},
    {"fg_default_p", (PyCFunction)NcPlane_fg_default_p, METH_NOARGS, PyDoc_STR("Is the plane's foreground using the \"default foreground color\"?")},
    {"bg_alpha", (PyCFunction)NcPlane_bg_alpha, METH_NOARGS, PyDoc_STR("Extract 2 bits of background alpha from the plane, shifted to LSBs.")},
    {"bg_default_p", (PyCFunction)NcPlane_bg_default_p, METH_NOARGS, PyDoc_STR("Is the plane's background using the \"default background color\"?")},

    {"fg_rgb8", (PyCFunction)NcPlane_fg_rgb8, METH_NOARGS, PyDoc_STR("Extract 24 bits of foreground RGB from the plane, split into components.")},
    {"bg_rgb8", (PyCFunction)NcPlane_bg_rgb8, METH_NOARGS, PyDoc_STR("Extract 24 bits of background RGB from the plane, split into components.")},
    {"set_fchannel", (PyCFunction)NcPlane_set_fchannel, METH_VARARGS, PyDoc_STR("Set an entire foreground channel of the plane, return new channels.")},
    {"set_bchannel", (PyCFunction)NcPlane_set_bchannel, METH_VARARGS, PyDoc_STR("Set an entire background channel of the plane, return new channels.")},

    {"set_fg_rgb8", (PyCFunction)NcPlane_set_fg_rgb8, METH_VARARGS, PyDoc_STR("Set the current foreground color using RGB specifications.")},
    {"set_bg_rgb8", (PyCFunction)NcPlane_set_bg_rgb8, METH_VARARGS, PyDoc_STR("Set the current background color using RGB specifications.")},
    {"set_bg_rgb8_clipped", (PyCFunction)NcPlane_set_bg_rgb8_clipped, METH_VARARGS, PyDoc_STR("Set the current foreground color using RGB specifications but clipped to [0..255].")},
    {"set_fg_rgb8_clipped", (PyCFunction)NcPlane_set_fg_rgb8_clipped, METH_VARARGS, PyDoc_STR("Set the current background color using RGB specifications but clipped to [0..255].")},
    {"set_fg_rgb", (PyCFunction)NcPlane_set_fg_rgb, METH_VARARGS, PyDoc_STR("Set the current foreground color using channel.")},
    {"set_bg_rgb", (PyCFunction)NcPlane_set_bg_rgb, METH_VARARGS, PyDoc_STR("Set the current background color using channel.")},

    {"set_fg_default", (PyCFunction)NcPlane_set_fg_default, METH_NOARGS, PyDoc_STR("Use the default color for the foreground.")},
    {"set_bg_default", (PyCFunction)NcPlane_set_bg_default, METH_NOARGS, PyDoc_STR("Use the default color for the background.")},
    {"set_fg_palindex", (PyCFunction)NcPlane_set_fg_palindex, METH_VARARGS, PyDoc_STR("Set the ncplane's foreground palette index.")},
    {"set_bg_palindex", (PyCFunction)NcPlane_set_bg_palindex, METH_VARARGS, PyDoc_STR("Set the ncplane's background palette index.")},
    {"set_fg_alpha", (PyCFunction)NcPlane_set_fg_alpha, METH_VARARGS, PyDoc_STR("Set the foreground alpha parameters for the plane.")},
    {"set_bg_alpha", (PyCFunction)NcPlane_set_bg_alpha, METH_VARARGS, PyDoc_STR("Set the background alpha parameters for the plane.")},

    {"fadeout", (PyCFunction)NcPlane_fadeout, METH_VARARGS, PyDoc_STR("Fade the ncplane out over the provided time, calling 'fader' at each iteration.")},
    {"fadein", (PyCFunction)NcPlane_fadein, METH_VARARGS, PyDoc_STR("Fade the ncplane in over the specified time. ")},
    {"fade_setup", (PyCFunction)NcPlane_fade_setup, METH_VARARGS, PyDoc_STR("Create NcFadeCtx.")},
    {"fadeout_iteration", (PyCFunction)NcPlane_fadeout_iteration, METH_VARARGS, PyDoc_STR("TODO")},
    {"fadein_iteration", (PyCFunction)NcPlane_fadein_iteration, METH_VARARGS, PyDoc_STR("TODO")},
    {"pulse", (PyCFunction)NcPlane_pulse, METH_VARARGS, PyDoc_STR("Pulse the plane in and out until the callback returns non-zero.")},

    {"cells_load_box", (void *)NcPlane_cells_load_box, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Load up six cells with the EGCs necessary to draw a box.")},
    {"cells_rounded_box", (void *)NcPlane_cells_rounded_box, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Load up six cells with the EGCs necessary to draw a round box.")},
    {"perimeter_rounded", (void *)NcPlane_perimeter_rounded, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Draw a perimeter around plane.")},

    {"rounded_box_sized", (void *)NcPlane_rounded_box_sized, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Draw a round box around plane.")},
    {"cells_double_box", (void *)NcPlane_cells_double_box, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Draw a double box with cells.")},
    {"double_box", (void *)NcPlane_double_box, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Draw a double box.")},
    {"perimeter_double", (void *)NcPlane_perimeter_double, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Draw a double perimeter.")},
    {"double_box_sized", (void *)NcPlane_double_box_sized, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Draw a double box sized.")},

    {"ncvisual_from_plane", (void *)NcPlane_ncvisual_from_plane, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Promote an plane to an NcVisual.")},
    {"as_rgba", (void *)NcPlane_as_rgba, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Create an RGBA flat array from the selected region of the plane.")},
    {"reel_create", (void *)NcPlane_reel_create, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Take over the plane and use it to draw a reel.")},
    {"greyscale", (PyCFunction)NcPlane_greyscale, METH_NOARGS, PyDoc_STR("Convert the plane's content to greyscale.")},
    {"selector_create", (void *)NcPlane_selector_create, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Create NcSelector.")},
    {"multiselector_create", (void *)NcPlane_multiselector_create, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Create MultiSelector.")},
    {"tree_create", (void *)NcPlane_tree_create, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Create NcTree.")},
    {"menu_create", (void *)NcPlane_menu_create, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Create NcMenu.")},
    {"progbar_create", (void *)NcPlane_progbar_create, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Create NcProgbar.")},
    {"tabbed_create", (void *)NcPlane_tabbed_create, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Create NcTabbed.")},
    {"uplot_create", (void *)NcPlane_uplot_create, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Create NcUplot.")},
    {"dplot_create", (void *)NcPlane_dplot_create, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Create NcDplot.")},
    {"fdplane_create", (void *)NcPlane_fdplane_create, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Create NcFdPlane.")},

    {"subproc_createv", (void *)NcPlane_subproc_createv, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Create subprocess plane.")},
    {"subproc_createvp", (void *)NcPlane_subproc_createvp, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Create subprocess plane.")},
    {"subproc_createvpe", (void *)NcPlane_subproc_createvpe, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Create subprocess plane.")},

    {"qrcode", (void *)NcPlane_qrcode, METH_VARARGS, PyDoc_STR("Create QR code, return y and x size.")},

    {"reader_create", (void *)NcPlane_reader_create, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Create NcReader.")},

    {"pile_top", (PyCFunction)NcPlane_pile_top, METH_NOARGS, PyDoc_STR("Return the topmost plane of the pile.")},
    {"pile_bottom", (PyCFunction)NcPlane_pile_bottom, METH_NOARGS, PyDoc_STR("Return the bottommost plane of the pile.")},
    {"pile_render", (PyCFunction)NcPlane_pile_render, METH_NOARGS, PyDoc_STR("Renders the pile of which plane is a part.")},
    {"pile_rasterize", (PyCFunction)NcPlane_pile_rasterize, METH_NOARGS, PyDoc_STR("Make the physical screen match the last rendered frame from the pile.")},

    {"pile_render_to_buffer", (PyCFunction)NcPlane_pile_render_to_buffer, METH_NOARGS, "Perform the rendering and rasterization portion of notcurses_render() and write it to bytes object instead of terminal."},
    {"render_to_file", (PyCFunction)NcPlane_pile_render_to_file, METH_VARARGS, "Write the last rendered frame, in its entirety, to file descriptor. If render() has not yet been called, nothing will be written."},

    {"scrollup", (PyCFunction)NcPlane_scrollup, METH_VARARGS, "Effect scroll events on the plane."},

    //  {"", (PyCFunction) NULL, METH_VARARGS, PyDoc_STR("")},
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
    .tp_methods = NcPlane_methods,
};

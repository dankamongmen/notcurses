#include <Python.h>
#include <stdint.h>

#include "notcurses-python.h"

// TODO: alpha flags on channels
// TODO: indexed color channels
// TODO: perimeter function
// TODO: rationalize coordinate / size args
// TODO: provide a way to set channels for each corner
// TODO: docstrings
// TODO: unit tests

/*
 * Converts borrowed `obj` to a channel value in `channel`.  Returns 1 on
 * success.
 */
static int
to_channel(PyObject* obj, uint32_t* channel) {
  // None → default color.
  if (obj == Py_None) {
    *channel = 0;
    return 1;
  }

  // A single long → channel value.
  long long const value = PyLong_AsLongLong(obj);
  if (PyErr_Occurred())
    PyErr_Clear();
    // And fall through.
  else if (value & ~0xffffffffll) {
    PyErr_Format(PyExc_ValueError, "invalid channel: %lld", value);
    return 0;
  }
  else {
    *channel = (uint32_t) value;
    return 1;
  }

  PyErr_Format(PyExc_TypeError, "not a channel: %R", obj);
  return 0;
}

static PyObject*
pync_meth_box(PyObject* Py_UNUSED(self), PyObject* args, PyObject* kwargs) {
  static char* keywords[] = {
    "plane", "ystop", "xstop", "box_chars", "styles", "fg", "bg",
    "ctlword", NULL
  };
  NcPlaneObject* plane_arg;
  unsigned ystop;
  unsigned xstop;
  const char* box_chars = NCBOXASCII;
  uint16_t styles = 0;
  PyObject* fg_arg = 0;
  PyObject* bg_arg = 0;
  unsigned ctlword = 0;
  if (!PyArg_ParseTupleAndKeywords(
        args, kwargs, "O!II|s$HOOI:box", keywords,
        &NcPlane_Type, &plane_arg,
        &ystop, &xstop, &box_chars,
        &styles, &fg_arg, &bg_arg, &ctlword))
    return NULL;

  uint32_t fg;
  if (!to_channel(fg_arg, &fg))
    return NULL;
  uint32_t bg;
  if (!to_channel(bg_arg, &bg))
    return NULL;
  uint64_t const channels = (uint64_t) fg << 32 | bg;

  struct ncplane* const plane = plane_arg->ncplane_ptr;

  if (!notcurses_canutf8(ncplane_notcurses(plane)))
    // No UTF-8 support; force ASCII.
    box_chars = NCBOXASCII;

  int ret;
  nccell ul = NCCELL_TRIVIAL_INITIALIZER;
  nccell ur = NCCELL_TRIVIAL_INITIALIZER;
  nccell ll = NCCELL_TRIVIAL_INITIALIZER;
  nccell lr = NCCELL_TRIVIAL_INITIALIZER;
  nccell hl = NCCELL_TRIVIAL_INITIALIZER;
  nccell vl = NCCELL_TRIVIAL_INITIALIZER;
  ret = nccells_load_box(
    plane, styles, channels, &ul, &ur, &ll, &lr, &hl, &vl, box_chars);
  if (ret == -1) {
    PyErr_Format(PyExc_RuntimeError, "nccells_load_box returned %i", ret);
    return NULL;
  }

  ret = ncplane_box(plane, &ul, &ur, &ll, &lr, &hl, &vl, ystop, xstop, ctlword);
  if (ret < 0)
    PyErr_Format(PyExc_RuntimeError, "nplane_box returned %i", ret);

  nccell_release(plane, &ul);
  nccell_release(plane, &ur);
  nccell_release(plane, &ll);
  nccell_release(plane, &lr);
  nccell_release(plane, &hl);
  nccell_release(plane, &vl);

  if (ret < 0)
    return NULL;
  else
    Py_RETURN_NONE;
}

static PyObject*
pync_meth_rgb(PyObject* Py_UNUSED(self), PyObject* args) {
  int r;
  int g;
  int b;
  if (!PyArg_ParseTuple(args, "iii", &r, &g, &b))
    return NULL;

  if ((r & ~0xff) == 0 && (g & ~0xff) == 0 && (b & ~0xff) == 0)
    return PyLong_FromLong(
      0x40000000u | (uint32_t) r << 16 | (uint32_t) g <<  8 | (uint32_t) b);
  else {
    PyErr_Format(PyExc_ValueError, "invalid rgb: (%d, %d, %d)", r, g, b);
    return NULL;
  }
}

struct PyMethodDef pync_methods[] = {
    {
      "box",
      (void*) pync_meth_box,
      METH_VARARGS | METH_KEYWORDS,
      "FIXME: Docs."
    },
    {
      "rgb",
      (void*) pync_meth_rgb,
      METH_VARARGS,
      "FIXME: Docs."
    },
    {NULL, NULL, 0, NULL}
};


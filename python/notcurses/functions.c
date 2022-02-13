#include <Python.h>
#include <stdint.h>

#include "notcurses-python.h"

// TODO: function to construct channels: channel(None | pindex | color, alpha=0)
// TODO: split channels into two args
// TODO: perimeter version
// TODO: rationalize coordinate / size args
// TODO: provide a way to set channels for each corne
// TODO: docstring
// TODO: test

static PyObject*
pync_meth_box(PyObject* Py_UNUSED(self), PyObject* args, PyObject* kwargs) {
  static char* keywords[] = {
    "plane", "ystop", "xstop", "y", "x", "box_chars", "styles", "channels",
    "ctlword", NULL
  };
  NcPlaneObject* plane_arg;
  unsigned ystop;
  unsigned xstop;
  int y = -1;
  int x = -1;
  const char* box_chars = NCBOXASCII;
  uint16_t styles = 0;
  uint64_t channels = 0;
  unsigned ctlword = 0;
  if (!PyArg_ParseTupleAndKeywords(
        args, kwargs, "O!II|iis$HKI:box", keywords,
        &NcPlane_Type, &plane_arg,
        &ystop, &xstop, &y, &x, &box_chars, &styles, &channels, &ctlword))
    return NULL;

  if (!notcurses_canutf8(ncplane_notcurses(plane)))
    // No UTF-8 support; force ASCII.
    box_chars = NCBOXASCII;

  struct ncplane* const plane = plane_arg->ncplane_ptr;

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

  if (y != 1 || x != -1) {
    ret = ncplane_cursor_move_yx(plane, y, x);
    if (ret < 0) {
      PyErr_Format(PyExc_RuntimeError, "ncplane_cursor_move_yx returned %i", ret);
      goto done;
    }
  }

  ret = ncplane_box(plane, &ul, &ur, &ll, &lr, &hl, &vl, ystop, xstop, ctlword);
  if (ret < 0)
    PyErr_Format(PyExc_RuntimeError, "nplane_box returned %i", ret);

done:
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

struct PyMethodDef pync_methods[] = {
    {
        "box",
        (void*) pync_meth_box,
        METH_VARARGS | METH_KEYWORDS,
        "FIXME: Docs."
    },
    {NULL, NULL, 0, NULL}
};


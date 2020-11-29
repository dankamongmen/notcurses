## Tools

The library right now fully supports the [type hints](https://docs.python.org/3/library/typing.html).
This is a big advantage as it allows the static checking of the code as well as auto-completions.

It is recommended to run a *mypy* type checker after any changes to code: `mypy --strict --ignore-missing-imports ./notcurses/`
This will alert if there are any issues with type checking.

## Structure

The functions and classes starting with underscore_ mean it is internal and should not be used by end user.

### notcurses/_notcurses.c

This file contains the C code for the loadable module.

The C code is being kept extremely simple to have a lot of flexibility
inside Python code.

Recommended reading:
    https://docs.python.org/3/extending/index.html
    https://docs.python.org/3/c-api/index.html

#### Class definitions

```
typedef struct
{
    PyObject_HEAD;
    struct ncplane *ncplane_ptr;
} NcPlaneObject;

static PyMethodDef NcPlane_methods[] = {
    {NULL, NULL, 0, NULL},
};

static PyTypeObject NcPlaneType = {
    PyVarObject_HEAD_INIT(NULL, 0)
        .tp_name = "notcurses._notcurses._NcPlane",
    .tp_doc = "Notcurses Plane",
    .tp_basicsize = sizeof(NcPlaneObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
    .tp_init = NULL,
    .tp_methods = NcPlane_methods,
};
```

First is a typedef which contains the data that Python object will hold.
Whose fields are not directly accessible unless specified in PyMemberDef structure.

Second is the member definition. Its empty in this case.

Third the definition of the type.

If you want to add a new type you need all three field with new unique names.
Also you need to initialize the type in the `PyInit__notcurses` function.


#### Function definitions

```
static PyObject *
_nc_direct_get_dim_x(PyObject *self, PyObject *args)
{
    NcDirectObject *ncdirect_ref = NULL;
    if (!PyArg_ParseTuple(args, "O!", &NcDirectType, &ncdirect_ref))
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to parse _ncdirect_get_dim_x arguments");
        return NULL;
    }
    if (ncdirect_ref != NULL)
    {
        return PyLong_FromLong(ncdirect_dim_x(ncdirect_ref->ncdirect_ptr));
    }
    else
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to acquire NcDirectObject");
        return NULL;
    }
}
```

The functions return the pointers to PyObject.

`PyArg_ParseTuple` parses the arguments passed. This is position only function meaning keywords are not supported. There are functions to parse keywords and position arguments but for the sake of simplicity its not being used.

https://docs.python.org/3/c-api/arg.html

If you want to add a new function you need to add it to `NotcursesMethods` struct.

```
static PyMethodDef NotcursesMethods[] = {
    {"_nc_direct_init", (PyCFunction)_nc_direct_init, METH_VARARGS, NULL},
    ...
};
```

#### Module init funciton

Last is `PyInit__notcurses` function which initializes the module.

```
if (PyType_Ready(&NcDirectType) < 0)
    return NULL;
```

These class make sure the type is ready. Should be called for each type.

```
Py_INCREF(&NcInputType);
if (PyModule_AddObject(py_module, "_NcInput", (PyObject *)&NcInputType) < 0)
{
    Py_DECREF(&NcInputType);
    Py_DECREF(py_module);
    return NULL;
}
```

These calls add the objects to the module.

```
constants_control_value |= PyModule_AddIntMacro(py_module, NCKEY_INVALID);
```

These calls initialize the constants of the module.


The module can be compiled independently from setuptools by running `gcc -fPIC -Wall --shared -I/usr/include/python3.8 -lnotcurses ./notcurses/_notcurses.c -o ./notcurses/_notcurses.so`

### notcurses/_notcurses.py

This is stub file for python type checkers and documentation generators.

It should closely follow the `_notcurses.c` file.

For example, `_nc_plane_dimensions_yx`:

C code:
```
static PyObject *
_nc_plane_dimensions_yx(PyObject *self, PyObject *args)
{
    NcPlaneObject *nc_plane_ref = NULL;
    int y_dim = 0;
    int x_dim = 0;
    if (!PyArg_ParseTuple(args, "O!",
                          &NcPlaneType, &nc_plane_ref))
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to parse _nc_plane_set_foreground_rgb arguments");
        return NULL;
    }

    if (nc_plane_ref == NULL)
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to acquire NcPlaneObject");
        return NULL;
    }

    ncplane_dim_yx(nc_plane_ref->ncplane_ptr, &y_dim, &x_dim);
    return PyTuple_Pack(2, PyLong_FromLong(y_dim), PyLong_FromLong(x_dim));
}
```

Python prototype:
```
def _nc_plane_dimensions_yx(nc_plane: _NcPlane, /) -> Tuple[int, int]:
    ...
```

`/` means the function only accepts positional arguments
`-> Tuple[int, int]` means the function returns the tuple of two ints
`...` the function body is not defined as C code has the actual function body

### notcurses/notcurses.py

This file contains the actual python classes that are available to user.

It imports raw functions from `_notcurses` and wraps them in nice and beautiful objects.

For example, `NcPlane.put_lines` takes a line iterators and puts over the plane. Such function does not exist in original API.

To understand how to create beautiful python classes I recommend reading zen of python: https://en.wikipedia.org/wiki/Zen_of_Python

### notcurses/__init__.py

This file defines the exports of the module.
It should only import from `notcurses.py` and add them to `__all__` attribute.

#include <Python.h>

static PyObject*
pygmediadb_add_entry (PyObject *self, PyObject *args)
{
    int a, b;

    if (!PyArg_ParseTuple (args, "ii", &a, &b)) {
        return NULL;
    }

    return Py_BuildValue ("i", a + b);
}

static PyMethodDef gmediadb_methods[] = {
    { "add_entry", (PyCFunction) pygmediadb_add_entry, METH_VARARGS, NULL },
};

PyMODINIT_FUNC
initgmediadb ()
{
    Py_InitModule3 ("gmediadb", gmediadb_methods, "GMediaDB database module");
}

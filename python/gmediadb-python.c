#include <Python.h>

#include <glib.h>

#include "gmediadb-GMediaDB.h"

static PyMethodDef gmediadb_methods[] = {
    { NULL }  /* Sentinel */
};

#ifndef PyMODINIT_FUNC /* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif

PyMODINIT_FUNC
initgmediadb(void)
{
    PyObject* m;

    g_type_init ();

    GMediaDBType.tp_new = PyType_GenericNew;
    if (PyType_Ready (&GMediaDBType) < 0)
        return;

    m = Py_InitModule3 ("gmediadb", gmediadb_methods,
                       "Example module that creates an extension type.");

    Py_INCREF (&GMediaDBType);
    PyModule_AddObject (m, "GMediaDB", (PyObject*) &GMediaDBType);
}

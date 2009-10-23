#include "gmediadb-GMediaDB.h"

int
GMediaDB_init (gmediadb_GMediaDB *self, PyObject *args, PyObject *kwds)
{
    PyObject *name;

    static char *kwlist[] = { "type" , NULL };

    if (! PyArg_ParseTupleAndKeywords (args, kwds, "|O", kwlist, &name)) {
        return -1;
    }

    if (name) {
        self->db = gmediadb_new (PyString_AsString (name));
    }
}

void
GMediaDB_dealloc (gmediadb_GMediaDB *self)
{
    g_object_unref (self->db);
}

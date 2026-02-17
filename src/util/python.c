/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "util/python.h"

#include <limits.h>

void python_tp_dealloc(PyObject *self) {
    PyTypeObject *tp = Py_TYPE(self);

    if (PyType_HasFeature(tp, Py_TPFLAGS_HAVE_GC)) {
        PyObject_GC_UnTrack(self);
    }

    /* Clear Python references from the object. */
    inquiry tp_clear = tp->tp_clear;
    if (tp_clear != NULL && tp_clear(self) < 0) {
        PyErr_WriteUnraisable(self);
    }

    /* Deallocate the object memory. */
    freefunc tp_free = tp->tp_free;
    assert(tp_free != NULL);
    tp_free(self);

    /* Heap types hold a reference to their type. Remove it here. */
    if (PyType_HasFeature(tp, Py_TPFLAGS_HEAPTYPE)) {
        Py_DECREF(tp);
    }
}

bool python_parse_int(int *out, PyObject *ob) {
    int overflow;
    long tmp = PyLong_AsLongAndOverflow(ob, &overflow);

    if (overflow != 0 || tmp < INT_MIN || tmp > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError, "Python int too large to convert to C int");
        return false;
    }

    *out = tmp;
    return true;
}

bool python_parse_unsigned_int(unsigned int *out, PyObject *ob) {
    unsigned long tmp = PyLong_AsUnsignedLong(ob);
    if (PyErr_Occurred()) {
        return false;
    }

    if (tmp > UINT_MAX) {
        PyErr_SetString(PyExc_OverflowError, "Python int too large to convert to C int");
        return false;
    }

    *out = tmp;
    return true;
}

bool python_parse_unsigned_long_long(unsigned long long *out, PyObject *ob) {
    unsigned long long tmp = PyLong_AsUnsignedLongLong(ob);

    if (PyErr_Occurred()) {
        return false;
    }

    *out = tmp;
    return true;
}

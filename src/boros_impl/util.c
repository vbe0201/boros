/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "util.h"

#include <assert.h>

PyObject *boros_py_alloc(PyTypeObject *tp) {
#ifdef Py_LIMITED_API
    allocfunc tp_alloc = PyType_GetSlot(tp, Py_tp_alloc);
#else
    allocfunc tp_alloc = tp->tp_alloc;
#endif

    assert(tp_alloc != NULL);
    return tp_alloc(tp, 0);
}

void boros_tp_dealloc(PyObject *self) {
    PyTypeObject *tp = Py_TYPE(self);

    /* Untrack the object if it participates in garbage collection. */
    if (PyType_HasFeature(tp, Py_TPFLAGS_HAVE_GC)) {
        PyObject_GC_UnTrack(self);
    }

    /* Clear Python references from the object. */
#ifdef Py_LIMITED_API
    inquiry tp_clear = PyType_GetSlot(tp, Py_tp_clear);
#else
    inquiry tp_clear = tp->tp_clear;
#endif
    if (tp_clear != NULL && tp_clear(self) < 0) {
        PyErr_WriteUnraisable(self);
    }

    /* Deallocate the object memory. */
#ifdef Py_LIMITED_API
    freefunc tp_free = PyType_GetSlot(tp, Py_tp_free);
#else
    freefunc tp_free = tp->tp_free;
#endif
    assert(tp_free != NULL);
    tp_free(self);

    /* Heap types hold a reference to their type. Remove it here. */
    Py_DECREF(tp);
}

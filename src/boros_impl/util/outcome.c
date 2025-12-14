/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "util/outcome.h"

#include <errno.h>
#include <string.h>

static inline PyObject *tag_pointer(PyObject *ob) {
    return (PyObject *)((uintptr_t)ob | 1);
}

static inline PyObject *untag_pointer(PyObject *ob) {
    return (PyObject *)((uintptr_t)ob & ~1ULL);
}

static inline bool is_pointer_tagged(PyObject *ob) {
    return ((uintptr_t)ob & 1) != 0;
}

void outcome_init(Outcome *outcome) {
    outcome->value = NULL;
}

bool outcome_empty(Outcome *outcome) {
    return outcome->value == NULL;
}

int outcome_traverse(Outcome *outcome, visitproc visit, void *arg) {
    PyObject *ob = untag_pointer(outcome->value);
    Py_VISIT(ob);
    return 0;
}

void outcome_clear(Outcome *outcome) {
    PyObject *ob = untag_pointer(outcome->value);
    Py_CLEAR(ob);
    outcome->value = ob;
}

void outcome_store_result(Outcome *outcome, PyObject *ob) {
    outcome->value = tag_pointer(ob);
}

void outcome_store_error(Outcome *outcome, PyObject *err) {
    outcome->value = err;
}

void outcome_capture(Outcome *outcome, PyObject *ob) {
    if (ob != NULL) {
        outcome_store_result(outcome, ob);
    } else {
        outcome_capture_error(outcome);
    }
}

void outcome_capture_error(Outcome *outcome) {
    outcome_store_error(outcome, PyErr_GetRaisedException());
}

void outcome_capture_errno(Outcome *outcome) {
    int e = errno;

    PyObject *error = PyLong_FromLong(e);
    if (error == NULL) {
        outcome_capture_error(outcome);
        return;
    }

    PyObject *message = PyUnicode_DecodeLocale(strerror(e), "surrogateescape");
    if (message == NULL) {
        outcome_capture_error(outcome);
        return;
    }

    PyObject *args[3] = {NULL, error, message};
    Py_ssize_t nargsf = 2 | PY_VECTORCALL_ARGUMENTS_OFFSET;

    PyObject *exc = PyObject_Vectorcall(PyExc_OSError, args + 1, nargsf, NULL);
    if (exc == NULL) {
        outcome_capture_error(outcome);
        return;
    }

    outcome_store_error(outcome, exc);
}

PyObject *outcome_unwrap(Outcome *outcome) {
    bool tagged  = is_pointer_tagged(outcome->value);
    PyObject *ob = untag_pointer(outcome->value);

    outcome->value = NULL;
    if (tagged) {
        return ob;
    } else if (PyErr_Occurred()) {
        PyErr_WriteUnraisable(ob);
        return NULL;
    } else {
        PyErr_SetRaisedException(ob);
        return NULL;
    }
}

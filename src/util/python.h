/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <limits.h>

/* Reusable PyObject allocation function based on tp_alloc. */
static inline PyObject *python_alloc(PyTypeObject *tp) {
    allocfunc tp_alloc = tp->tp_alloc;
    assert(tp_alloc != NULL);
    return tp_alloc(tp, 0);
}

/* Reusable PyObject tp_dealloc slot for custom types. */
void python_tp_dealloc(PyObject *self);

/* Attempts to parse a given PyObject into a C int value. */
bool python_parse_int(int *out, PyObject *ob);

/* Attempts to parse a given PyObject into a C unsigned int value. */
bool python_parse_unsigned_int(unsigned int *out, PyObject *ob);

/* Attempts to parse a given PyObject into a C unsigned long long value. */
bool python_parse_unsigned_long_long(unsigned long long *out, PyObject *ob);

/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <limits.h>

/* Reusable PyObject allocation function based on tp_alloc. */
PyObject *python_alloc(PyTypeObject *tp);

/* Reusable PyObject tp_dealloc slot for custom types. */
void python_tp_dealloc(PyObject *self);

/* Attempts to parse a given PyObject into a C int value. */
bool python_parse_int(int *out, PyObject *ob);

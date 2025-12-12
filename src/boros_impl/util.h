/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>

/* Reusable PyObject allocation function based on tp_alloc. */
PyObject *boros_py_alloc(PyTypeObject *tp);

/* Reusable PyObject tp_dealloc routine for custom types. */
void boros_tp_dealloc(PyObject *self);

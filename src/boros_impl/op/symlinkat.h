/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#pragma once

#include "op/base.h"

typedef struct {
    /* newdirfd is stored in base.scratch */
    Operation base;
    PyObject *target;
    PyObject *linkpath;
} SymlinkAtOperation;

PyObject *symlinkat_operation_create(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf);
PyTypeObject *symlinkat_operation_register(PyObject *mod);

/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#pragma once

#include "op/base.h"

typedef struct {
    Operation base;
    PyObject *oldpath;
    PyObject *newpath;
} RenameOperation;

PyObject *rename_operation_create(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf);
PyTypeObject *rename_operation_register(PyObject *mod);

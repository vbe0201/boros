/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#pragma once

#include "op/base.h"

typedef struct {
    Operation base;
    PyObject *path;
    mode_t mode;
} MkdirOperation;

PyObject *mkdir_operation_create(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf);
PyTypeObject *mkdir_operation_register(PyObject *mod);

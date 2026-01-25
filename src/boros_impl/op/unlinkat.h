/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#pragma once

#include "op/base.h"

typedef struct {
    /* dirfd is stored in base.scratch */
    Operation base;
    PyObject *path;
    int flags;
} UnlinkAtOperation;

PyObject *unlinkat_operation_create(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf);
PyTypeObject *unlinkat_operation_register(PyObject *mod);

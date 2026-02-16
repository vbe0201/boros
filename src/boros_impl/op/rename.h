/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#pragma once

#include "op/base.h"

typedef struct {
    /* olddfd is stored in base.scratch */
    Operation base;
    PyObject *oldpath;
    PyObject *newpath;
    int newdfd;
    unsigned int flags;
} RenameAtOperation;

PyObject *renameat_operation_create(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf);
PyTypeObject *renameat_operation_register(PyObject *mod);

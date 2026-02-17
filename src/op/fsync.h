/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#pragma once

#include "op/base.h"

typedef struct {
    /* fd is stored in base.scratch */
    Operation base;
    unsigned int fsync_flags;
} FsyncOperation;

PyObject *fsync_operation_create(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf);
PyTypeObject *fsync_operation_register(PyObject *mod);

/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#pragma once

#include <sys/types.h>

#include "op/base.h"

typedef struct {
    /* flags is stored in base.scratch */
    Operation base;
    PyObject *path;
    int dfd;
    mode_t mode;
} OpenAtOperation;

PyObject *openat_operation_create(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf);
PyTypeObject *openat_operation_register(PyObject *mod);

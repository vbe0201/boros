/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#pragma once

#include "op/base.h"

typedef struct {
    /* fd is stored in base.scratch */
    Operation base;
    PyObject *buf;
    unsigned int nbytes;
    unsigned long long offset;
} ReadOperation;

PyObject *read_operation_create(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf);
PyTypeObject *read_operation_register(PyObject *mod);

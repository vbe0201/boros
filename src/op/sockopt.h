/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#pragma once

#include "op/base.h"

typedef struct {
    /* fd is stored in base.scratch */
    Operation base;
    PyObject *buf;
    int level;
    int optname;
    int optlen;
    bool raw;
} GetsockoptOperation;

typedef struct {
    /* fd is stored in base.scratch */
    Operation base;
    PyObject *buf;
    int level;
    int optname;
    int optlen;
} SetsockoptOperation;

PyObject *getsockopt_operation_create(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf);
PyTypeObject *getsockopt_operation_register(PyObject *mod);

PyObject *setsockopt_operation_create(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf);
PyTypeObject *setsockopt_operation_register(PyObject *mod);

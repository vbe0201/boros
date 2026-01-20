/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#pragma once

#include "op/base.h"

typedef struct {
    /* flags is stored in base.scratch */
    Operation base;
    int olddirfd;
    int newdirfd;
    PyObject *oldpath;
    PyObject *newpath;
} LinkAtOperation;

PyObject *linkat_operation_create(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf);
PyTypeObject *linkat_operation_register(PyObject *mod);

/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#pragma once

#include "op/base.h"

typedef struct {
    /* cancel fd is stored in base.scratch, if given */
    Operation base;
    Operation *target;
} CancelOperation;

PyObject *cancel_operation_create_fd(PyObject *mod, PyObject *fd);
PyObject *cancel_operation_create_op(PyObject *mod, PyObject *op);
PyTypeObject *cancel_operation_register(PyObject *mod);

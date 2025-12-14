/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#pragma once

#include "op/base.h"

/* Representation of the io_uring nop operation. */
typedef struct {
    Operation base;
} NopOperation;

/* Creates and returns a nop operation. */
PyObject *nop_operation_create(PyObject *mod, PyObject *res);

/* Registers NopOperation as a Python class onto the module. */
PyTypeObject *nop_operation_register(PyObject *mod);

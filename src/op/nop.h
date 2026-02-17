/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#pragma once

#include "op/base.h"

typedef struct {
    Operation base;
} NopOperation;

PyObject *nop_operation_create(PyObject *mod, PyObject *res);
PyTypeObject *nop_operation_register(PyObject *mod);

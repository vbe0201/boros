/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>

typedef struct {
    /* Python type objects that belong to this module. */
    PyTypeObject *Task_type;
    PyTypeObject *Operation_type;
    PyTypeObject *OperationWaiter_type;
    PyTypeObject *NopOperation_type;

    /* The thread-local runtime context. */
    Py_tss_t *local_context;
} ImplState;

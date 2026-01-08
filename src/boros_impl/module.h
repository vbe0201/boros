/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>

typedef struct _ImplState {
    /* Python type objects that belong to this module. */
    PyTypeObject *RunConfig_type;
    PyTypeObject *Task_type;
    PyTypeObject *Operation_type;
    PyTypeObject *OperationWaiter_type;
    PyTypeObject *NopOperation_type;
    PyTypeObject *SocketOperation_type;
    PyTypeObject *OpenAtOperation_type;
    PyTypeObject *ReadOperation_type;
    PyTypeObject *WriteOperation_type;
    PyTypeObject *CloseOperation_type;
    PyTypeObject *CancelOperation_type;
    PyTypeObject *ConnectOperation_type;
    PyTypeObject *MkdirOperation_type;
    PyTypeObject *RenameOperation_type;

    /* The thread-local runtime context. */
    Py_tss_t *local_context;
} ImplState;

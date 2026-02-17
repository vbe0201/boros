/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <liburing.h>

#include "task.h"
#include "util/outcome.h"

/* The Operation state machine lifecycle. */
typedef enum {
    State_Pending,
    State_Blocked,
    State_Ready,
} OperationState;

/* Virtual functions that must be provided by Operation subclasses. */
typedef struct {
    void (*prepare)(PyObject *, struct io_uring_sqe *);
    void (*complete)(PyObject *, struct io_uring_cqe *);
} OperationVTable;

struct _ImplState;

/* Represents the base state of I/O operations in the runtime. */
typedef struct {
    PyObject_HEAD
    OperationVTable *vtable;
    struct _ImplState *module_state;
    Task *awaiter;
    OperationState state;
    int scratch;
    Outcome outcome;
} Operation;

/* State machine for awaiting the completion of an Operation. */
typedef struct {
    PyObject_HEAD
    PyObject *op;
} OperationWaiter;

Operation *operation_alloc(PyTypeObject *tp, struct _ImplState *state);

int operation_traverse(Operation *self, visitproc visit, void *arg);
int operation_clear(Operation *self);

PyTypeObject *operation_register(PyObject *mod);
PyTypeObject *operation_waiter_register(PyObject *mod);

/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "op/cancel.h"

#include <liburing.h>

#include "module.h"
#include "util/python.h"

static void cancel_prepare(PyObject *self, struct io_uring_sqe *sqe) {
    CancelOperation *op = (CancelOperation *)self;

    if (op->target != NULL) {
        /* Cancel a specific operation, if provided. */
        io_uring_prep_cancel(sqe, op->target, 0);
    } else {
        /* Cancel all operations on the fd. */
        io_uring_prep_cancel_fd(sqe, op->base.scratch, IORING_ASYNC_CANCEL_ALL);
    }
}

static void cancel_complete(PyObject *self, struct io_uring_cqe *cqe) {
    Operation *op = (Operation *)self;

    if (cqe->res < 0) {
        errno = -cqe->res;
        outcome_capture_errno(&op->outcome);
    } else {
        outcome_capture(&op->outcome, PyLong_FromLong(cqe->res));
    }
}

static OperationVTable g_cancel_operation_vtable = {
    .prepare  = cancel_prepare,
    .complete = cancel_complete,
};

PyObject *cancel_operation_create_fd(PyObject *mod, PyObject *arg) {
    ImplState *state = PyModule_GetState(mod);

    int fd;
    if (!python_parse_int(&fd, arg)) {
        return NULL;
    }

    CancelOperation *op = (CancelOperation *)operation_alloc(state->CancelOperation_type, state);
    if (op != NULL) {
        op->base.vtable  = &g_cancel_operation_vtable;
        op->target       = NULL;
        op->base.scratch = fd;
    }

    return (PyObject *)op;
}

PyObject *cancel_operation_create_op(PyObject *mod, PyObject *arg) {
    ImplState *state = PyModule_GetState(mod);

    if (PyObject_TypeCheck(arg, state->Operation_type) == 0) {
        PyErr_SetString(PyExc_TypeError, "Expected Operation object");
        return NULL;
    }

    CancelOperation *op = (CancelOperation *)operation_alloc(state->CancelOperation_type, state);
    if (op != NULL) {
        op->base.vtable = &g_cancel_operation_vtable;
        op->target      = (Operation *)Py_NewRef(arg);
    }

    return (PyObject *)op;
}

static int cancel_traverse(PyObject *self, visitproc visit, void *arg) {
    CancelOperation *op = (CancelOperation *)self;

    Py_VISIT(Py_TYPE(self));
    Py_VISIT(op->target);
    return operation_traverse(&op->base, visit, arg);
}

static int cancel_clear(PyObject *self) {
    CancelOperation *op = (CancelOperation *)self;

    Py_CLEAR(op->target);
    return operation_clear(&op->base);
}

static PyType_Slot g_cancel_operation_slots[] = {
    {Py_tp_traverse, cancel_traverse},
    {Py_tp_clear, cancel_clear},
    {0, NULL},
};

static PyType_Spec g_cancel_operation_spec = {
    .name      = "_impl._CancelOperation",
    .basicsize = sizeof(CancelOperation),
    .itemsize  = 0,
    .flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE,
    .slots     = g_cancel_operation_slots,
};

PyTypeObject *cancel_operation_register(PyObject *mod) {
    ImplState *state = PyModule_GetState(mod);
    return (PyTypeObject *)PyType_FromModuleAndSpec(mod, &g_cancel_operation_spec, (PyObject *)state->Operation_type);
}

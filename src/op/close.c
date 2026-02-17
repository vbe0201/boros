/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "op/close.h"

#include "util/python.h"

#include "module.h"

static void close_prepare(PyObject *self, struct io_uring_sqe *sqe) {
    CloseOperation *op = (CloseOperation *)self;
    io_uring_prep_close(sqe, op->base.scratch);
}

static void close_complete(PyObject *self, struct io_uring_cqe *cqe) {
    CloseOperation *op = (CloseOperation *)self;

    if (cqe->res < 0) {
        errno = -cqe->res;
        outcome_capture_errno(&(op->base.outcome));
    } else {
        outcome_capture(&(op->base.outcome), PyLong_FromLong(cqe->res));
    }
}

static OperationVTable g_close_operation_vtable = {
    .prepare  = close_prepare,
    .complete = close_complete,
};

PyObject *close_operation_create(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf) {
    ImplState *state = PyModule_GetState(mod);

    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (nargs != 1) {
        PyErr_Format(PyExc_TypeError, "Expected 1 argument, got %zu instead", nargs);
        return NULL;
    }

    int fd;
    if (!python_parse_int(&fd, args[0])) {
        return NULL;
    }

    CloseOperation *op = (CloseOperation *)operation_alloc(state->CloseOperation_type, state);
    if (op != NULL) {
        op->base.vtable  = &g_close_operation_vtable;
        op->base.scratch = fd;
    }

    return (PyObject *)op;
}

static PyType_Slot g_close_operation_slots[] = {
    {0, NULL},
};

static PyType_Spec g_close_operation_spec = {
    .name      = "_impl._CloseOperation",
    .basicsize = sizeof(CloseOperation),
    .itemsize  = 0,
    .flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE,
    .slots     = g_close_operation_slots,
};

PyTypeObject *close_operation_register(PyObject *mod) {
    ImplState *state = PyModule_GetState(mod);
    return (PyTypeObject *)PyType_FromModuleAndSpec(mod, &g_close_operation_spec, (PyObject *)state->Operation_type);
}

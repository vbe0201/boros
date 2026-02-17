/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "op/accept.h"

#include "util/python.h"

#include "module.h"
#include "util/sockaddr.h"

static void accept_prepare(PyObject *self, struct io_uring_sqe *sqe) {
    AcceptOperation *op = (AcceptOperation *)self;

    op->addrlen = sizeof(op->addr);
    io_uring_prep_accept(sqe, op->base.scratch, (struct sockaddr *)&op->addr, &op->addrlen, op->flags);
}

static void accept_complete(PyObject *self, struct io_uring_cqe *cqe) {
    AcceptOperation *op = (AcceptOperation *)self;

    if (cqe->res < 0) {
        errno = -cqe->res;
        outcome_capture_errno(&op->base.outcome);
    } else {
        PyObject *addr = format_sockaddr((const struct sockaddr *)&op->addr, op->addrlen);
        if (addr != NULL) {
            PyObject *result = PyTuple_Pack(2, PyLong_FromLong(cqe->res), addr);
            Py_DECREF(addr);
            outcome_capture(&op->base.outcome, result);
        } else {
            outcome_capture_error(&op->base.outcome);
        }
    }
}

static OperationVTable g_accept_operation_vtable = {
    .prepare  = accept_prepare,
    .complete = accept_complete,
};

PyObject *accept_operation_create(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf) {
    ImplState *state = PyModule_GetState(mod);

    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (nargs != 2) {
        PyErr_Format(PyExc_TypeError, "Expected 2 arguments, got %zu instead", nargs);
        return NULL;
    }

    int fd;
    if (!python_parse_int(&fd, args[0])) {
        return NULL;
    }

    int flags;
    if (!python_parse_int(&flags, args[1])) {
        return NULL;
    }

    AcceptOperation *op = (AcceptOperation *)operation_alloc(state->AcceptOperation_type, state);
    if (op != NULL) {
        op->base.vtable  = &g_accept_operation_vtable;
        op->base.scratch = fd;
        op->flags        = flags;
    }

    return (PyObject *)op;
}

static PyType_Slot g_accept_operation_slots[] = {
    {0, NULL},
};

static PyType_Spec g_accept_operation_spec = {
    .name      = "_impl._AcceptOperation",
    .basicsize = sizeof(AcceptOperation),
    .itemsize  = 0,
    .flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE,
    .slots     = g_accept_operation_slots,
};

PyTypeObject *accept_operation_register(PyObject *mod) {
    ImplState *state = PyModule_GetState(mod);
    return (PyTypeObject *)PyType_FromModuleAndSpec(mod, &g_accept_operation_spec, (PyObject *)state->Operation_type);
}

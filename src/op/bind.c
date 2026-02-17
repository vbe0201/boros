/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "op/bind.h"

#include <string.h>

#include "module.h"
#include "util/sockaddr.h"

static void bind_prepare(PyObject *self, struct io_uring_sqe *sqe) {
    BindOperation *op = (BindOperation *)self;

    io_uring_prep_bind(sqe, op->base.scratch, (struct sockaddr *)&op->addr, op->addrlen);
}

static void bind_complete(PyObject *self, struct io_uring_cqe *cqe) {
    Operation *op = (Operation *)self;

    if (cqe->res < 0) {
        errno = -cqe->res;
        outcome_capture_errno(&op->outcome);
    } else {
        assert(cqe->res == 0);
        outcome_capture(&op->outcome, Py_None);
    }
}

static OperationVTable g_bind_operation_vtable = {
    .prepare  = bind_prepare,
    .complete = bind_complete,
};

PyObject *bind_operation_create(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf) {
    ImplState *state = PyModule_GetState(mod);

    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (nargs != 3) {
        PyErr_Format(PyExc_TypeError, "Expected 3 arguments, got %zu instead", nargs);
        return NULL;
    }

    int fd;
    if (!python_parse_int(&fd, args[0])) {
        return NULL;
    }

    int af;
    if (!python_parse_int(&af, args[1])) {
        return NULL;
    }

    BindOperation *op = (BindOperation *)operation_alloc(state->BindOperation_type, state);
    if (op == NULL) {
        return NULL;
    }

    op->base.vtable  = &g_bind_operation_vtable;
    op->base.scratch = fd;
    if (!parse_sockaddr(af, args[2], &op->addr, &op->addrlen)) {
        Py_DECREF(op);
        return NULL;
    }

    return (PyObject *)op;
}

static PyType_Slot g_bind_operation_slots[] = {
    {0, NULL},
};

static PyType_Spec g_bind_operation_spec = {
    .name      = "_impl._BindOperation",
    .basicsize = sizeof(BindOperation),
    .itemsize  = 0,
    .flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE,
    .slots     = g_bind_operation_slots,
};

PyTypeObject *bind_operation_register(PyObject *mod) {
    ImplState *state = PyModule_GetState(mod);
    return (PyTypeObject *)PyType_FromModuleAndSpec(mod, &g_bind_operation_spec, (PyObject *)state->Operation_type);
}

/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "op/connect.h"

#include <string.h>

#include "module.h"
#include "util/sockaddr.h"

static void connect_prepare(PyObject *self, struct io_uring_sqe *sqe) {
    ConnectOperation *op = (ConnectOperation *)self;

    io_uring_prep_connect(sqe, op->base.scratch, (const struct sockaddr *)&op->addr, op->addr_len);
}

static void connect_complete(PyObject *self, struct io_uring_cqe *cqe) {
    Operation *op = (Operation *)self;

    if (cqe->res < 0) {
        errno = -cqe->res;
        outcome_capture_errno(&op->outcome);
    } else {
        assert(cqe->res == 0);
        outcome_capture(&op->outcome, Py_None);
    }
}

static OperationVTable g_connect_operation_vtable = {
    .prepare  = connect_prepare,
    .complete = connect_complete,
};

PyObject *connect_operation_create(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf) {
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

    ConnectOperation *op = (ConnectOperation *)operation_alloc(state->ConnectOperation_type, state);
    if (op == NULL) {
        return NULL;
    }

    op->base.vtable  = &g_connect_operation_vtable;
    op->base.scratch = fd;
    if (!parse_sockaddr(af, args[2], &op->addr, &op->addr_len)) {
        Py_DECREF(op);
        return NULL;
    }

    return (PyObject *)op;
}

static PyType_Slot g_connect_operation_slots[] = {
    {0, NULL},
};

static PyType_Spec g_connect_operation_spec = {
    .name      = "_impl._ConnectOperation",
    .basicsize = sizeof(ConnectOperation),
    .itemsize  = 0,
    .flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE,
    .slots     = g_connect_operation_slots,
};

PyTypeObject *connect_operation_register(PyObject *mod) {
    ImplState *state = PyModule_GetState(mod);
    return (PyTypeObject *)PyType_FromModuleAndSpec(mod, &g_connect_operation_spec, (PyObject *)state->Operation_type);
}

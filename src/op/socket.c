/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "op/socket.h"

#include <liburing.h>

#include "module.h"
#include "util/python.h"

static void socket_prepare(PyObject *self, struct io_uring_sqe *sqe) {
    SocketOperation *op = (SocketOperation *)self;

    io_uring_prep_socket(sqe, op->base.scratch, op->type, op->protocol, 0);
}

static void socket_complete(PyObject *self, struct io_uring_cqe *cqe) {
    Operation *op = (Operation *)self;
    outcome_capture(&op->outcome, PyLong_FromLong(cqe->res));
}

static OperationVTable g_socket_operation_vtable = {
    .prepare  = socket_prepare,
    .complete = socket_complete,
};

PyObject *socket_operation_create(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf) {
    ImplState *state = PyModule_GetState(mod);

    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (nargs != 3) {
        PyErr_Format(PyExc_TypeError, "Expected 3 arguments, got %zu instead", nargs);
        return NULL;
    }

    int domain;
    if (!python_parse_int(&domain, args[0])) {
        return NULL;
    }

    int type;
    if (!python_parse_int(&type, args[1])) {
        return NULL;
    }

    int protocol;
    if (!python_parse_int(&protocol, args[2])) {
        return NULL;
    }

    SocketOperation *op = (SocketOperation *)operation_alloc(state->SocketOperation_type, state);
    if (op != NULL) {
        op->base.vtable  = &g_socket_operation_vtable;
        op->base.scratch = domain;
        op->type         = type;
        op->protocol     = protocol;
    }

    return (PyObject *)op;
}

static PyType_Slot g_socket_operation_slots[] = {
    {0, NULL},
};

static PyType_Spec g_socket_operation_spec = {
    .name      = "_impl._SocketOperation",
    .basicsize = sizeof(SocketOperation),
    .itemsize  = 0,
    .flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE,
    .slots     = g_socket_operation_slots,
};

PyTypeObject *socket_operation_register(PyObject *mod) {
    ImplState *state = PyModule_GetState(mod);
    return (PyTypeObject *)PyType_FromModuleAndSpec(mod, &g_socket_operation_spec, (PyObject *)state->Operation_type);
}
